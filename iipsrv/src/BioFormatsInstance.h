/*
 * File:   BioFormatsInstance.h
 */

#ifndef BIOFORMATSINSTANCE_H
#define BIOFORMATSINSTANCE_H

#include <vector>
#include <string>
#include <cstring>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <jni.h>
#include "BioFormatsThread.h"

/*
Memory management
helpful webpages https://stackoverflow.com/questions/2093112
https://stackoverflow.com/questions/10617735
Especially https://stackoverflow.com/a/13940735
Receiving objects gives us local references, which would normally be freed
when we return from the current function, but we don't return to Java,
C++ is our main code, so for us there's no difference between local references
and global references. Global references are normally the ones that won't be freed
on return. Nevertheless use global references for stylistic correctness.

For simplicity don't use jstring but use our common communication_buffer.

To use this library, do:
BioFormatsInstance bfi = BioFormatsManager::get_new();
bfi.bf_open(filename); // or any other methods
...
and when you're done:
BioFormatsManager::free(std::move(bfi));

Or see BioFormatsImage.cc/h for an example of keeping the Java class alive with
the C++ class
*/

// Allow 2048*2048 four channels of 16 bits
#define bfi_communication_buffer_len 33554432

class BioFormatsInstance
{
public:
  // std::unique_ptr<BioFormatsThread> or shared ptr would also work
  static BioFormatsThread jvm;

  char *communication_buffer;

  jobject bfbridge;

  BioFormatsInstance()
  {
    jobject bfbridge_local = jvm.env->NewObject(jvm.bfbridge_base, jvm.constructor);
    // Should be freed
    bfbridge = (jobject)jvm.env->NewGlobalRef(bfbridge_local);
    jvm.env->DeleteLocalRef(bfbridge_local);

    // Should be freed
    communication_buffer = new char[bfi_communication_buffer_len];
    jobject buffer = jvm.env->NewDirectByteBuffer(communication_buffer, bfi_communication_buffer_len);
    if (!buffer)
    {
      // https://stackoverflow.com/questions/32323406/what-happens-if-a-constructor-throws-an-exception
      // Warning: throwing inside a c++ constructor means destructor
      // might not be called, so lines marked "Should be freed" above
      // need to be freed before throwing in a constructor.
      jvm.env->DeleteGlobalRef(bfbridge);
      delete[] communication_buffer;
      fprintf(stderr, "Couldn't allocate %d: too little RAM or JVM JNI doesn't support native memory access?", bfi_communication_buffer_len);
      throw "Allocation failed";
    }
    fprintf(stderr, "classloading almost done %d should be 0\n", (int)jvm.env->ExceptionCheck());
    /*
    How we would do if we hadn't been caching methods beforehand: This way:
    jmethodID BFSetCommunicationBuffer =
      jvm.env->GetMethodID(jvm.bfbridge_base, "BFSetCommunicationBuffer",
      "(Ljava/nio/ByteBuffer;)V"
    );
    jvm.env->CallVoidMethod(bfbridge, BFSetCommunicationBuffer, buffer);
    */
    jvm.env->CallVoidMethod(bfbridge, jvm.BFSetCommunicationBuffer, buffer);
    fprintf(stderr, "classloading almost3 done\n");

    jvm.env->DeleteLocalRef(buffer);
    fprintf(stderr, "classloading hopefully done\n");
  }

  // If we allow copy, the previous one might be destroyed then it'll call
  // destroy VM but while sharing a pointer with the new one
  // so the new one will be broken as well, so use std::move
  // https://www.codementor.io/@sandesh87/the-rule-of-five-in-c-1pdgpzb04f
  // https://en.cppreference.com/w/cpp/language/rule_of_three
  // The alternative is reference counting
  // https://ps.uci.edu/~cyu/p231C/LectureNotes/lecture13:referenceCounting/lecture13.pdf
  // Or the simplest way would be to use a pointer to BioFormatsInstance and never use it directly. Pointers are easier to move manually
  // count, in an int*, the number of copies and deallocate when reach 0

  // We can't use default ones because we want to avoid leaking classes.
  // jclass must be tracked until its destruction and freed manually.
  // When we move-construct or move-assign the previous one should be destroyed,
  // otherwise the previous one's destructor will free it so the new
  // one will be broken. That's why we can't use defaults instead of Rule of 5.
  // Here we choose to reuse jclasses rather than to destroy
  // and recreate them unnecessarily.
  BioFormatsInstance(const BioFormatsInstance &) = delete;
  /*BioFormatsInstance(BioFormatsInstance &&) = default;*/
  BioFormatsInstance(BioFormatsInstance &&other)
  {
    bfbridge = other.bfbridge;
    communication_buffer = other.communication_buffer;
    other.bfbridge = nullptr;
    other.communication_buffer = nullptr;
  }
  BioFormatsInstance &operator=(const BioFormatsInstance &) = delete;
  BioFormatsInstance &operator=(BioFormatsInstance &&other)
  {
    bfbridge = other.bfbridge;
    communication_buffer = other.communication_buffer;
    other.bfbridge = nullptr;
    other.communication_buffer = nullptr;
    return *this;
  }

  ~BioFormatsInstance()
  {
    if (bfbridge)
    {
      jvm.env->DeleteGlobalRef(bfbridge);
    }
    if (communication_buffer)
    {
      delete[] communication_buffer;
    }
  }

  // changed ownership: user opened new file, etc.
  void refresh()
  {
    fprintf(stderr, "calling refresh\n");
    jvm.env->CallVoidMethod(bfbridge, jvm.BFClose);
    fprintf(stderr, "called refresh\n");
  }

  std::string bf_get_error()
  {
    int len = jvm.env->CallIntMethod(bfbridge, jvm.BFGetErrorLength);
    std::string err;
    err.assign(communication_buffer, len);
    return err;
  }

  int bf_is_compatible(std::string filepath)
  {
    fprintf(stderr, "calling is compatible %s\n", filepath.c_str());
    int len = filepath.length();
    memcpy(communication_buffer, filepath.c_str(), len);
    fprintf(stderr, "called is compatible\n");
    return jvm.env->CallIntMethod(bfbridge, jvm.BFIsCompatible, len);
  }

  int bf_open(std::string filepath)
  {
    int len = filepath.length();
    memcpy(communication_buffer, filepath.c_str(), len);
    return jvm.env->CallIntMethod(bfbridge, jvm.BFOpen, len);
  }

  int bf_close()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFClose);
  }

  int bf_get_resolution_count()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetResolutionCount);
  }

  int bf_set_current_resolution(int res)
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFSetCurrentResolution, res);
  }

  int bf_set_series(int ser)
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFSetSeries, ser);
  }

  int bf_get_series_count()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetSeriesCount);
  }

  int bf_get_size_x()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetSizeX);
  }

  int bf_get_size_y()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetSizeY);
  }

  int bf_get_size_z()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetSizeZ);
  }

  int bf_get_size_c()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetSizeC);
  }

  int bf_get_size_t()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetSizeT);
  }

  int bf_get_effective_size_c()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetEffectiveSizeC);
  }

  int bf_get_optimal_tile_width()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetOptimalTileWidth);
  }

  int bf_get_optimal_tile_height()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetOptimalTileHeight);
  }

  int bf_get_pixel_type()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetPixelType);
  }

  int bf_get_bytes_per_pixel()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetBytesPerPixel);
  }

  int bf_get_rgb_channel_count()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetRGBChannelCount);
  }

  int bf_get_image_count()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFGetImageCount);
  }

  int bf_is_rgb()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFIsRGB);
  }

  int bf_is_interleaved()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFIsInterleaved);
  }

  int bf_is_little_endian()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFIsLittleEndian);
  }

  int bf_is_false_color()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFIsFalseColor);
  }

  int bf_is_indexed_color()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFIsIndexedColor);
  }

  // Returns char* to communication_buffer that will later be overwritten
  char *bf_get_dimension_order()
  {
    int len = jvm.env->CallIntMethod(bfbridge, jvm.BFGetDimensionOrder);
    if (len < 0)
    {
      return NULL;
    }
    // We know that for this function len can never be close to
    // bfi_communication_buffer_len, so no writing past it
    communication_buffer[len] = 0;
    return communication_buffer;
  }

  int bf_is_order_certain()
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFIsOrderCertain);
  }

  int bf_open_bytes(int x, int y, int w, int h)
  {
    return jvm.env->CallIntMethod(bfbridge, jvm.BFOpenBytes, x, y, w, h);
  }
};

#endif /* BIOFORMATSINSTANCE_H */

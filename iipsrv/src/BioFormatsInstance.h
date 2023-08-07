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
  static BioFormatsThread thread;

  bfbridge_instance_t bfinstance;

  BioFormatsInstance();

  // If we allow copy, the previous one might be destroyed then it'll call
  // destroy VM but while sharing a pointer with the new one
  // so the new one will be broken as well, so use std::move
  // https://www.codementor.io/@sandesh87/the-rule-of-five-in-c-1pdgpzb04f
  // https://en.cppreference.com/w/cpp/language/rule_of_three
  // The alternative is reference counting
  // https://ps.uci.edu/~cyu/p231C/LectureNotes/lecture13:referenceCounting/lecture13.pdf
  // Or the simplest way would be to use a pointer to BioFormatsInstance.
  // Pointers are easier to move manually and count, in an int*, the number of copies and deallocate when reach 0

  // We can't use default ones because we want to avoid leaking classes.
  // jclass must be tracked until its destruction and freed manually.
  // When we move-construct or move-assign the previous references should be destroyed,
  // otherwise the previous one's destructor will free it so the new
  // one will be broken. That's why we can't use defaults instead of Rule of 5.
  // Here we choose to reuse jclasses rather than to destroy
  // and recreate them unnecessarily.
  BioFormatsInstance(const BioFormatsInstance &) = delete;
  BioFormatsInstance(BioFormatsInstance &&other)
  {
    // Copy both the Java instance class and the communication buffer
    bfinstance = other.bfinstance;
    other.bfinstance = null;
  }
  BioFormatsInstance &operator=(const BioFormatsInstance &) = delete;
  BioFormatsInstance &operator=(BioFormatsInstance &&other)
  {
    bfinstance = other.bfinstance;
    other.bfinstance = null;
    return *this;
  }

  ~BioFormatsInstance()
  {
    char *communication_buffer =
      bfbridge_instance_get_communication_buffer(bfinstance, NULL);
    delete[] communication_buffer;

    bfbridge_free_instance(&bfinstance, &thread.bflibrary);
  }

  // changed ownership: user opened new file, etc.
  void refresh()
  {
    fprintf(stderr, "calling refresh\n");
    // closing current file will help garbage collector do more
    //jvm.env->CallVoidMethod(bfbridge, jvm.BFClose);
    thread.bflibrary.env->CallVoidMethod(bfinstance.bfbridge, thread.bflibrary.BFClose);
    fprintf(stderr, "called refresh\n");
  }

  std::string bf_get_error()
  {
    int len = thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetErrorLength);
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
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFIsCompatible, len);
  }

  int bf_open(std::string filepath)
  {
    int len = filepath.length();
    memcpy(communication_buffer, filepath.c_str(), len);
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFOpen, len);
  }

  int bf_close()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFClose);
  }

  int bf_get_resolution_count()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetResolutionCount);
  }

  int bf_set_current_resolution(int res)
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFSetCurrentResolution, res);
  }

  int bf_set_series(int ser)
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFSetSeries, ser);
  }

  int bf_get_series_count()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetSeriesCount);
  }

  int bf_get_size_x()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetSizeX);
  }

  int bf_get_size_y()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetSizeY);
  }

  int bf_get_size_z()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetSizeZ);
  }

  int bf_get_size_c()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetSizeC);
  }

  int bf_get_size_t()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetSizeT);
  }

  int bf_get_effective_size_c()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetEffectiveSizeC);
  }

  int bf_get_optimal_tile_width()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetOptimalTileWidth);
  }

  int bf_get_optimal_tile_height()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetOptimalTileHeight);
  }

  int bf_get_pixel_type()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetPixelType);
  }

  int bf_get_bytes_per_pixel()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetBytesPerPixel);
  }

  int bf_get_rgb_channel_count()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetRGBChannelCount);
  }

  int bf_get_image_count()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetImageCount);
  }

  int bf_is_rgb()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFIsRGB);
  }

  int bf_is_interleaved()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFIsInterleaved);
  }

  int bf_is_little_endian()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFIsLittleEndian);
  }

  int bf_is_false_color()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFIsFalseColor);
  }

  int bf_is_indexed_color()
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFIsIndexedColor);
  }

  // Returns char* to communication_buffer that will later be overwritten
  char *bf_get_dimension_order()
  {
    int len = thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFGetDimensionOrder);
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
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFIsOrderCertain);
  }

  int bf_open_bytes(int x, int y, int w, int h)
  {
    return thread.bflibrary.env->CallIntMethod(bfinstance.bfbridge, thread.bflibrary.BFOpenBytes, x, y, w, h);
  }
};

#endif /* BIOFORMATSINSTANCE_H */

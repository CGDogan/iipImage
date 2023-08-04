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
and global references. Global references are the ones that won't be freed
when returning. Nevertheless use global references for stylistic correctness.

For simplicity don't use jstring but use our common communication_buffer
*/

// Allow 2048*2048 four channels of 16 bits
#define bfi_communication_buffer_len 33554432

class BioFormatsInstance
{
public:
  static std::unique_ptr<BioFormatsThread> jvm;
  char *communication_buffer;

  // use: javap (-s) (-p) org.camicroscope.BFBridge
  // after "javac -cp ".:jar_files/*" org/camicroscope/BFBridge.java"
  // in the bfbridge directory to see param descriptors such as:
  /*
static void BFSetCommunicationBuffer(java.nio.ByteBuffer);
  descriptor: (Ljava/nio/ByteBuffer;)V
static void BFReset();
  descriptor: ()V
static int BFGetErrorLength();
  descriptor: ()I
...
  descriptor: ()I
static int BFToolsGenerateSubresolutions(int, int, int);
  descriptor: (III)I
*/
  /*jmethodID BFSetCommunicationBuffer;
  jmethodID BFReset;
  jmethodID BFGetErrorLength;
  jmethodID BFIsCompatible;
  jmethodID BFOpen;
  jmethodID BFIsSingleFile;
  jmethodID BFGetUsedFiles;
  jmethodID BFGetCurrentFile;
  jmethodID BFClose;
  jmethodID BFGetResolutionCount;
  jmethodID BFSetCurrentResolution;
  jmethodID BFSetSeries;
  jmethodID BFGetSeriesCount;
  jmethodID BFGetSizeX;
  jmethodID BFGetSizeY;
  jmethodID BFGetSizeZ;
  jmethodID BFGetSizeT;
  jmethodID BFGetSizeC;
  jmethodID BFGetEffectiveSizeC;
  jmethodID BFGetOptimalTileWidth;
  jmethodID BFGetOptimalTİleHeight;
  jmethodID BFGetFormat;
  jmethodID BFGetPixelType;
  jmethodID BFGetBitsPerPixel;
  jmethodID BFGetBytesPerPixel;
  jmethodID BFGetRGBChannelCount;
  jmethodID BFGetImageCount;
  jmethodID BFIsRGB;
  jmethodID BFIsInterleaved;
  jmethodID BFIsLittleEndian;
  jmethodID BFIsFalseColor;
  jmethodID BFIsIndexedColor;
  jmethodID BFGetDimensionOrder;
  jmethodID BFIsOrderCertain;
  jmethodID BFOpenBytes;
  jmethodID BFGetMPPX; // double
  jmethodID BFGetMPPY; // double
  jmethodID BFGetMPPZ; // double
  jmethodID BFIsAnyFileOpen;
  jmethodID BFToolsShouldGenerate;
  jmethodID BFToolsGenerateSubresolutions;*/

  jobject bfbridge;

  BioFormatsInstance()
  {
    jobject bfbridge_local = jvm->env->NewObject(jvm->bfbridge_base, jvm->constructor);
    // Needs freeing
    bfbridge = (jobject)jvm->env->NewGlobalRef(bfbridge_local);
    jvm->env->DeleteLocalRef(bfbridge_local);

    // Needs freeing
    communication_buffer = new char[bfi_communication_buffer_len];
    jobject buffer = jvm->env->NewDirectByteBuffer(communication_buffer, bfi_communication_buffer_len);
    if (!buffer)
    {
      // https://stackoverflow.com/questions/32323406/what-happens-if-a-constructor-throws-an-exception
      // Warning: throwing inside a c++ constructor means destructor
      // might not be called, so lines marked "Needs freeing" above
      // need to be freed before throwing in a constructor.
      jvm->env->DeleteGlobalRef(bfbridge);
      delete[] communication_buffer;
      fprintf(stderr, "Couldn't allocate %d: too little RAM or JVM JNI doesn't support native memory access?", bfi_communication_buffer_len);
      throw "Allocation failed";
    }
    fprintf(stderr, "classloading almost done %d should be 0\n", (int)jvm->env->ExceptionCheck());
    /*jmethodID bufferSetter = jvm->env->GetMethodID(jvm->bfbridge_base, "BFSetCommunicationBuffer", "(Ljava/nio/ByteBuffer;)V");
    fprintf(stderr, "classloading almost2 done %p\n", bufferSetter);*/
    jvm->env->CallVoidMethod(bfbridge, jvm->BFSetCommunicationBuffer, buffer);
    fprintf(stderr, "classloading almost3 done\n");

    jvm->env->DeleteLocalRef(buffer);
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
      jvm->env->DeleteGlobalRef(bfbridge);
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
    jmethodID close = jvm->env->GetMethodID(jvm->bfbridge_base, "BFClose", "()I");
    fprintf(stderr, "mid called refresh\n");
    jvm->env->CallVoidMethod(bfbridge, close);
    fprintf(stderr, "called refresh\n");
  }

  std::string bf_get_error()
  {
    jmethodID BFGetErrorLength = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetErrorLength", "()I");
    int len = jvm->env->CallIntMethod(bfbridge, BFGetErrorLength);
    std::string err;
    err.assign(communication_buffer, len);
    return err;
  }

  int bf_is_compatible(std::string filepath)
  {
    fprintf(stderr, "calling is compatible %s\n", filepath.c_str());
    jmethodID BFIsCompatible = jvm->env->GetMethodID(jvm->bfbridge_base, "BFIsCompatible", "(I)I");
    int len = filepath.length();
    memcpy(communication_buffer, filepath.c_str(), len);
    fprintf(stderr, "called is compatible\n");
    return jvm->env->CallIntMethod(bfbridge, BFIsCompatible, len);
  }

  int bf_open(std::string filepath)
  {

    jmethodID BFOpen = jvm->env->GetMethodID(jvm->bfbridge_base, "BFOpen", "(I)I");
    int len = filepath.length();
    memcpy(communication_buffer, filepath.c_str(), len);
    return jvm->env->CallIntMethod(bfbridge, BFOpen, len);
  }

  int bf_close()
  {

    jmethodID BFClose = jvm->env->GetMethodID(jvm->bfbridge_base, "BFClose", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFClose);
  }

  int bf_get_resolution_count()
  {
    jmethodID BFGetResolutionCount = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetResolutionCount", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetResolutionCount);
  }

  int bf_set_current_resolution(int res)
  {
    jmethodID BFSetResolutionCount = jvm->env->GetMethodID(jvm->bfbridge_base, "BFSetCurrentResolution", "(I)I");
    return jvm->env->CallIntMethod(bfbridge, BFSetResolutionCount, res);
  }

  int bf_set_series(int ser)
  {
    jmethodID BFSetSeries = jvm->env->GetMethodID(jvm->bfbridge_base, "BFSetSeries", "(I)I");
    return jvm->env->CallIntMethod(bfbridge, BFSetSeries, ser);
  }

  int bf_get_series_count()
  {
    jmethodID BFGetSeriesCount = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetSeriesCount", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetSeriesCount);
  }

  int bf_get_size_x()
  {
    jmethodID BFGetSizeX = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetSizeX", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetSizeX);
  }

  int bf_get_size_y()
  {
    jmethodID BFGetSizeY = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetSizeY", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetSizeY);
  }

  int bf_get_size_z()
  {
    jmethodID BFGetSizeZ = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetSizeZ", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetSizeZ);
  }

  int bf_get_size_c()
  {
    jmethodID BFGetSizeC = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetSizeC", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetSizeC);
  }

  int bf_get_size_t()
  {
    jmethodID BFGetSizeT = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetSizeT", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetSizeT);
  }

  int bf_get_effective_size_c()
  {
    jmethodID BFGetEffectiveSizeC = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetEffectiveSizeC", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetEffectiveSizeC);
  }

  int bf_get_optimal_tile_width()
  {
    jmethodID BFGetOptimalTileWidth = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetOptimalTileWidth", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetOptimalTileWidth);
  }

  int bf_get_optimal_tile_height()
  {
    jmethodID BFGetOptimalTileHeight = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetOptimalTileHeight", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetOptimalTileHeight);
  }

  int bf_get_pixel_type()
  {
    jmethodID BFGetPixelType = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetPixelType", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetPixelType);
  }

  int bf_get_bytes_per_pixel()
  {
    jmethodID BFGetBytesPerPixel = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetBytesPerPixel", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetBytesPerPixel);
  }

  int bf_get_rgb_channel_count()
  {
    jmethodID BFGetRGBChannelCount = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetRGBChannelCount", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetRGBChannelCount);
  }

  int bf_get_image_count()
  {
    jmethodID BFGetImageCount = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetImageCount", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFGetImageCount);
  }

  int bf_is_rgb()
  {
    jmethodID BFIsRGB = jvm->env->GetMethodID(jvm->bfbridge_base, "BFIsRGB", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFIsRGB);
  }

  int bf_is_interleaved()
  {
    jmethodID BFIsInterleaved = jvm->env->GetMethodID(jvm->bfbridge_base, "BFIsInterleaved", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFIsInterleaved);
  }

  int bf_is_little_endian()
  {
    jmethodID BFIsLittleEndian = jvm->env->GetMethodID(jvm->bfbridge_base, "BFIsLittleEndian", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFIsLittleEndian);
  }

  int bf_is_false_color()
  {
    jmethodID BFIsFalseColor = jvm->env->GetMethodID(jvm->bfbridge_base, "BFIsFalseColor", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFIsFalseColor);
  }

  int bf_is_indexed_color()
  {
    jmethodID BFIsIndexedColor = jvm->env->GetMethodID(jvm->bfbridge_base, "BFIsIndexedColor", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFIsIndexedColor);
  }

  // Returns char* to communication_buffer that will later be overwritten
  char *bf_get_dimension_order()
  {
    jmethodID BFGetDimensionOrder = jvm->env->GetMethodID(jvm->bfbridge_base, "BFGetDimensionOrder", "()I");
    int len = jvm->env->CallIntMethod(bfbridge, BFGetDimensionOrder);
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
    jmethodID BFIsOrderCertain = jvm->env->GetMethodID(jvm->bfbridge_base, "BFIsOrderCertain", "()I");
    return jvm->env->CallIntMethod(bfbridge, BFIsOrderCertain);
  }

  int bf_open_bytes(int x, int y, int w, int h)
  {
    jmethodID BFOpenBytes = jvm->env->GetMethodID(jvm->bfbridge_base, "BFOpenBytes", "(IIII)I");
    return jvm->env->CallIntMethod(bfbridge, BFOpenBytes, x, y, w, h);
  }
};

#endif /* BIOFORMATSINSTANCE_H */

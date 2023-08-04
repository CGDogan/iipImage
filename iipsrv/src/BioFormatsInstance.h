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
when returning.

For simplicity don't use jstring but use our common communication_buffer
*/

#define communication_buffer_len 33554432

class BioFormatsInstance
{
public:
  static std::unique_ptr<BioFormatsThread> jvm;
  char *communication_buffer;

  // javap (-s) (-p) org.camicroscope.BFBridge
  /*
static void BFSetCommunicationBuffer(java.nio.ByteBuffer);
  descriptor: (Ljava/nio/ByteBuffer;)V
static void BFReset();
  descriptor: ()V
static int BFGetErrorLength();
  descriptor: ()I
static int BFIsCompatible(int);
  descriptor: (I)I
static int BFOpen(int);
  descriptor: (I)I
static int BFIsSingleFile(int);
  descriptor: (I)I
static int BFGetUsedFiles();
  descriptor: ()I
static int BFGetCurrentFile();
  descriptor: ()I
static int BFClose();
  descriptor: ()I
static int BFGetResolutionCount();
  descriptor: ()I
static int BFSetCurrentResolution(int);
  descriptor: (I)I
static int BFSetSeries(int);
  descriptor: (I)I
static int BFGetSeriesCount();
  descriptor: ()I
static int BFGetSizeX();
  descriptor: ()I
static int BFGetSizeY();
  descriptor: ()I
static int BFGetSizeZ();
  descriptor: ()I
static int BFGetSizeT();
  descriptor: ()I
static int BFGetSizeC();
  descriptor: ()I
static int BFGetEffectiveSizeC();
  descriptor: ()I
static int BFGetOptimalTileWidth();
  descriptor: ()I
static int BFGetOptimalTİleHeight();
  descriptor: ()I
static int BFGetFormat();
  descriptor: ()I
static int BFGetPixelType();
  descriptor: ()I
static int BFGetBitsPerPixel();
  descriptor: ()I
static int BFGetBytesPerPixel();
  descriptor: ()I
static int BFGetRGBChannelCount();
  descriptor: ()I
static int BFGetImageCount();
  descriptor: ()I
static int BFIsRGB();
  descriptor: ()I
static int BFIsInterleaved();
  descriptor: ()I
static int BFIsLittleEndian();
  descriptor: ()I
static int BFIsFalseColor();
  descriptor: ()I
static int BFIsIndexedColor();
  descriptor: ()I
static int BFGetDimensionOrder();
  descriptor: ()I
static int BFIsOrderCertain();
  descriptor: ()I
static int BFOpenBytes(int, int, int, int);
  descriptor: (IIII)I
static double BFGetMPPX();
  descriptor: ()D
static double BFGetMPPY();
  descriptor: ()D
static double BFGetMPPZ();
  descriptor: ()D
static int BFIsAnyFileOpen();
  descriptor: ()I
static int BFToolsShouldGenerate();
  descriptor: ()I
static int BFToolsGenerateSubresolutions(int, int, int);
  descriptor: (III)I
*/
  jclass bfbridge;
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

  BioFormatsInstance()
  {
      jclass bfbridge_local = jvm->env->FindClass("org/camicroscope/BFBridge");
      if (!bfbridge_local)
      {
        fprintf(stderr, "org.camicroscope.BFBridge (or a dependency of it) could not be found; is the jar in %s ?\n", jvm->cp.c_str());
        if (jvm->env->ExceptionCheck() == 1)
        {
          fprintf(stderr, "exception\n");
          jvm->env->ExceptionDescribe();
        }
        else
          fprintf(stderr, "no exception\n");

        throw "org.camicroscope.BFBridge could not be found; is the jar in %s ?\n" + jvm->cp;
      }

      bfbridge = (jclass)jvm->env->NewGlobalRef(bfbridge_local);
      fprintf(stderr, "bfbridge %p\n", bfbridge);
      jvm->env->DeleteLocalRef(bfbridge_local);

      // Allow 2048*2048 four channels of 16 bits
      communication_buffer = new char[communication_buffer_len];
      jobject buffer = jvm->env->NewDirectByteBuffer(communication_buffer, communication_buffer_len);
      if (!buffer)
      {
        fprintf(stderr, "Couldn't allocate %d: too little RAM or JVM JNI doesn't support native memory access?", communication_buffer_len);
        throw "Allocation failed";
      }
      jmethodID bufferSetter = jvm->env->GetStaticMethodID(bfbridge, "BFSetCommunicationBuffer", "(Ljava/nio/ByteBuffer;)V");
      jvm->env->CallStaticVoidMethod(bfbridge, bufferSetter, buffer);
      jvm->env->DeleteLocalRef(buffer);
    }
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
  BioFormatsInstance(const BioFormatsInstance &) = delete;
  /*BioFormatsInstance(BioFormatsInstance &&) = default;*/
  BioFormatsInstance(BioFormatsInstance &&other)
  {
    // jvm = other.jvm;
    // jvm->env = other.jvm->env;
    communication_buffer = other.communication_buffer;
    // other.jvm = NULL;
  }
  BioFormatsInstance &operator=(const BioFormatsInstance &) = delete;
  /*BioFormatsInstance &operator=(BioFormatsInstance &&) = default;*/
  // We cant use these two default ones because std::move still keeps
  // the previous object so it calls the destructor.
  // the previous one cannot call destructor now; set its jvm pointer to null
  // so that it can't break the new class.
  BioFormatsInstance &operator=(BioFormatsInstance &&other)
  {
    /*jvm = other.jvm;
    jvm->env = other.jvm->env;*/
    communication_buffer = other.communication_buffer;
    // other.jvm = NULL;
    return *this;
  }

  ~BioFormatsInstance()
  {
    if (jvm)
    {
      // Not needed: destroy vm already
      // jvm->env->DeleteGlobalRef(bfbridge);
      // ...
      // TODO: fix me
      /*jvm->DestroyJavaVM();
      delete[] communication_buffer;*/
    }
  }

  // changed ownership
  void refresh()
  {
    fprintf(stderr, "calling refresh\n");
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID close = jvm->env->GetStaticMethodID(bfbridge, "BFClose", "()I");
    fprintf(stderr, "mid called refresh\n");
    jvm->env->CallStaticVoidMethod(bfbridge, close);
    fprintf(stderr, "called refresh\n");
  }

  std::string bf_get_error()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetErrorLength = jvm->env->GetStaticMethodID(bfbridge, "BFGetErrorLength", "()I");
    int len = jvm->env->CallStaticIntMethod(bfbridge, BFGetErrorLength);
    std::string err;
    err.assign(communication_buffer, len);
    return err;
  }

  int bf_is_compatible(std::string filepath)
  {
    fprintf(stderr, "calling is compatible\n");
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsCompatible = jvm->env->GetStaticMethodID(bfbridge, "BFIsCompatible", "(I)I");
    int len = filepath.length();
    memcpy(communication_buffer, filepath.c_str(), len);
    fprintf(stderr, "called is compatible\n");
    return jvm->env->CallStaticIntMethod(bfbridge, BFIsCompatible, len);
  }

  int bf_open(std::string filepath)
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");

    jmethodID BFOpen = jvm->env->GetStaticMethodID(bfbridge, "BFOpen", "(I)I");
    int len = filepath.length();
    memcpy(communication_buffer, filepath.c_str(), len);
    return jvm->env->CallStaticIntMethod(bfbridge, BFOpen, len);
  }

  int bf_close()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");

    jmethodID BFClose = jvm->env->GetStaticMethodID(bfbridge, "BFClose", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFClose);
  }

  int bf_get_resolution_count()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetResolutionCount = jvm->env->GetStaticMethodID(bfbridge, "BFGetResolutionCount", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetResolutionCount);
  }

  int bf_set_current_resolution(int res)
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFSetResolutionCount = jvm->env->GetStaticMethodID(bfbridge, "BFSetCurrentResolution", "(I)I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFSetResolutionCount, res);
  }

  int bf_set_series(int ser)
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFSetSeries = jvm->env->GetStaticMethodID(bfbridge, "BFSetSeries", "(I)I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFSetSeries, ser);
  }

  int bf_get_series_count()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetSeriesCount = jvm->env->GetStaticMethodID(bfbridge, "BFGetSeriesCount", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetSeriesCount);
  }

  int bf_get_size_x()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetSizeX = jvm->env->GetStaticMethodID(bfbridge, "BFGetSizeX", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetSizeX);
  }

  int bf_get_size_y()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetSizeY = jvm->env->GetStaticMethodID(bfbridge, "BFGetSizeY", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetSizeY);
  }

  int bf_get_size_z()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetSizeZ = jvm->env->GetStaticMethodID(bfbridge, "BFGetSizeZ", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetSizeZ);
  }

  int bf_get_size_c()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetSizeC = jvm->env->GetStaticMethodID(bfbridge, "BFGetSizeC", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetSizeC);
  }

  int bf_get_size_t()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetSizeT = jvm->env->GetStaticMethodID(bfbridge, "BFGetSizeT", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetSizeT);
  }

  int bf_get_effective_size_c()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetEffectiveSizeC = jvm->env->GetStaticMethodID(bfbridge, "BFGetEffectiveSizeC", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetEffectiveSizeC);
  }

  int bf_get_optimal_tile_width()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetOptimalTileWidth = jvm->env->GetStaticMethodID(bfbridge, "BFGetOptimalTileWidth", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetOptimalTileWidth);
  }

  int bf_get_optimal_tile_height()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetOptimalTileHeight = jvm->env->GetStaticMethodID(bfbridge, "BFGetOptimalTileHeight", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetOptimalTileHeight);
  }

  int bf_get_pixel_type()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetPixelType = jvm->env->GetStaticMethodID(bfbridge, "BFGetPixelType", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetPixelType);
  }

  int bf_get_bytes_per_pixel()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetBytesPerPixel = jvm->env->GetStaticMethodID(bfbridge, "BFGetBytesPerPixel", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetBytesPerPixel);
  }

  int bf_get_rgb_channel_count()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetRGBChannelCount = jvm->env->GetStaticMethodID(bfbridge, "BFGetRGBChannelCount", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetRGBChannelCount);
  }

  int bf_get_image_count()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetImageCount = jvm->env->GetStaticMethodID(bfbridge, "BFGetImageCount", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFGetImageCount);
  }

  int bf_is_rgb()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsRGB = jvm->env->GetStaticMethodID(bfbridge, "BFIsRGB", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFIsRGB);
  }

  int bf_is_interleaved()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsInterleaved = jvm->env->GetStaticMethodID(bfbridge, "BFIsInterleaved", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFIsInterleaved);
  }

  int bf_is_little_endian()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsLittleEndian = jvm->env->GetStaticMethodID(bfbridge, "BFIsLittleEndian", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFIsLittleEndian);
  }

  int bf_is_false_color()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsFalseColor = jvm->env->GetStaticMethodID(bfbridge, "BFIsFalseColor", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFIsFalseColor);
  }

  int bf_is_indexed_color()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsIndexedColor = jvm->env->GetStaticMethodID(bfbridge, "BFIsIndexedColor", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFIsIndexedColor);
  }

  // Returns char* to communication_buffer that will later be overwritten
  char *bf_get_dimension_order()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetDimensionOrder = jvm->env->GetStaticMethodID(bfbridge, "BFGetDimensionOrder", "()I");
    int len = jvm->env->CallStaticIntMethod(bfbridge, BFGetDimensionOrder);
    if (len < 0)
    {
      return NULL;
    }
    // We know that for this function len can never be close to
    // communication_buffer_length, so no writing past it
    communication_buffer[len] = 0;
    return communication_buffer;
  }

  int bf_is_order_certain()
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsOrderCertain = jvm->env->GetStaticMethodID(bfbridge, "BFIsOrderCertain", "()I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFIsOrderCertain);
  }

  int bf_open_bytes(int x, int y, int w, int h)
  {
    bfbridge = jvm->env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFOpenBytes = jvm->env->GetStaticMethodID(bfbridge, "BFOpenBytes", "(IIII)I");
    return jvm->env->CallStaticIntMethod(bfbridge, BFOpenBytes, x, y, w, h);
  }
};

#endif /* BIOFORMATSINSTANCE_H */

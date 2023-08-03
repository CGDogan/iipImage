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
#include <jni.h>

// TODO: use its equivalent on Windows
#include <dirent.h>

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

class BioFormatsInstance
{
public:
  const int communication_buffer_len = 33554432;

  // TODO make some of these private
  JavaVM *jvm;
  JNIEnv *env;
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
    fprintf(stderr, "initializing\n");
    // https://docs.oracle.com/en/java/javase/20/docs/specs/jni/invocation.html
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_20;
    JavaVMOption *options = new JavaVMOption[2];

#ifndef BFBRIDGE_CLASSPATH
#error Please define BFBRIDGE_CLASSPATH to the path with compiled classes and dependency jars. Example: gcc -DBFBRIDGE_CLASSPATH=/usr/lib/java
#endif

// https://stackoverflow.com/a/2411008
// define with compiler's -D flag
#define BFBRIDGE_STRINGARG(s) #s
#define BFBRIDGE_STRINGVALUE(s) BFBRIDGE_STRINGARG(s)

    std::string cp = BFBRIDGE_STRINGVALUE(BFBRIDGE_CLASSPATH);
    if (cp.back() != '/')
    {
      cp += "/";
    }

    std::string path_arg = "-Djava.class.path=" + cp + "*:" + cp;

    // For some reason unlike the -cp arg, .../* does not work
    // so we need to list every jar file
    // https://en.cppreference.com/w/cpp/filesystem/directory_iterator
    DIR *cp_dir = opendir(cp.c_str());
    if (!cp_dir)
    {
      fprintf(stderr, "could not read classpath dir %s\n", cp.c_str());
      throw "Could not read dir";
    }
    struct dirent *cp_dirent;
    while ((cp_dirent = readdir(cp_dir)) != NULL)
    {
      path_arg += ":" + cp + std::string(cp_dirent->d_name);
    }
    closedir(cp_dir);

    /*char path_arg[] = "-Djava.class.path=" BFBRIDGE_STRINGVALUE(BFBRIDGE_CLASSPATH) ":"  "/*:" BFBRIDGE_STRINGVALUE(BFBRIDGE_CLASSPATH) "/formats-api-6.13.0.jar";*/
    fprintf(stderr, "Java classpath (BFBRIDGE_CLASSPATH): %s\n", path_arg.c_str());
    // https://docs.oracle.com/en/java/javase/20/docs/specs/man/java.html#performance-tuning-examples
    char optimize1[] = "-XX:+UseParallelGC";
    // char optimize2[] = "-XX:+UseLargePages"; Not compatible with our linux distro
    options[0].optionString = (char *)path_arg.c_str();
    options[1].optionString = optimize1;
    // options[2].optionString = optimize2;
    // options[3].optionString = "-verbose:jni";
    vm_args.options = options;
    vm_args.nOptions = 2; // 3
    vm_args.ignoreUnrecognized = false;
    int code = JNI_CreateJavaVM(&jvm, (void **)&env, &vm_args);
    if (code < 0)
    {
      fprintf(stderr, "couldn't create jvm with code %d on https://docs.oracle.com/en/java/javase/20/docs/specs/jni/functions.html#return-codes\n", code);
      throw "jvm failed";
    }
    if (!env)
    {
      fprintf(stderr, "null env in initialization\n");
      throw "jvm failed";
    }
    jclass bfbridge_local = env->FindClass("org/camicroscope/BFBridge");
    if (!bfbridge_local)
    {
      fprintf(stderr, "org.camicroscope.BFBridge (or a dependency of it) could not be found; is the jar in %s ?\n", options[0].optionString);
      if (env->ExceptionCheck() == 1)
      {
        fprintf(stderr, "exception\n");
        env->ExceptionDescribe();
      }
      else
        fprintf(stderr, "no exception\n");

      throw "org.camicroscope.BFBridge could not be found; is the jar in %s ?\n" + std::string(options[0].optionString);
    }
    delete[] options;

    bfbridge = (jclass)env->NewGlobalRef(bfbridge_local);
    fprintf(stderr, "bfbridge %p\n", bfbridge);
    //env->DeleteLocalRef(bfbridge_local);

    // Allow 2048*2048 four channels of 16 bits
    communication_buffer = new char[communication_buffer_len];
    jobject buffer = env->NewDirectByteBuffer(communication_buffer, communication_buffer_len);
    if (!buffer)
    {
      fprintf(stderr, "Couldn't allocate %d: too little RAM or JVM JNI doesn't support native memory access?", communication_buffer_len);
      throw "Allocation failed";
    }
    jmethodID bufferSetter = env->GetStaticMethodID(bfbridge, "BFSetCommunicationBuffer", "(Ljava/nio/ByteBuffer;)V");
    env->CallStaticVoidMethod(bfbridge, bufferSetter, buffer);
    env->DeleteLocalRef(buffer);
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
    jvm = other.jvm;
    env = other.env;
    communication_buffer = other.communication_buffer;
    other.jvm = NULL;
  }
  BioFormatsInstance &operator=(const BioFormatsInstance &) = delete;
  /*BioFormatsInstance &operator=(BioFormatsInstance &&) = default;*/
  // We cant use these two default ones because std::move still keeps
  // the previous object so it calls the destructor.
  // the previous one cannot call destructor now; set its jvm pointer to null
  // so that it can't break the new class.
  BioFormatsInstance &operator=(BioFormatsInstance &&other)
  {
    jvm = other.jvm;
    env = other.env;
    communication_buffer = other.communication_buffer;
    other.jvm = NULL;
    return *this;
  }

  ~BioFormatsInstance()
  {
    if (jvm)
    {
      // Not needed: destroy vm already
      // env->DeleteGlobalRef(bfbridge);
      // ...
      jvm->DestroyJavaVM();
      delete[] communication_buffer;
    }
  }

  // changed ownership
  void refresh()
  {
    fprintf(stderr, "calling refresh\n");
    jmethodID close = env->GetStaticMethodID(bfbridge, "BFClose", "()I");
    fprintf(stderr, "mid called refresh\n");
    env->CallStaticVoidMethod(bfbridge, close);
    fprintf(stderr, "called refresh\n");
  }

  std::string bf_get_error()
  {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetErrorLength = env->GetStaticMethodID(bfbridge, "BFGetErrorLength", "()I");
    int len = env->CallStaticIntMethod(bfbridge, BFGetErrorLength);
    std::string err;
    err.assign(communication_buffer, len);
    return err;
  }

  int bf_is_compatible(std::string filepath)
  {
    fprintf("calling is compatible\n");
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsCompatible = env->GetStaticMethodID(bfbridge, "BFIsCompatible", "(I)I");
    int len = filepath.length();
    memcpy(communication_buffer, filepath.c_str(), len);
    fprintf(stderr, "called is compatible\n");
    return env->CallStaticIntMethod(bfbridge, BFIsCompatible, len);
  }

  int bf_open(std::string filepath)
  {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");

    jmethodID BFOpen = env->GetStaticMethodID(bfbridge, "BFOpen", "(I)I");
    int len = filepath.length();
    memcpy(communication_buffer, filepath.c_str(), len);
    return env->CallStaticIntMethod(bfbridge, BFOpen, len);
  }

  int bf_close()
  {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");

    jmethodID BFClose = env->GetStaticMethodID(bfbridge, "BFClose", "()I");
    return env->CallStaticIntMethod(bfbridge, BFClose);
  }

  int bf_get_resolution_count()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetResolutionCount = env->GetStaticMethodID(bfbridge, "BFGetResolutionCount", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetResolutionCount);
  }

  int bf_set_current_resolution(int res)
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFSetResolutionCount = env->GetStaticMethodID(bfbridge, "BFSetResolutionCount", "(I)I");
    return env->CallStaticIntMethod(bfbridge, BFSetResolutionCount, res);
  }

  int bf_set_series(int ser)
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFSetSeries = env->GetStaticMethodID(bfbridge, "BFSetSeries", "(I)I");
    return env->CallStaticIntMethod(bfbridge, BFSetSeries, ser);
  }

  int bf_get_series_count()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetSeriesCount = env->GetStaticMethodID(bfbridge, "BFGetSeriesCount", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetSeriesCount);
  }

  int bf_get_size_x()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetSizeX = env->GetStaticMethodID(bfbridge, "BFGetSizeX", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetSizeX);
  }

  int bf_get_size_y()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetSizeY = env->GetStaticMethodID(bfbridge, "BFGetSizeY", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetSizeY);
  }

  int bf_get_size_z()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetSizeZ = env->GetStaticMethodID(bfbridge, "BFGetSizeZ", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetSizeZ);
  }

  int bf_get_size_c()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetSizeC = env->GetStaticMethodID(bfbridge, "BFGetSizeC", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetSizeC);
  }

  int bf_get_size_t()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetSizeT = env->GetStaticMethodID(bfbridge, "BFGetSizeT", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetSizeT);
  }

  int bf_get_effective_size_c()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetEffectiveSizeC = env->GetStaticMethodID(bfbridge, "BFGetEffectiveSizeC", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetEffectiveSizeC);
  }

  int bf_get_optimal_tile_width()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetOptimalTileWidth = env->GetStaticMethodID(bfbridge, "BFGetOptimalTileWidth", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetOptimalTileWidth);
  }

  int bf_get_optimal_tile_height()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetOptimalTileHeight = env->GetStaticMethodID(bfbridge, "BFGetOptimalTileHeight", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetOptimalTileHeight);
  }

  int bf_get_pixel_type()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetPixelType = env->GetStaticMethodID(bfbridge, "BFGetPixelType", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetPixelType);
  }

  int bf_get_bytes_per_pixel()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetBytesPerPixel = env->GetStaticMethodID(bfbridge, "BFGetBytesPerPixel", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetBytesPerPixel);
  }

  int bf_get_rgb_channel_count()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetRGBChannelCount = env->GetStaticMethodID(bfbridge, "BFGetRGBChannelCount", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetRGBChannelCount);
  }

  int bf_get_image_count()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetImageCount = env->GetStaticMethodID(bfbridge, "BFGetImageCount", "()I");
    return env->CallStaticIntMethod(bfbridge, BFGetImageCount);
  }

  int bf_is_rgb()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsRGB = env->GetStaticMethodID(bfbridge, "BFIsRGB", "()I");
    return env->CallStaticIntMethod(bfbridge, BFIsRGB);
  }

  int bf_is_interleaved()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsInterleaved = env->GetStaticMethodID(bfbridge, "BFIsInterleaved", "()I");
    return env->CallStaticIntMethod(bfbridge, BFIsInterleaved);
  }

  int bf_is_little_endian()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsLittleEndian = env->GetStaticMethodID(bfbridge, "BFIsLittleEndian", "()I");
    return env->CallStaticIntMethod(bfbridge, BFIsLittleEndian);
  }

  int bf_is_false_color()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsFalseColor = env->GetStaticMethodID(bfbridge, "BFIsFalseColor", "()I");
    return env->CallStaticIntMethod(bfbridge, BFIsFalseColor);
  }

  int bf_is_indexed_color()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsIndexedColor = env->GetStaticMethodID(bfbridge, "BFIsIndexedColor", "()I");
    return env->CallStaticIntMethod(bfbridge, BFIsIndexedColor);
  }

  // Returns char* to communication_buffer that will later be overwritten
  char *bf_get_dimension_order()
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFGetDimensionOrder = env->GetStaticMethodID(bfbridge, "BFGetDimensionOrder", "()I");
    int len = env->CallStaticIntMethod(bfbridge, BFGetDimensionOrder);
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
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFIsOrderCertain = env->GetStaticMethodID(bfbridge, "BFIsOrderCertain", "()I");
    return env->CallStaticIntMethod(bfbridge, BFIsOrderCertain);
  }

  int bf_open_bytes(int x, int y, int w, int h)
    {
    bfbridge = env->FindClass("org/camicroscope/BFBridge");
    jmethodID BFOpenBytes = env->GetStaticMethodID(bfbridge, "BFOpenBytes", "(IIII)I");
    return env->CallStaticIntMethod(bfbridge, BFOpenBytes, x, y, w, h);
  }
};

#endif /* BIOFORMATSINSTANCE_H */

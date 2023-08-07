// TODO: To be moved to decoders directory

/*
 * File:     bfbridge_basiclib.h
 * Also see: BioFormatsThread.h in github.com/camicroscope/iipImage
 */

#ifndef BFBRIDGE_BASICLIB_H
#define BFBRIDGE_BASICLIB_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BFBRIDGE_INLINE
#define BFBRIDGE_INLINE_ME static inline
#else
#define BFBRIDGE_INLINE_ME
#endif

typedef enum bfbridge_error_code
{
    BFBRIDGE_INVALID_CLASSPATH,
    BFBRIDGE_CLASS_NOT_FOUND,
    BFBRIDGE_METHOD_NOT_FOUND,
    // https://docs.oracle.com/en/java/javase/20/docs/specs/jni/functions.html#return-codes
    BFBRIDGE_JNI_ERR = JNI_ERR,
    BFBRIDGE_JNI_EDETACHED = JNI_EDETACHED,
    BFBRIDGE_JNI_EVERSION = JNI_EVERSION,
    BFBRIDGE_JNI_ENOMEM = JNI_EVERSION,
    BFBRIDGE_JNI_EEXIST = JNI_EEXIST,
    BFBRIDGE_JNI_EINVAL = JNI_EINVAL,

    // Instance initialization only:
    BFBRIDGE_INVALID_COMMUNICATON_BUFFER,
    BFBRIDGE_OUT_OF_MEMORY_ERROR,
    BFBRIDGE_JVM_LACKS_BYTE_BUFFERS,
} bfbridge_error_code_t;

typedef struct bfbridge_error
{
    bfbridge_error_code_t code;
    char *description;
} bfbridge_error_t;

BFBRIDGE_INLINE_ME void bfbridge_free_error(bfbridge_error_t *);

typedef struct bfbridge_library
{
    JavaVM *jvm;
    JNIEnv *env;

    // Calling FindClass later invalidates this pointer,
    // so please use this jclass variable instead of FindClass.
    jclass bfbridge_base;

    jmethodID constructor;

    jmethodID BFSetCommunicationBuffer;
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
    jmethodID BFGetOptimalTileHeight;
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
    jmethodID BFGetMPPX;
    jmethodID BFGetMPPY;
    jmethodID BFGetMPPZ;
    jmethodID BFIsAnyFileOpen;
    jmethodID BFToolsShouldGenerate;
    jmethodID BFToolsGenerateSubresolutions;
} bfbridge_library_t;

// On success, returns NULL and sets dest
// On failure, sets dest to NULL and returns error
// cpdir: a string to a single directory containing jar files (and maybe classes)
// cachedir: NULL or the directory path to store file caches for faster opening
BFBRIDGE_INLINE_ME bfbridge_error_t *bfbridge_make_library(
    bfbridge_library_t **dest,
    char *cpdir,
    char *cachedir);

BFBRIDGE_INLINE_ME void bfbridge_free_library(bfbridge_library_t *);

typedef struct bfbridge_instance
{
    bfbridge_library_t *library;
    jobject bfbridge;
    char *communication_buffer;
    int communication_buffer_len;
} bfbridge_instance_t;

// On success, returns NULL and sets dest
// On failure, sets dest to NULL and returns error
// library: the library that was allocated for this thread with bfbridge_make_library
// (you may otherwise DetachCurrentThread then AttachCurrentThread)
// Please allocate a char* from heap and pass it and its length.
// You should keep the pointer (or call bfbridge_instance_get_communication_buffer)
// and free it about the same time as bfbridge_free_instance or reuse it.
// communication_buffer:
// This will be used for two-way communication.
// Suggested length is 33 MB (33554432) to allow
// 2048 by 2048 four channels of 16 bit
BFBRIDGE_INLINE_ME bfbridge_error_t *bfbridge_make_instance(
    bfbridge_instance_t **dest,
    bfbridge_library_t *library,
    char *communication_buffer,
    int communication_buffer_len);

// bfbridge_free_instance mustnt't be called after bfbridge_free_library
// bfbridge_free_library removes the need to call bfbridge_free_instance.
// This function does not free communication_buffer, you should free it
BFBRIDGE_INLINE_ME void bfbridge_free_instance(bfbridge_instance_t *);

// returns the communication buffer
// sets *len if len is not NULL to the buffer length
// Usecases:
// - call next to bfbridge_free_instance to free the buffer
// - some of our library functions would like to return a byte array
// so they return the number bytes to be read, an integer, and
// expect that the user will read from there.
BFBRIDGE_INLINE_ME char *bfbridge_instance_get_communication_buffer(bfbridge_instance_t *, int *len);

/*
How is the communication buffer used internally?
Some functions receive from BioFormats through the communication buffer
Some functions communicate to BioFormats (such as passing a filename) through it
Some functions don't use the buffer at all
bf_open_bytes, for example, that reads a region to bytes,
receives bytes to the communication buffer. Returns an int,
bytes to be read, which then the user of the library should read from the buffer.
 */

#if defined(BFBRIDGE_INLINE)
#define BFBRIDGE_HEADER
#include "bfbridge_basiclib.c"
#undef BFBRIDGE_HEADER
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BFBRIDGE_BASICLIB_H

// TODO: To be moved to decoders directory

/*
 * File:     bfbridge_basiclib.h
 * Also see: BioFormatsThread.h in github.com/camicroscope/iipImage
 */

// Optionally define: BFBRIDGE_INLINE (makes it a header-only library)
// Please note: in this case you're encouraged to call bfbridge_make_library
// and bfbridge_make_library from a single compilation unit only to save from
// final executable size, or wrap it around a non-static caller which
// you can call from multiple files
// Please note: The best way to define BFBRIDGE_INLINE is through the flag
// -DBFBRIDGE_INLINE but it also works speed-wise to #define BFBRIDGE_INLINE
// before you include it.

// Optionally define: BFBRIDGE_KNOW_BUFFER_LEN
// This will save some memory by not storing buffer length in our structs.

#ifndef BFBRIDGE_BASICLIB_H
#define BFBRIDGE_BASICLIB_H

#include <jni.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef BFBRIDGE_INLINE
#define BFBRIDGE_INLINE_ME_EXTRA static
#else
#define BFBRIDGE_INLINE_ME_EXTRA
#endif

// As an example, inlining solves the issue that
// passing struct ptrs requires dereference https://stackoverflow.com/a/552250
// so without inline, we would need to pass instance pointers by value
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
    BFBRIDGE_LIBRARY_UNINITIALIZED,
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

    // Please keep this list in order with javap output
    // See the comment "To print descriptors (encoded function types) ..."
    // for the javap command
    jmethodID BFSetCommunicationBuffer;
    jmethodID BFGetErrorLength;
    jmethodID BFIsCompatible;
    jmethodID BFOpen;
    jmethodID BFIsSingleFile;
    jmethodID BFGetUsedFiles;
    jmethodID BFGetCurrentFile;
    jmethodID BFClose;
    jmethodID BFGetResolutionCount;
    jmethodID BFSetCurrentResolution;
    jmethodID BFGetSeriesCount;
    jmethodID BFSetCurrentSeries;
    jmethodID BFGetSizeX;
    jmethodID BFGetSizeY;
    jmethodID BFGetSizeC;
    jmethodID BFGetSizeZ;
    jmethodID BFGetSizeT;
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

// On success, returns NULL and fills *dest
// On failure, returns error, and it may have modified *dest
// cpdir: a string to a single directory containing jar files (and maybe classes)
// cachedir: NULL or the directory path to store file caches for faster opening
BFBRIDGE_INLINE_ME_EXTRA bfbridge_error_t *bfbridge_make_library(
    bfbridge_library_t *dest,
    char *cpdir,
    char *cachedir);

// Copies while making the freeing of the previous a noop
// Dest: doesn't need to be initialized but allocated
// This function would benefit from restrict but if we inline, not necessary
BFBRIDGE_INLINE_ME void bfbridge_move_library(
    bfbridge_library_t *dest, bfbridge_library_t *lib);

// Does not free the library struct but its contents
BFBRIDGE_INLINE_ME void bfbridge_free_library(bfbridge_library_t *);

// Almost all functions that need a bfbridge_instance_t must be passed
// the instance's related bfbridge_library_t and not any other bfbridge_library_t

typedef struct bfbridge_instance
{
    jobject bfbridge;
    char *communication_buffer;
#ifndef BFBRIDGE_KNOW_BUFFER_LEN
    int communication_buffer_len;
#endif
} bfbridge_instance_t;

// On success, returns NULL and fills *dest
// On failure, returns error, and it may have modified *dest
// (But on failure, it will still have set the communication_buffer
// and the communication_buffer_len, to help you free it)
// library: the library that was allocated for this thread with bfbridge_make_library
// (you may otherwise DetachCurrentThread then AttachCurrentThread)
// communication_buffer: Please allocate a char* from heap and pass it and its length.
// You should keep the pointer (or call bfbridge_instance_get_communication_buffer)
// and free it about the same time as bfbridge_free_instance or reuse it.
// communication_buffer:
// This will be used for two-way communication.
// Suggested length is 33 MB (33554432) to allow
// 2048 by 2048 four channels of 16 bit
// communication_buffer_len: You should pass this correctly
// even if you define BFBRIDGE_KNOW_BUFFER_LEN
BFBRIDGE_INLINE_ME_EXTRA bfbridge_error_t *bfbridge_make_instance(
    bfbridge_instance_t *dest,
    bfbridge_library_t *library,
    char *communication_buffer,
    int communication_buffer_len);

// Copies while making the freeing of the previous a noop
// Dest: doesn't need to be initialized but allocated
// This function would benefit from restrict but if we inline, not necessary
BFBRIDGE_INLINE_ME void bfbridge_move_instance(
    bfbridge_instance_t *dest, bfbridge_instance_t *lib);

// bfbridge_free_instance mustnt't be called after bfbridge_free_library
// bfbridge_free_library removes the need to call bfbridge_free_instance.
// This function does not free communication_buffer, you should free it
// It also does not free the instance struct but its contents only.
BFBRIDGE_INLINE_ME void bfbridge_free_instance(
    bfbridge_instance_t *, bfbridge_library_t *library);

// returns the communication buffer
// sets *len, if the len ptr is not NULL, to the buffer length
// Usecases:
// - call next to bfbridge_free_instance to free the buffer
// - some of our library functions would like to return a byte array
// so they return the number bytes to be read, an integer, and
// expect that the user will read from there.
BFBRIDGE_INLINE_ME char *bfbridge_instance_get_communication_buffer(bfbridge_instance_t *, int *len);

// Return a C string with the last error
// This should only be called when the last bf_* method returned an error code
// May otherwise return undisplayable characters
BFBRIDGE_INLINE_ME char *bf_get_error_convenience(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

// bf_get_error_length: fills the communication buffer with an error message
// returns: the number of bytes to read
BFBRIDGE_INLINE_ME int bf_get_error_length(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

// 1 if the filepath can be read by BioFormats otherwise 0
BFBRIDGE_INLINE_ME int bf_is_compatible(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    char *filepath, int filepath_len);

BFBRIDGE_INLINE_ME int bf_open(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    char *filepath, int filepath_len);

BFBRIDGE_INLINE_ME int bf_is_single_file(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    char *filepath, int filepath_len);

BFBRIDGE_INLINE_ME int bf_get_used_files(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_current_file(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_close(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_resolution_count(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_set_current_resolution(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    int res);

BFBRIDGE_INLINE_ME int bf_set_current_series(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    int ser);

BFBRIDGE_INLINE_ME int bf_get_size_x(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_size_y(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_size_c(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_size_z(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_size_t(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_effective_size_c(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_effective_size_c(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_optimal_tile_width(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_optimal_tile_height(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_format(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_pixel_type(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_bits_per_pixel(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_bytes_per_pixel(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_rgb_channel_count(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_image_count(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_is_rgb(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_is_interleaved(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_is_little_endian(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_is_false_color(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_is_indexed_color(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_get_dimension_order(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_is_order_certain(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_open_bytes(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    int x, int y, int w, int h);

BFBRIDGE_INLINE_ME double bf_get_mpp_x(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME double bf_get_mpp_y(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME double bf_get_mpp_z(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_is_any_file_open(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

BFBRIDGE_INLINE_ME int bf_tools_should_generate(
    bfbridge_instance_t *instance, bfbridge_library_t *library);

/*
How is the communication buffer used internally?
Some functions receive from BioFormats through the communication buffer
Some functions communicate to BioFormats (such as passing a filename) through it
Some functions don't use the buffer at all
bf_open_bytes, for example, that reads a region to bytes,
receives bytes to the communication buffer. Returns an int,
bytes to be read, which then the user of the library should read from the buffer.
 */

#ifdef BFBRIDGE_INLINE
#define BFBRIDGE_HEADER
#include "bfbridge_basiclib.c"
#undef BFBRIDGE_HEADER
#endif

#ifdef __cplusplus
} // extern "C"
#endif

#endif // BFBRIDGE_BASICLIB_H

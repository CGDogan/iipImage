// TODO: To be moved to decoders directory

// TODO: Add example use. Mention that
/*bfbridge_make_library will either return success
or won't require any free, except for the user-allocated struct

make_instance will either return success or fail while setting
communication buffer so one can free that easily and
allow you to call free_instance even if unsuccessful (write under heading
maybe: "failure guarantees")

Ease of freeing:
Free can be called for both the library and the instance
because the makers set to null if going to fail
so you can call free regardless of whether the makers failed.

But you can make instance if library call failed
*/

// If inlining but erroneously still compiling .c, make it empty
#if !(defined(BFBRIDGE_INLINE) && !defined(BFBRIDGE_HEADER))

#include "bfbridge_basiclib.h"
#include <stdio.h>
#include <string.h>

#ifdef WIN32
#include <windows.h>
#define BFBRIDGE_JNI_PATH_SEPARATOR_STR ";"
#define BFBRIDGE_PATH_SEPARATOR '\\'
#else
#include <dirent.h>
#define BFBRIDGE_JNI_PATH_SEPARATOR_STR ":"
#define BFBRIDGE_PATH_SEPARATOR '/'
#endif

// Define BFENVA - BF ENV Access
// In a single-header mode of operation, we need to be able to
// call JNI in its both modes
// C++: env->JNIMethodABC..(x,y,z)
// C: (*env)->JNIMethodABC..(env, x,y,z)
// This can also be used for JVM*, such as (*jvm)->DestroyJavaVM(jvm);
// Define a second one (ENV Access Void), BFENVAV, for no args,
// as __VA_ARGS__ requires at least one
#ifdef __cplusplus
#define BFENVA(env_ptr, method_name, ...) \
    ((env_ptr)->method_name(__VA_ARGS__))
#define BFENVAV(env_ptr, method_name) \
    ((env_ptr)->method_name())
#else
#define BFENVA(env_ptr, method_name, ...) \
    ((*(env_ptr))->method_name((env_ptr), __VA_ARGS__))
#define BFENVAV(env_ptr, method_name, ...) \
    ((*(env_ptr))->method_name((env_ptr)))
#endif

void bfbridge_free_error(bfbridge_error_t *error)
{
    free(error->description);
    free(error);
}

typedef struct bfbridge_basiclib_string
{
    char *str;
    int len;
    int alloc_len;
} bfbridge_basiclib_string_t;

static bfbridge_basiclib_string_t *allocate_string(const char *initial)
{
    int initial_len = strlen(initial);
    bfbridge_basiclib_string_t *bbs =
        (bfbridge_basiclib_string_t *)
            malloc(sizeof(bfbridge_basiclib_string_t));
    bbs->alloc_len = 2 * initial_len;
    bbs->str = (char *)malloc(bbs->alloc_len * sizeof(char) + 1);
    bbs->len = initial_len;
    strcpy(bbs->str, initial);
    return bbs;
}

static void free_string(bfbridge_basiclib_string *bbs)
{
    free(bbs->str);
    free(bbs);
}

static void append_to_string(bfbridge_basiclib_string_t *bbs, const char *s)
{
    int s_len = strlen(s);
    int required_len = s_len + bbs->len;
    if (bbs->alloc_len < required_len)
    {
        do
        {
            bbs->alloc_len = bbs->alloc_len + bbs->alloc_len / 2;
        } while ((bbs->alloc_len < required_len));
        bbs->str = (char *)realloc(bbs->str, bbs->alloc_len + 1);
    }
    strcat(bbs->str, s);
    bbs->len += s_len;
}

// description: optional string to be appended to end of operation
static bfbridge_error_t *make_error(
    bfbridge_error_code_t code,
    const char *operation,
    const char *description)
{
    bfbridge_error_t *error = (bfbridge_error_t *)malloc(sizeof(bfbridge_error_t));
    error->code = code;
    bfbridge_basiclib_string_t *desc = allocate_string(operation);
    if (description)
    {
        append_to_string(desc, description);
    }
    error->description = desc->str;
    return error;
}

// JVM restriction: Allows only one JVM alive per thread
bfbridge_error_t *bfbridge_make_library(
    bfbridge_library_t *dest, char *cpdir, char *cachedir)
{
    // Ease of freeing
    dest->jvm = NULL;

    if (cpdir == NULL || cpdir[0] == '\0')
    {
        return make_error(BFBRIDGE_INVALID_CLASSPATH, "bfbridge_make_library: no classpath supplied", NULL);
    }

    // Plus one if needed for the purpose below, and one for nullchar
    int cp_len = strlen(cpdir);
    char cp[cp_len + 1 + 1];
    strcpy(cp, cpdir);
    // To be able to append filenames to the end of path,
    // it needs the optional "/" at the end
    if (cp[cp_len - 1] != BFBRIDGE_PATH_SEPARATOR)
    {
        cp[cp_len++] = BFBRIDGE_PATH_SEPARATOR;
        cp[cp_len] = 0;
    }

    // Should be freed: path_arg
    bfbridge_basiclib_string_t *path_arg = allocate_string("-Djava.class.path=");

    append_to_string(path_arg, cp);

    // Also add .../*, ending with asterisk, just in case
    append_to_string(path_arg, BFBRIDGE_JNI_PATH_SEPARATOR_STR);
    append_to_string(path_arg, cp);
    append_to_string(path_arg, "*");

// But for some reason unlike the -cp arg, .../* does not work
// so we need to list every jar file
// https://en.cppreference.com/w/cpp/filesystem/directory_iterator
#ifdef WIN32
#error "directory file listing not implemented on Windows"
#else
    DIR *cp_dir = opendir(cp);
    if (!cp_dir)
    {
        free_string(path_arg);
        return make_error(BFBRIDGE_INVALID_CLASSPATH, "bfbridge_make_library: a single classpath folder containing jars was expected but got ", cp);
    }
    struct dirent *cp_dirent;
    while ((cp_dirent = readdir(cp_dir)) != NULL)
    {
        append_to_string(path_arg, BFBRIDGE_JNI_PATH_SEPARATOR_STR);
        append_to_string(path_arg, cp);
        append_to_string(path_arg, cp_dirent->d_name);
    }
    closedir(cp_dir);
#endif

    JavaVMOption options[3];

    fprintf(stderr, "Java classpath (BFBRIDGE_CLASSPATH): %s\n", path_arg->str);
    // https://docs.oracle.com/en/java/javase/20/docs/specs/man/java.html#performance-tuning-examples
    char optimize1[] = "-XX:+UseParallelGC";
    // char optimize2[] = "-XX:+UseLargePages"; Not compatible with our linux distro
    options[0].optionString = path_arg->str;
    options[1].optionString = optimize1;
    // options[2].optionString = "-verbose:jni";
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_20;
    vm_args.nOptions = 2;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = 0;

    // Should be freed: path_arg, cache_arg
    bfbridge_basiclib_string_t *cache_arg = allocate_string("-Dbfbridge.cachedir=");

    fprintf(stderr, "passingbfbridge cache dir: %s\n", cachedir);
    if (cachedir && cachedir[0] != '\0')
    {
        // Remember that pointers we use must be valid until JNI_CreateJavaVM.
        append_to_string(cache_arg, cachedir);
        options[vm_args.nOptions++].optionString = cache_arg->str;
    }

    JavaVM *jvm;
    JNIEnv *env;

    // Should be freed: path_arg, cache_arg, jvm
    int code = JNI_CreateJavaVM(&jvm, (void **)&env, &vm_args);
    free_string(cache_arg);

    // Should be freed: path_arg, jvm

    if (code < 0)
    {
        free_string(path_arg);
        return make_error((bfbridge_error_code_t)code, "JNI_CreateJavaVM failed, please see https://docs.oracle.com/en/java/javase/20/docs/specs/jni/functions.html#return-codes for error code description", NULL);
    }

    jclass bfbridge_base = BFENVA(env, FindClass, "org/camicroscope/BFBridge");
    if (!bfbridge_base)
    {
        bfbridge_basiclib_string_t *error = allocate_string("FindClass failed because org.camicroscope.BFBridge (or a dependency of it) could not be found. Are the jars in: ");
        append_to_string(error, path_arg->str);

        if (BFENVA(env, ExceptionCheck) == 1)
        {
            BFENVA(env, ExceptionDescribe);
            append_to_string(error, "? An exception was printed to stderr.");
        }

        free_string(path_arg);
        BFENVA(jvm, DestroyJavaVM);

        bfbridge_error_t *err = make_error(BFBRIDGE_CLASS_NOT_FOUND, error->str, NULL);
        free_string(error);
        return err;
    }

    free_string(path_arg);

    dest->env = env;
    dest->bfbridge_base = bfbridge_base;

    dest->constructor = BFENVA(env, GetMethodID, bfbridge_base, "<init>", "()V");
    if (!dest->constructor)
    {
        BFENVA(jvm, DestroyJavaVM);
        return make_error(BFBRIDGE_METHOD_NOT_FOUND, "Could not find BFBridge constructor", NULL);
    }

    // Now do the same for methods but in shorthand form
#define prepare_method_id(name, descriptor)                         \
    dest->name =                                                    \
        BFENVA(env, GetMethodID, bfbridge_base, #name, descriptor); \
    if (!dest->name)                                                \
    {                                                               \
        BFENVA(jvm, DestroyJavaVM);                                 \
        return make_error(                                          \
            BFBRIDGE_METHOD_NOT_FOUND,                              \
            "Could not find BFBridge method ",                      \
            #name ". Maybe check and update the descriptor?");      \
    }

    // To print descriptors (encoded function types) to screen
    // ensure that curent dir's org/camicroscope folder has BFBridge.class
    // otherwise, compile in the parent folder of "org" with:
    // "javac -cp ".:jar_files/*" org/camicroscope/BFBridge.java".
    // Run: javap -s (-p) org.camicroscope.BFBridge

    prepare_method_id(BFSetCommunicationBuffer, "(Ljava/nio/ByteBuffer;)V");
    prepare_method_id(BFGetErrorLength, "()I");
    prepare_method_id(BFIsCompatible, "(I)I");
    prepare_method_id(BFIsAnyFileOpen, "()I");
    prepare_method_id(BFOpen, "(I)I");
    prepare_method_id(BFGetFormat, "()I");
    prepare_method_id(BFIsSingleFile, "(I)I");
    prepare_method_id(BFGetCurrentFile, "()I");
    prepare_method_id(BFGetUsedFiles, "()I");
    prepare_method_id(BFClose, "()I");
    prepare_method_id(BFGetSeriesCount, "()I");
    prepare_method_id(BFSetCurrentSeries, "(I)I");
    prepare_method_id(BFGetResolutionCount, "()I");
    prepare_method_id(BFSetCurrentResolution, "(I)I");
    prepare_method_id(BFGetSizeX, "()I");
    prepare_method_id(BFGetSizeY, "()I");
    prepare_method_id(BFGetSizeC, "()I");
    prepare_method_id(BFGetSizeZ, "()I");
    prepare_method_id(BFGetSizeT, "()I");
    prepare_method_id(BFGetEffectiveSizeC, "()I");
    prepare_method_id(BFGetImageCount, "()I");
    prepare_method_id(BFGetDimensionOrder, "()I");
    prepare_method_id(BFIsOrderCertain, "()I");
    prepare_method_id(BFGetOptimalTileWidth, "()I");
    prepare_method_id(BFGetOptimalTileHeight, "()I");
    prepare_method_id(BFGetPixelType, "()I");
    prepare_method_id(BFGetBitsPerPixel, "()I");
    prepare_method_id(BFGetBytesPerPixel, "()I");
    prepare_method_id(BFGetRGBChannelCount, "()I");
    prepare_method_id(BFIsRGB, "()I");
    prepare_method_id(BFIsInterleaved, "()I");
    prepare_method_id(BFIsLittleEndian, "()I");
    prepare_method_id(BFIsIndexedColor, "()I");
    prepare_method_id(BFIsFalseColor, "()I");
    prepare_method_id(BFGet8BitLookupTable, "()I");
    prepare_method_id(BFGet16BitLookupTable, "()I");
    prepare_method_id(BFOpenBytes, "(IIIII)I");
    prepare_method_id(BFGetMPPX, "(I)D");
    prepare_method_id(BFGetMPPY, "(I)D");
    prepare_method_id(BFGetMPPZ, "(I)D");
    prepare_method_id(BFToolsShouldGenerate, "()I");
    prepare_method_id(BFToolsGenerateSubresolutions, "(III)I");

    // Ease of freeing: keep null until we can return without error
    dest->jvm = jvm;

    return NULL;
}

void bfbridge_move_library(bfbridge_library_t *dest, bfbridge_library_t *lib)
{
    // Ease of freeing
    if (lib->jvm)
    {
        *dest = *lib;
        lib->jvm = NULL;
    }
    else
    {
        dest->jvm = NULL;
    }
}

void bfbridge_free_library(bfbridge_library_t *lib)
{
    // Ease of freeing
    if (lib->jvm)
    {
        BFENVA(lib->jvm, DestroyJavaVM);
    }
    // Now, after DestroyJavaVM, there's no need to free bfbridge_base
    // DetachCurrentThread would also free this reference
}

bfbridge_error_t *bfbridge_make_instance(
    bfbridge_instance_t *dest,
    bfbridge_library_t *library,
    char *communication_buffer,
    int communication_buffer_len)
{
    // Ease of freeing
    dest->bfbridge = NULL;
    dest->communication_buffer = communication_buffer;
#ifndef BFBRIDGE_KNOW_BUFFER_LEN
    dest->communication_buffer_len = communication_buffer_len;
#endif

    if (!library->jvm)
    {
        return make_error(
            BFBRIDGE_LIBRARY_UNINITIALIZED,
            "a bfbridge_library_t must have been initialized before bfbridge_make_instance",
            NULL);
    }

    if (communication_buffer == NULL || communication_buffer_len < 0)
    {
        return make_error(
            BFBRIDGE_INVALID_COMMUNICATON_BUFFER,
            "communication_buffer NULL or has negative length",
            NULL);
    }

    JNIEnv *env = library->env;

    jobject bfbridge_local =
        BFENVA(env, NewObject, library->bfbridge_base, library->constructor);
    // Should be freed: bfbridge
    jobject bfbridge = (jobject)BFENVA(env, NewGlobalRef, bfbridge_local);
    BFENVA(env, DeleteLocalRef, bfbridge_local);

    // Should be freed: bfbridge, buffer (the jobject only)
    jobject buffer =
        BFENVA(env,
               NewDirectByteBuffer,
               communication_buffer,
               communication_buffer_len);
    if (!buffer)
    {
        if (BFENVA(env, ExceptionCheck) == 1)
        {
            // As of JDK 20, NewDirectByteBuffer only raises OutOfMemoryError
            BFENVA(env, ExceptionDescribe);
            BFENVA(env, DeleteGlobalRef, bfbridge);

            return make_error(
                BFBRIDGE_OUT_OF_MEMORY_ERROR,
                "NewDirectByteBuffer failed, printing debug info to stderr",
                NULL);
        }
        else
        {
            BFENVA(env, DeleteGlobalRef, bfbridge);
            return make_error(
                BFBRIDGE_JVM_LACKS_BYTE_BUFFERS,
                "Used JVM implementation does not support direct byte buffers"
                "which means that communication between Java and our library"
                "would need to copy data inefficiently, but only"
                "the direct byte buffer mode is supported by our library",
                // To implement this, one can save, in a boolean flag
                // in this library, that a copy required, then
                // make a function in Java that makes new java.nio.ByteBuffer
                // and sets the buffer variable to it,
                // then use getByteArrayRegion and setByteArrayRegion from
                // this library to copy when needed to interact with buffer.
                NULL);
        }
    }

    /*
    How we would do if we hadn't been caching methods beforehand: This way:
    jmethodID BFSetCommunicationBuffer =
      BFENVA(env, GetMethodID,
      library->bfbridge_base, "BFSetCommunicationBuffer",
      "(Ljava/nio/ByteBuffer;)V");
    );
    BFENVA(env, CallVoidMethod, bfbridge, library->BFSetCommunicationBuffer, buffer);
    */
    BFENVA(env, CallVoidMethod, bfbridge, library->BFSetCommunicationBuffer, buffer);
    BFENVA(env, DeleteLocalRef, buffer);

    // Ease of freeing: keep null until we can return without error
    dest->bfbridge = bfbridge;
    return NULL;
}

// Copies while making the freeing of the previous a noop
// Dest: doesn't need to be initialized but allocated
// This function would benefit from restrict but if we inline, not necessary
void bfbridge_move_instance(bfbridge_instance_t *dest, bfbridge_instance_t *lib)
{
    // Ease of freeing
    if (lib->bfbridge)
    {
        *dest = *lib;
        lib->bfbridge = NULL;
        lib->communication_buffer = NULL;
    }
    else
    {
        dest->bfbridge = NULL;
        dest->communication_buffer = NULL;
    }
}

void bfbridge_free_instance(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    // Ease of freeing
    if (instance->bfbridge)
    {
        BFENVA(library->env, DeleteGlobalRef, instance->bfbridge);
    }
}

char *bfbridge_instance_get_communication_buffer(
    bfbridge_instance_t *instance, int *len)
{
#ifndef BFBRIDGE_KNOW_BUFFER_LEN
    if (len)
    {
        *len = instance->communication_buffer_len;
    }
#endif
    return instance->communication_buffer;
}

// Shorthand for JavaENV:
#define BFENV (library->env)
// Instance class:
#define BFINSTC (instance->bfbridge)

// Call easily
// #define BFFUNC(method_name, ...) BFENVA(BFENV, method_name, BFINSTC, __VA_ARGS__)
// Even more easily:
//      #define BFFUNC(caller, method, ...) \
//BFENVA(BFENV, caller, BFINSTC, library->method, __VA_ARGS__)
// Super easily:
#define BFFUNC(method, type, ...) \
    BFENVA(BFENV, Call##type##Method, BFINSTC, library->method, __VA_ARGS__)
// Use the second one, void one, for no args as __VA_ARGS__ requires at least one
#define BFFUNCV(method, type) \
    BFENVA(BFENV, Call##type##Method, BFINSTC, library->method)

// Methods
// Please keep in order with bfbridge_library_t members

// BFSetCommunicationBuffer is used internally

char *bf_get_error_convenience(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    int len = BFFUNCV(BFGetErrorLength, Int);
    // The case of overflow is handled on Java side
    instance->communication_buffer[len] = '\0';
    return instance->communication_buffer;
}

int bf_get_error_length(bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetErrorLength, Int);
}

int bf_is_compatible(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    char *filepath, int filepath_len)
{
    fprintf(stderr, "calling is compatible %s\n", filepath);
    memcpy(instance->communication_buffer, filepath, filepath_len);
    fprintf(stderr, "called is compatible\n");
    return BFFUNC(BFIsCompatible, Int, filepath_len);
}

int bf_is_any_file_open(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFIsAnyFileOpen, Int);
}

int bf_open(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    char *filepath, int filepath_len)
{
    memcpy(instance->communication_buffer, filepath, filepath_len);
    return BFFUNC(BFOpen, Int, filepath_len);
}

int bf_get_format(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetFormat, Int);
}

int bf_is_single_file(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    char *filepath, int filepath_len)
{
    memcpy(instance->communication_buffer, filepath, filepath_len);
    return BFFUNC(BFIsSingleFile, Int, filepath_len);
}

int bf_get_current_file(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetCurrentFile, Int);
}

// Lists null-separated filenames/filepaths for the currently open file.
// Returns bytes written including the last null
int bf_get_used_files(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetUsedFiles, Int);
}

int bf_close(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFClose, Int);
}

int bf_get_series_count(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetSeriesCount, Int);
}

int bf_set_current_series(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    int ser)
{
    return BFFUNC(BFSetCurrentSeries, Int, ser);
}

int bf_get_resolution_count(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetResolutionCount, Int);
}

int bf_set_current_resolution(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    int res)
{
    return BFFUNC(BFSetCurrentResolution, Int, res);
}

int bf_get_size_x(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetSizeX, Int);
}

int bf_get_size_y(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetSizeY, Int);
}

int bf_get_size_c(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetSizeC, Int);
}

int bf_get_size_z(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetSizeZ, Int);
}

int bf_get_size_t(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetSizeT, Int);
}

int bf_get_effective_size_c(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetEffectiveSizeC, Int);
}

int bf_get_image_count(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetImageCount, Int);
}

int bf_get_dimension_order(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetDimensionOrder, Int);
}

int bf_is_order_certain(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFIsOrderCertain, Int);
}

int bf_get_optimal_tile_width(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetOptimalTileWidth, Int);
}

int bf_get_optimal_tile_height(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetOptimalTileHeight, Int);
}

int bf_get_pixel_type(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetPixelType, Int);
}

int bf_get_bits_per_pixel(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetBitsPerPixel, Int);
}

int bf_get_bytes_per_pixel(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetBytesPerPixel, Int);
}

int bf_get_rgb_channel_count(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGetRGBChannelCount, Int);
}

int bf_is_rgb(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFIsRGB, Int);
}

int bf_is_interleaved(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFIsInterleaved, Int);
}

int bf_is_little_endian(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFIsLittleEndian, Int);
}

int bf_is_indexed_color(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFIsIndexedColor, Int);
}

int bf_is_false_color(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFIsFalseColor, Int);
}

int bf_get_8_bit_lookup_table(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGet8BitLookupTable, Int);
}

int bf_get_16_bit_lookup_table(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFGet16BitLookupTable, Int);
}

int bf_open_bytes(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    int plane, int x, int y, int w, int h)
{
    return BFFUNC(BFOpenBytes, Int, plane, x, y, w, h);
}

double bf_get_mpp_x(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    int series)
{
    return BFFUNC(BFGetMPPX, Double, series);
}

double bf_get_mpp_y(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    int series)
{
    return BFFUNC(BFGetMPPY, Double, series);
}

double bf_get_mpp_z(
    bfbridge_instance_t *instance, bfbridge_library_t *library,
    int series)
{
    return BFFUNC(BFGetMPPZ, Double, series);
}

int bf_tools_should_generate(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFFUNCV(BFToolsShouldGenerate, Int);
}

#undef BFENVA
#undef BFENV
#undef BFINSTC

#endif // !(defined(BFBRIDGE_INLINE) && !defined(BFBRIDGE_HEADER))
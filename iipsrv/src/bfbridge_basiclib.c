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
so you can call free regardless of whether the makers failed
*/

// If inlining but erroneously still compiling .c, make it empty
#if !(defined(BFBRIDGE_INLINE) && !defined(BFBRIDGE_HEADER))

#include "bfbridge_basiclib.h"
#include <stdio.h>

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
#ifdef __cplusplus
#define BFENVA(env, method_name, ...) (env)->method_name(__VA_ARGS__)
#else
#define BFENVA(env, method_name, ...) (*(env))->method_name((env), __VA_ARGS__)
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
    dest->env = NULL;

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
        return make_error((bfbridge_error_code_t) code, "JNI_CreateJavaVM failed, please see https://docs.oracle.com/en/java/javase/20/docs/specs/jni/functions.html#return-codes for error code description", NULL);
    }

    jclass bfbridge_base = (*env)->FindClass(env, "org/camicroscope/BFBridge");
    if (!bfbridge_base)
    {
        bfbridge_basiclib_string_t *error = allocate_string("FindClass failed because org.camicroscope.BFBridge (or a dependency of it) could not be found. Are the jars in: ");
        append_to_string(error, path_arg);

        if ((*env)->ExceptionCheck(env) == 1)
        {
            (*env)->ExceptionDescribe(env);
            append_to_string(error, "? An exception was printed to stderr.");
        }

        free_string(path_arg);
        jvm->DestroyJavaVM();

        bfbridge_error_t *err = make_error(BFBRIDGE_CLASS_NOT_FOUND, error->str, NULL);
        free_string(error);
        return err;
    }

    free_string(path_arg);

    dest->jvm = jvm;
    dest->env = env;
    dest->bfbridge_base = bfbridge_base;

    dest->constructor = (*env)->GetMethodID(env, bfbridge_base, "<init>", "()V");
    if (!dest->constructor)
    {
        jvm->DestroyJavaVM();
        return make_error(BFBRIDGE_METHOD_NOT_FOUND, "Could not find BFBridge constructor", NULL);
    }

    // Now do the same for methods but in shorthand form
#define prepare_method_id(name, descriptor)                         \
    dest->name =                                                    \
        (*env)->GetMethodID(env, bfbridge_base, #name, descriptor); \
    if (!dest->name)                                                \
    {                                                               \
        jvm->DestroyJavaVM();                                       \
        return make_error(                                          \
            BFBRIDGE_METHOD_NOT_FOUND,                              \
            "Could not find BFBridge method ",                      \
            #name " with expected descriptor " decriptor);          \
    }

    // To print descriptors (encoded function types) to screen
    // ensure that curent dir's org/camicroscope folder has BFBridge.class
    // otherwise, compile in the parent folder of "org" with:
    // "javac -cp ".:jar_files/*" org/camicroscope/BFBridge.java".
    // Run: javap -s (-p) org.camicroscope.BFBridge

    prepare_method_id(BFSetCommunicationBuffer, "(Ljava/nio/ByteBuffer;)V");
    prepare_method_id(BFGetErrorLength, "()I");
    prepare_method_id(BFIsCompatible, "(I)I");
    prepare_method_id(BFOpen, "(I)I");
    prepare_method_id(BFIsSingleFile, "(I)I");
    prepare_method_id(BFGetUsedFiles, "()I");
    prepare_method_id(BFGetCurrentFile, "()I");
    prepare_method_id(BFClose, "()I");
    prepare_method_id(BFGetResolutionCount, "()I");
    prepare_method_id(BFSetCurrentResolution, "(I)I");
    prepare_method_id(BFSetSeries, "(I)I");
    prepare_method_id(BFGetSeriesCount, "()I");
    prepare_method_id(BFGetSizeX, "()I");
    prepare_method_id(BFGetSizeY, "()I");
    prepare_method_id(BFGetSizeZ, "()I");
    prepare_method_id(BFGetSizeT, "()I");
    prepare_method_id(BFGetSizeC, "()I");
    prepare_method_id(BFGetEffectiveSizeC, "()I");
    prepare_method_id(BFGetOptimalTileWidth, "()I");
    prepare_method_id(BFGetOptimalTileHeight, "()I");
    prepare_method_id(BFGetFormat, "()I");
    prepare_method_id(BFGetPixelType, "()I");
    prepare_method_id(BFGetBitsPerPixel, "()I");
    prepare_method_id(BFGetBytesPerPixel, "()I");
    prepare_method_id(BFGetRGBChannelCount, "()I");
    prepare_method_id(BFGetImageCount, "()I");
    prepare_method_id(BFIsRGB, "()I");
    prepare_method_id(BFIsInterleaved, "()I");
    prepare_method_id(BFIsLittleEndian, "()I");
    prepare_method_id(BFIsFalseColor, "()I");
    prepare_method_id(BFIsIndexedColor, "()I");
    prepare_method_id(BFGetDimensionOrder, "()I");
    prepare_method_id(BFIsOrderCertain, "()I");
    prepare_method_id(BFOpenBytes, "(IIII)I");
    prepare_method_id(BFGetMPPX, "()D");
    prepare_method_id(BFGetMPPY, "()D");
    prepare_method_id(BFGetMPPZ, "()D");
    prepare_method_id(BFIsAnyFileOpen, "()I");
    prepare_method_id(BFToolsShouldGenerate, "()I");
    prepare_method_id(BFToolsGenerateSubresolutions, "(III)I");

    return NULL;
}

void bfbridge_free_library(bfbridge_library_t *lib)
{
    // Ease of freeing
    if (lib->env)
    {
        (*(lib->env))->DestroyJavaVM(env);
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

    if (communication_buffer == NULL || communication_buffer_len < 0)
    {
        return make_error(
            BFBRIDGE_INVALID_COMMUNICATON_BUFFER,
            "communication_buffer NULL or has negative length");
    }

    JNIEnv *env = library->env;

    jobject bfbridge_local =
        (*env)->NewObject(env, library->bfbridge_base, library->constructor);
    // Should be freed: bfbridge
    jobject bfbridge = (jobject)((*env)->NewGlobalRef(env, bfbridge_local));
    (*env)->DeleteLocalRef(env, bfbridge_local);

    // Should be freed: bfbridge, buffer (the jobject only)
    jobject buffer =
        (*env)->NewDirectByteBuffer(env, communication_buffer, communication_buffer_len);
    if (!buffer)
    {
        if ((*env)->ExceptionCheck(env) == 1)
        {
            // As of JDK 20, NewDirectByteBuffer only raises OutOfMemoryError
            (*env)->ExceptionDescribe(env);
            (*env)->DeleteGlobalRef(env, bfbridge);
            return make_error(
                BFBRIDGE_OUT_OF_MEMORY_ERROR,
                "NewDirectByteBuffer failed, printing debug info to stderr",
                NULL);
        }
        else
        {
            (*env)->DeleteGlobalRef(env, bfbridge);
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
      (*env)->GetMethodID(env, library->bfbridge_base, "BFSetCommunicationBuffer",
      "(Ljava/nio/ByteBuffer;)V"
    );
    (*env)->CallVoidMethod(env, bfbridge, library->BFSetCommunicationBuffer, buffer);
    */
    (*env)->CallVoidMethod(env, bfbridge, library->BFSetCommunicationBuffer, buffer);

    (*env)->DeleteLocalRef(env, buffer);
    dest->bfbridge = bfbridge;
    return NULL;
}

void bfbridge_free_instance(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    // Ease of freeing
    if (instance->bfbridge)
    {
        (*library->env)->DeleteGlobalRef(library->env, bfbridge);
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

// Methods
// Please keep in order with bfbridge_library_t members

// Shorthand for JavaENV:
#define BFENV (*library->env)
// Instance class:
#define BFINSTC (*instance->bfbridge)

// BFSetCommunicationBuffer is used internally

// Return a C string with the last error
// This should only be called when the last bf_* method returned an error code
// May otherwise return undisplayable characters
char *bf_get_error_convenience(
    bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    int len = BFENV->CallIntMethod(BFENV, BFINSTC, library->BFGetErrorLength);
    char *buffer = bfbridge_instance_get_communication_buffer(instance, NULL);
    // The case of overflow is handled on Java side
    buffer[len] = '\0';
    return buffer;
}

// bf_get_error: fills the communication buffer with an error message
// returns: the number of bytes to read
int bf_get_error(bfbridge_instance_t *instance, bfbridge_library_t *library)
{
    return BFENV->CallIntMethod(BFENV, BFINSTC, library->BFGetErrorLength);
}

/*int bf_is_compatible(std::string filepath)
{
    fprintf(stderr, "calling is compatible %s\n", filepath.c_str());
    int len = filepath.length();
    memcpy(communication_buffer, filepath.c_str(), len);
    fprintf(stderr, "called is compatible\n");
    return jvm.env->CallIntMethod(bfbridge, jvm.BFIsCompatible, len);
}*/

/*

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
}*/

#undef BFENV
#undef BFINSTC

#ifdef __cplusplus
#undef BFENVA
#endif

#endif // !(defined(BFBRIDGE_INLINE) && !defined(BFBRIDGE_HEADER))
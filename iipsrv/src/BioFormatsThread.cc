/*
 * File:   BioFormatsThread.cc
 */

#include "BioFormatsThread.h"

#ifdef _WIN32
#error BioFormatsThread.h uses dirent.h for listing directory contents, we do not have equivalent Windows code yet
#else
#include <dirent.h>
#endif

BioFormatsThread::BioFormatsThread() {
    fprintf(stderr, "initializing\n");
    // https://docs.oracle.com/en/java/javase/20/docs/specs/jni/invocation.html
    JavaVMInitArgs vm_args;
    vm_args.version = JNI_VERSION_20;

    // In our Docker caMicroscpe deployment we pass these using fcgid.conf
    // and other conf files
    char *cpdir = getenv("BFBRIDGE_CLASSPATH");
    if (!cpdir || cpdir[0] == '\0')
    {
        fprintf(stderr, "Please set BFBRIDGE_CLASSPATH to a single directory where jar files can be found.\n");
        exit(1);
    }

    std::string cp(cpdir);
    if (cp.back() != '/')
    {
        cp += "/";
    }

#ifdef WIN32
#define JNI_PATH_SEPARATOR ";"
#else
#define JNI_PATH_SEPARATOR ":"
#endif

    std::string path_arg = "-Djava.class.path=" + cp + "*" + JNI_PATH_SEPARATOR + cp;

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
        path_arg += JNI_PATH_SEPARATOR + cp + std::string(cp_dirent->d_name);
    }
    closedir(cp_dir);

    // Should be freed
    JavaVMOption *options = new JavaVMOption[3];

    fprintf(stderr, "Java classpath (BFBRIDGE_CLASSPATH): %s\n", path_arg.c_str());
    // https://docs.oracle.com/en/java/javase/20/docs/specs/man/java.html#performance-tuning-examples
    char optimize1[] = "-XX:+UseParallelGC";
    // char optimize2[] = "-XX:+UseLargePages"; Not compatible with our linux distro
    options[0].optionString = (char *)path_arg.c_str();
    options[1].optionString = optimize1;
    // options[2].optionString = "-verbose:jni";
    vm_args.nOptions = 2;
    vm_args.options = options;
    vm_args.ignoreUnrecognized = false;

    char *cachedir = getenv("BFBRIDGE_CACHEDIR");
    std::string cache_arg;
    fprintf(stderr, "passingbfbridge cache dir: %s\n", cachedir);
    if (cachedir && cachedir[0] != '\0')
    {
        // Remember that std::strings whose pointers we use
        // must be valid until JNI_CreateJavaVM so we can't declare,
        // std::string cache_arg for example, inside an if branch.
        cache_arg = "-Dbfbridge.cachedir=" + std::string(cachedir);
        options[vm_args.nOptions++].optionString = (char *)cache_arg.c_str();
    }

    /*fprintf(stderr, "Printing JVM options:\n");
    for (int i = 0; i < vm_args.nOptions; i++)
    {
        fprintf(stderr, "%s\n", vm_args.options[i].optionString);
    }*/

    // TODO fix memory leak: destroy vm when returning from error
    int code = JNI_CreateJavaVM(&jvm, (void **)&env, &vm_args);
    delete[] options;
    if (code < 0)
    {
        fprintf(stderr, "couldn't create jvm with code %d on https://docs.oracle.com/en/java/javase/20/docs/specs/jni/functions.html#return-codes\n", code);
        throw "jvm failed";
    }

    bfbridge_base = env->FindClass("org/camicroscope/BFBridge");
    if (!bfbridge_base)
    {
        fprintf(stderr, "org.camicroscope.BFBridge (or a dependency of it) could not be found; is the jar in %s ?\n", cp.c_str());
        if (env->ExceptionCheck() == 1)
        {
            env->ExceptionDescribe();
        }

        throw "org.camicroscope.BFBridge could not be found; is the jar in %s ?\n" + cp;
    }

    // We have a class instance, but we need to get method ids
    // from the base class. Do this for the constructor function.
    constructor = env->GetMethodID(bfbridge_base, "<init>", "()V");
    if (!constructor)
    {
        fprintf(stderr, "couldn't find constructor for BFBridge\n");
        throw "couldn't find constructor for BFBridge\n";
    }

    // Now do the same but in shorthand form

#define prepare_method_id(name, descriptor)                         \
    name = env->GetMethodID(bfbridge_base, #name, descriptor);      \
    if (!name)                                                      \
    {                                                               \
        fprintf(stderr, "couldn't find method id for %s\n", #name); \
        throw "couldn't find method id for " + std::string(#name);  \
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
}
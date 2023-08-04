/*
 * File:   BioFormatsThread.h
 */

#ifndef BIOFORMATSTHREAD_H
#define BIOFORMATSTHREAD_H

#include <jni.h>
#include <string>

#ifdef _WIN32
#error BioFormatsThread.h uses dirent.h for listing directory contents, we do not have equivalent Windows code yet
#endif
#include <dirent.h>

/*
JVM/JNI has a requirement that a thread of ours
can only create 1 JVM. This can be shared among BioFormatsInstances,
or we can fork() to use new JVMs.

Instead we can use multiple BioFormatsInstance in a thread. These
allow keeping multiple files open. They share a JVM.
*/
class BioFormatsThread
{
public:
    JavaVM *jvm;
    JNIEnv *env;

    // Call FindClass just once and store it there.
    // It seems that additional calls to FindClass invalidate this pointer
    // so calling FindClass outside of BioFormatsThread breaks other code.
    jclass bfbridge_base;

    jmethodID constructor;
    jmethodID BFSetCommunicationBuffer;
    jmethodID BFClose;
    jmethodID BFGetErrorLength;
    jmethodID BFIsCompatible;
    jmethodID BFOpen;
    jmethodID BFClose;
    jmethodID BFGetResolutionCount;
    jmethodID BFSetResolutionCount;
    jmethodID BFSetSeries;
    jmethodID BFGetSeriesCount;
    jmethodID BFGetSizeX;
    jmethodID BFGetSizeY;
    jmethodID BFGetSizeZ;
    jmethodID BFGetSizeC;
    jmethodID BFGetSizeT;
    jmethodID BFGetEffectiveSizeC;
    jmethodID BFGetOptimalTileWidth;
    jmethodID BFGetOptimalTileHeight;
    jmethodID BFGetPixelType;
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

    BioFormatsThread()
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

#define prepare_method_id(name, descriptor)                           \
    name = env->GetMethodID(bfbridge_base, #name, descriptor);        \
    if (!name)                                                        \
    {                                                                 \
        fprintf(stderr, "couldn't find constructor for %s\n", #name); \
        throw "couldn't find constructor for " + std::string(#name);  \
    }

        // To print descriptors to screen
        // ensure that org/camicroscope folder has BFBridge.class
        // otherwise, compile in the parent folder of "org" with:
        // "javac -cp ".:jar_files/*" org/camicroscope/BFBridge.java"
        // and run: javap (-s) (-p) org.camicroscope.BFBridge

        prepare_method_id(BFSetCommunicationBuffer, "(Ljava/nio/ByteBuffer;)V");

        prepare_method_id(BFClose, "()I");
        prepare_method_id(BFGetErrorLength, "()I");
        prepare_method_id(BFIsCompatible, "(I)I");
        prepare_method_id(BFOpen, "(I)I");
        prepare_method_id(BFClose, "()I");
        prepare_method_id(BFGetResolutionCount, "()I");
        prepare_method_id(BFSetResolutionCount, "(I)I");
        prepare_method_id(BFSetSeries, "(I)I");
        prepare_method_id(BFGetSeriesCount, "()I");
        prepare_method_id(BFGetSizeX, "()I");
        prepare_method_id(BFGetSizeY, "()I");
        prepare_method_id(BFGetSizeZ, "()I");
        prepare_method_id(BFGetSizeC, "()I");
        prepare_method_id(BFGetSizeT, "()I");
        prepare_method_id(BFGetEffectiveSizeC, "()I");
        prepare_method_id(BFGetOptimalTileWidth, "()I");
        prepare_method_id(BFGetOptimalTileHeight, "()I");
        prepare_method_id(BFGetPixelType, "()I");
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
    }

    // iipsrv uses BioFormatsXXX on one thread and copying
    // won't be helpful, but we might construct a new one on a new thread
    BioFormatsThread(const BioFormatsThread &other) = delete;
    // Copy constructor is also not helpful, initialize JVM from scratch
    BioFormatsThread &operator=(const BioFormatsThread &other) = delete;

    ~BioFormatsThread()
    {
        jvm->DestroyJavaVM();
    }
};

#endif /* BIOFORMATSTHREAD_H */

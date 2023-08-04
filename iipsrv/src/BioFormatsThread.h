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
*/
class BioFormatsThread
{
public:
    JavaVM *jvm;
    JNIEnv *env;
    std::string cp;

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

        cp = BFBRIDGE_STRINGVALUE(BFBRIDGE_CLASSPATH);
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
        delete[] options;
        if (code < 0)
        {
            fprintf(stderr, "couldn't create jvm with code %d on https://docs.oracle.com/en/java/javase/20/docs/specs/jni/functions.html#return-codes\n", code);
            throw "jvm failed";
        }
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

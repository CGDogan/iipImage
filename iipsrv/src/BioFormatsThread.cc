/*
 * File:   BioFormatsThread.cc
 */

#include "BioFormatsThread.h"

BioFormatsThread::BioFormatsThread() {
    fprintf(stderr, "started bioformatsthread\n");
    // In our Docker caMicroscpe deployment we pass these using fcgid.conf
    // and other conf files
    // Required:
    char *cpdir = getenv("BFBRIDGE_CLASSPATH");
    if (!cpdir || cpdir[0] == '\0')
    {
        fprintf(stderr, "Please set BFBRIDGE_CLASSPATH to a single directory where jar files can be found.\n");
        exit(1);
    }

    // Optional:
    char *cachedir = getenv("BFBRIDGE_CACHEDIR");
    if (cachedir && cachedir[0] == '\0')
    {
        cachedir = NULL;
    }
    fprintf(stderr, "started bioformatsthread2\n");
    bfbridge_error_t *error = bfbridge_make_vm(&bfvm, cpdir, cachedir);
    fprintf(stderr, "started bioformatsthrea2.5\n");
    if (error) {
        fprintf(stderr, "BioFormatsThread.cc bfbridge_make_vm gave error\n");
        throw "";
    }
    fprintf(stderr, "started bioformatsthrea3\n");

    // Expensive function being used from a header-only library.
    // Shouldn't be called from a header file
    error = bfbridge_make_thread(&bfthread, &bfvm);
    fprintf(stderr, "started bioformatsthrea3.5\n");
    if (error) {
        fprintf(stderr, "BioFormatsThread.cc bfbridge_make_thread gave error\n");
        throw "";
    }
    fprintf(stderr, "started bioformatsthrea4\n");
}
/*
 * File:   BioFormatsThread.h
 */

#ifndef BIOFORMATSTHREAD_H
#define BIOFORMATSTHREAD_H

#include <jni.h>
#include <string>
#include <stdlib.h>
#define BFBRIDGE_INLINE
#define BFBRIDGE_KNOW_BUFFER_LEN
#include "bfbridge_basiclib.h"

/*
JVM/JNI has a requirement that a thread of ours
can only create 1 JVM. This can be shared among BioFormatsInstances,
or we can fork() to use new JVMs.

If we move to a multithread architecture, we can dedicate one
thread to JVM and interact through it, or make a new JVM from every thread
or call DetachCurrentThread then AttachCurrentThread in every thread.
Note: Detaching thread invalidates references https://stackoverflow.com/q/47834463

Instead we can use multiple BioFormatsInstance in a thread. These
allow keeping multiple files open. They share a JVM.
*/
class BioFormatsThread
{
public:
    bfbridge_library_t bflibrary;

    BioFormatsThread();

    // Copying a BioFormatsThread means copying a JVM and this is not
    // possible. The attempt to do that is a sign of faulty code
    // so we should show an error.
    BioFormatsThread(const BioFormatsThread &other) = delete;
    BioFormatsThread &operator=(const BioFormatsThread &other) = delete;

    ~BioFormatsThread()
    {
        bfbridge_free_library(&bflibrary);
    }
};

#endif /* BIOFORMATSTHREAD_H */

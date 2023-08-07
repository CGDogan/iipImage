/*
 * File:   BioFormatsThread.h
 */

#ifndef BIOFORMATSTHREAD_H
#define BIOFORMATSTHREAD_H

#include <jni.h>
#include <string>
#include <stdlib.h>
/*#define BFBRIDGE_INLINE
#define BFBRIDGE_KNOW_BUFFER_LEN
#include "bfbridge_basiclib.h"*/

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
    JavaVM *jvm;
    JNIEnv *env;

    // Call FindClass just once and store it there.
    // It seems that additional calls to FindClass invalidate this pointer
    // so calling FindClass outside of BioFormatsThread breaks other code.
    jclass bfbridge_base;

    jmethodID constructor;

    // See the comment "To print descriptors to screen ..."
    // for the javap command and please keep these in order with the output
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

    BioFormatsThread();

    // Copying a BioFormatsThread means copying a JVM and this is not
    // possible. The attempt to do that is a sign of faulty code
    // so we should show an error.
    BioFormatsThread(const BioFormatsThread &other) = delete;
    BioFormatsThread &operator=(const BioFormatsThread &other) = delete;

    ~BioFormatsThread()
    {
        jvm->DestroyJavaVM();
    }
};

#endif /* BIOFORMATSTHREAD_H */

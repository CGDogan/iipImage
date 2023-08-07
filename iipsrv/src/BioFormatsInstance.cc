#include "BioFormatsInstance.h"
#include "BioFormatsThread.h"

BioFormatsThread BioFormatsInstance::jvm;

BioFormatsInstance::BioFormatsInstance()
{
    jobject bfbridge_local = jvm.env->NewObject(jvm.bfbridge_base, jvm.constructor);
    // Should be freed
    bfbridge = (jobject)jvm.env->NewGlobalRef(bfbridge_local);
    jvm.env->DeleteLocalRef(bfbridge_local);

    // Should be freed
    communication_buffer = new char[bfi_communication_buffer_len];
    jobject buffer = jvm.env->NewDirectByteBuffer(communication_buffer, bfi_communication_buffer_len);
    if (!buffer)
    {
        // https://stackoverflow.com/questions/32323406/what-happens-if-a-constructor-throws-an-exception
        // Warning: throwing inside a c++ constructor means destructor
        // might not be called, so lines marked "Should be freed" above
        // need to be freed before throwing in a constructor.
        jvm.env->DeleteGlobalRef(bfbridge);
        delete[] communication_buffer;
        fprintf(stderr, "Couldn't allocate %d: too little RAM or JVM JNI doesn't support native memory access?", bfi_communication_buffer_len);
        throw "Allocation failed";
    }
    fprintf(stderr, "classloading almost done %d should be 0\n", (int)jvm.env->ExceptionCheck());
    /*
    How we would do if we hadn't been caching methods beforehand: This way:
    jmethodID BFSetCommunicationBuffer =
      jvm.env->GetMethodID(jvm.bfbridge_base, "BFSetCommunicationBuffer",
      "(Ljava/nio/ByteBuffer;)V"
    );
    jvm.env->CallVoidMethod(bfbridge, BFSetCommunicationBuffer, buffer);
    */
    jvm.env->CallVoidMethod(bfbridge, jvm.BFSetCommunicationBuffer, buffer);
    fprintf(stderr, "classloading almost3 done\n");

    jvm.env->DeleteLocalRef(buffer);
    fprintf(stderr, "classloading hopefully done\n");
}
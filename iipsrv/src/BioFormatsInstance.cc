#include "BioFormatsInstance.h"
#include "BioFormatsThread.h"

BioFormatsThread BioFormatsInstance::jvm;

BioFormatsInstance::BioFormatsInstance()
{
    // Expensive function being used from a header-only library.
    // Shouldn't be called from a header file
    bfbridge_error_t *error =
        bfbridge_make_instance(
            &bfinstance,
            &jvm,
            new char[bfi_communication_buffer_len],
            bfi_communication_buffer_len);
    if (error)
    {
        fprintf(stderr, "BioFormatsInstance.cc gave error\n");
        throw "";
    }
}
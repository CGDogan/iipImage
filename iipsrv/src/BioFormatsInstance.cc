#include "BioFormatsInstance.h"
#include "BioFormatsThread.h"

BioFormatsThread BioFormatsInstance::thread;

BioFormatsInstance::BioFormatsInstance()
{
    // Expensive function being used from a header-only library.
    // Shouldn't be called from a header file
    bfbridge_error_t *error =
        bfbridge_make_instance(
            &bfinstance,
            &thread.bfthread,
            new char[bfi_communication_buffer_len],
            bfi_communication_buffer_len);
    if (error)
    {
        fprintf(stderr, "BioFormatsInstance.cc gave error\n");
        throw "";
    }
}
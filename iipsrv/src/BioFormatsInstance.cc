#include "BioFormatsInstance.h"
#include "BioFormatsThread.h"

std::unique_ptr<BioFormatsThread> BioFormatsInstance::jvm =
    std::unique_ptr<BioFormatsThread>(new BioFormatsThread());

char *BioFormatsInstance::communication_buffer;

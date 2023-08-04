#include <jni.h>
#include "BioFormatsInstance.h"
#include "BioFormatsThread.h"

std::unique_ptr<BioFormatsThread> BioFormatsInstance::jvm =
    std::make_unique<BioFormatsThread>(new BioFormatsThread());

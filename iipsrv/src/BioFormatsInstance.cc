#include <jni.h>
#include "BioFormatsInstance.h"

JavaVM *BioFormatsInstance::jvm = nullptr;
JNIEnv *BioFormatsInstance::env = nullptr;

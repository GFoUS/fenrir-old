#include "context.h"

Context::Context(): instance(Context::FinalizeExtensions(), VALIDATION_LAYERS) {}
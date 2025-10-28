#include "LayerBase.h"
#include <algorithm>

void LayerBase::setOpacity(float opacity) {
    opacity_ = std::clamp(opacity, 0.0f, 1.0f);
}

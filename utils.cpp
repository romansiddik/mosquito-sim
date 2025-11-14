#include "utils.h"

// Utility random
float randFloat(float a, float b) {
    return a + static_cast<float>(rand()) / RAND_MAX * (b - a);
}

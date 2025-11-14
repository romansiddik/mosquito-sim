#ifndef UTILS_H
#define UTILS_H

#include <cstdlib>

// Utility random
float randFloat(float a, float b) {
    return a + static_cast<float>(rand()) / RAND_MAX * (b - a);
}

#endif // UTILS_H
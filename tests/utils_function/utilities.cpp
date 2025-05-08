#include "utilities.h"

// Sign extension functions
int32_t signExtend(uint32_t value, int bits) {
    // Check if sign bit is set
    if (value & (1 << (bits - 1))) {
        // If sign bit is 1, extend with 1's
        value |= (~0u << bits);
    } else {
        // If sign bit is 0, extend with 0's
        value &= ((1u << bits) - 1);
    }
    return static_cast<int32_t>(value);
}




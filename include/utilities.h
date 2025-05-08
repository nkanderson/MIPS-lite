#ifndef UTILITIES_H
#define UTILITIES_H

#include <stdint.h> // For fixed-width types like int32_t

// Sign extend a value 
int32_t signExtend(uint32_t value, int bits);

#endif // UTILITIES_H

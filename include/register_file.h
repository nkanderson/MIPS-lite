#pragma once

#include <cstdint>

class RegisterFile {
   private:
    uint32_t registers[32] = {0};  // Initialize all registers to 0

   public:
    // Read from register (R0 is hardwired to 0)
    inline uint32_t read(uint8_t reg) const { return (reg == 0) ? 0 : registers[reg]; }

    // Write to register (can't write to R0)
    inline void write(uint8_t reg, uint32_t value) {
        if (reg != 0) {
            registers[reg] = value;
        }
    }
};

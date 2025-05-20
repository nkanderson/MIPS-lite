/**
 * @file memory_interface.h
 * @brief Abstract memory access interface for use in processor simulation.
 *
 * This interface defines the contract for memory read and write operations.
 *
 * It is especially useful for dependency injection in unit testing,
 * where a mock implementation (e.g. using Google Mock) can simulate
 * memory behavior without relying on file I/O or large memory state.
 *
 * Typical use cases include:
 *  - Injecting test memory into a functional simulator
 *  - Verifying memory interactions using EXPECT_CALL
 *  - Avoiding file-based initialization (e.g. MemoryParser constructor)
 *
 */

#pragma once

#include <cstdint>

/**
 * @interface IMemoryParser
 * @brief Abstract interface for memory access to support mocking in tests.
 */
class IMemoryParser {
   public:
    virtual ~IMemoryParser() = default;

    /**
     * @brief Read an instruction from the given address.
     * @param address Memory address to read from.
     * @return 32-bit instruction.
     */
    virtual uint32_t readInstruction(uint32_t address) = 0;
    /**
     * @brief Read a 32-bit value from memory.
     * @param address Memory address to read.
     * @return 32-bit value.
     */
    virtual uint32_t readMemory(uint32_t address) = 0;
    /**
     * @brief Write a 32-bit value to memory.
     * @param address Memory address to write.
     * @param value 32-bit value to write.
     */
    virtual void writeMemory(uint32_t address, uint32_t value) = 0;
};

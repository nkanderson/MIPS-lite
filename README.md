# MIPS Lite Pipeline Simulator

Model of a simplified version of the MIPS ISA called MIPS-lite, including its in-order 5-stage pipeline.

## Overview

This project implements both functional and timing simulation of a MIPS-lite processor with the following capabilities:
- **Functional Simulation**: Executes instructions and tracks machine state changes
- **Timing Simulation**: Models pipeline behavior with hazard detection and optional forwarding
- **Statistics Collection**: Tracks instruction categories, register/memory usage, and timing metrics

## Instruction Set Architecture

The MIPS-lite ISA supports 18 instructions across 4 categories:

| Category | Instructions | Description |
|----------|-------------|-------------|
| **Arithmetic** | ADD, ADDI, SUB, SUBI, MUL, MULI | Basic arithmetic operations |
| **Logical** | OR, ORI, AND, ANDI, XOR, XORI | Bitwise logical operations |
| **Memory Access** | LDW, STW | Load/store word operations |
| **Control Flow** | BZ, BEQ, JR, HALT | Branches, jumps, and program termination |

### Instruction Formats
- **R-type**: `opcode(6) | rs(5) | rt(5) | rd(5) | unused(11)` - Used by ADD, SUB, MUL, OR, AND, XOR
- **I-type**: `opcode(6) | rs(5) | rt(5) | immediate(16)` - Used by all other instructions

## Requirements

### Build Dependencies
- **C++ Compiler**: GCC 7+ or Clang 6+ with C++17 support
- **Build System**: CMake 3.15+ and Ninja
- **Testing**: Google Test (automatically fetched)
- **Code Formatting**: clang-format (optional)
> See `cmake/toolchain.cmake` for more details.


## Building the Project

```bash
# Configure build
cmake --preset Debug

# Build
cmake --build --preset Debug

# Run tests
cd build/Debug && ctest --output-on-failure
```

## Usage

### Basic Simulation
```bash
./build/Debug/bin/mips_simulator -i <input_trace_file>
```

### Command Line Options
```
MIPS-Lite Pipeline Simulator

Usage: mips_simulator [OPTIONS]

Options:
  -i <file>     Input memory trace file (optional), if not provided defaults to traces/hex/randomtrace.txt
                Format: Hexadecimal instructions, one per line
                
  -o <file>     Output memory trace file (optional)
                Saves final memory state if specified
                
  -m            Print complete memory contents to stdout
                Shows final memory state in hex format
                
  -t            Enable timing information output
                Displays cycle counts and pipeline metrics
                
  -f            Enable data forwarding
                Reduces pipeline stalls through bypassing
                
Examples:
  # Basic functional simulation
  ./build/Debug/bin/mips_simulator -i traces/hex/add.txt
  
  # Timing simulation with forwarding
  ./build/Debug/bin/mips_simulator -i traces/hex/add.txt -t -f
  
  # Prints entire memory contents and saves to output file
  ./build/Debug/bin/mips_simulator -i traces/hex/add.txt -o output.txt -m
```

### Memory Trace Format

Input files should contain hexadecimal instruction words, one per line:
```
04010005    # ADDI R1, R0, 5
04020006    # ADDI R2, R0, 6
00222800    # ADD R5, R1, R2
44000000    # HALT
```

#### Creating Memory Traces

The project includes a MIPS-lite compiler that converts assembly programs to hex format:

```bash
# Compile assembly to hex
python3 scripts/mips-lite_compiler.py -i input.txt -o output.txt

# Example usage
python3 scripts/mips-lite_compiler.py -i traces/assembly/add.txt -o traces/hex/add.txt
```

**Assembly Format:**
```assembly
ADDI R1 R0 5        # Add immediate: R1 = R0 + 5
ADDI R2 R0 6        # Add immediate: R2 = R0 + 6  
ADD R5 R1 R2        # Add registers: R5 = R1 + R2
HALT                # Stop execution
```

**Instruction Syntax:**
- **R-type**: `OPCODE RD RS RT` (e.g., `ADD R3 R1 R2`)
- **I-type**: `OPCODE RT RS IMM` (e.g., `ADDI R1 R0 100`)
- **Special**: `BZ RS IMM`, `BEQ RS RT IMM`, `JR RS`, `HALT`

The compiler automatically handles:
- Instruction encoding and opcode translation 
- Register number parsing (R0-R31)
- Error checking for invalid instructions

See `traces/assembly/` for example programs that can be compiled and run.

## Testing

The project includes comprehensive unit and integration tests:

```bash
# Run all tests
cd build/Debug && ctest

# List all tests
ctest --show-only

# Run specific test(s) using Regex
ctest -R "Integration"

```

### Test Coverage
- **Unit Tests**: Individual component testing (instruction parsing, memory access, etc.)
- **Integration Tests**: Complete pipeline simulation with known expected outputs

## Project Structure

```
├── src/                    # Source code
│   ├── main.cpp            # Main simulator executable
│   ├── functional_simulator.cpp
│   ├── mips_instruction.cpp
│   ├── mips_mem_parser.cpp
│   └── stats.cpp
├── include/                # Header files
├── tests/                  # Unit and integration tests
├── traces/                 # Test programs
│   ├── assembly/           # Human-readable assembly
│   └── hex/                # Compiled hex traces
├── scripts/                # Compiler/decompiler utilities
└── build/                  # Build artifacts
```

## Pipeline Implementation

The simulator models a 5-stage pipeline:
1. **Fetch (IF)**: Instruction fetch from memory
2. **Decode (ID)**: Register read
3. **Execute (EX)**: ALU operations and branch resolution
4. **Memory (MEM)**: Data memory access
5. **Writeback (WB)**: Register file updates
> `cycle()` method handles pipeline control logic such as hazard detection and pipeline flushing when a branch is taken. 

### Hazard Handling
- **RAW Hazards**: Detected and resolved through stalling or forwarding
- **Control Hazards**: Always-not-taken branch prediction with 2-cycle penalty
- **Forwarding**: Optional EX-to-EX and MEM-to-EX bypassing

## Authors

Team project for ECE 486/586: Computer Architecture, Spring 2025
- Nik Anderson 
- Truong Le
- Teresa Nguyen
- Reece Wayt
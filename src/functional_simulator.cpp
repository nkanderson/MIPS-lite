#include "functional_simulator.h"

#include "memory_interface.h"
#include "mips_instruction.h"
#include "register_file.h"
#include "stats.h"

FunctionalSimulator::FunctionalSimulator(RegisterFile* rf, Stats* st, IMemoryParser* mem,
                                         bool enable_forwarding)
    : pc(0),
      register_file(rf),
      stats(st),
      memory_parser(mem),
      forward(enable_forwarding),
      stall(0) {
    for (auto& stage : pipeline) {
        stage = nullptr;
    }
}

uint32_t FunctionalSimulator::getPC() const { return pc; }

bool FunctionalSimulator::isForwardingEnabled() const { return forward; }

uint8_t FunctionalSimulator::getStall() const { return stall; }

Instruction* FunctionalSimulator::getPipelineStage(int stage) const {
    if (stage < 0 || stage >= 5) {
        throw std::out_of_range("Pipeline stage index out of range");
    }
    return pipeline[stage];
}

void FunctionalSimulator::setPC(uint32_t new_pc) { pc = new_pc; }

void FunctionalSimulator::setStall(uint8_t cycles) { stall = cycles; }

void FunctionalSimulator::instructionFetch() {
    // Stub: Should first check stall count to determine if a null pointer should be inserted
    // Otherwise, fetch instruction at PC using memory_parser and insert into pipeline[0],
    // then update PC by word size
}

void FunctionalSimulator::instructionDecode(Instruction* instr) {
    // Stub: Decode the instruction (parse operands, maybe identify hazards)
    // Set dirty bit on registers which will be written back to later
    (void)instr;  // Silence unused parameter warning
}

void FunctionalSimulator::execute(Instruction* instr) {
    // Stub: Perform ALU operation specified or calculate branch effective address
    (void)instr;
}

void FunctionalSimulator::memory(Instruction* instr) {
    // Stub: Access memory if needed (load or store only)
    (void)instr;
}

void FunctionalSimulator::writeBack(Instruction* instr) {
    // Stub: Write result back to register file, clear dirty bit
    (void)instr;
}

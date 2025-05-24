#include "log_instr_set.h"

using namespace mips_lite;

void execute_logical(const Instruction& instr, RegisterFile& rf) {
    uint32_t rs_val = rf.read(instr.getRs());

    uint32_t result = 0;
    switch (instr.getOpcode()) {
        case opcode::AND:
            result = rs_val & rf.read(instr.getRt());
            rf.write(instr.getRd(), result);
            break;
        case opcode::ANDI:
            result = rs_val & static_cast<uint16_t>(instr.getImmediate());
            rf.write(instr.getRt(), result);
            break;
        case opcode::OR:
            result = rs_val | rf.read(instr.getRt());
            rf.write(instr.getRd(), result);
            break;
        case opcode::ORI:
            result = rs_val | static_cast<uint16_t>(instr.getImmediate());
            rf.write(instr.getRt(), result);
            break;
        case opcode::XOR:
            result = rs_val ^ rf.read(instr.getRt());
            rf.write(instr.getRd(), result);
            break;
        case opcode::XORI:
            result = rs_val ^ static_cast<uint16_t>(instr.getImmediate());
            rf.write(instr.getRt(), result);
            break;
        default:
            throw std::runtime_error("Unsupported logical opcode");
    }
}

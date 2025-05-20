#pragma once
#include "mips_instruction.h"
#include "mips_lite_defs.h"
#include "register_file.h"

void execute_logical(const Instruction& instr, RegisterFile& rf);

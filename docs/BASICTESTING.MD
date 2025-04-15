# Basic Testing for MIPS-lite
For basic testing, 4 categories will be tested:

1. Arithmetic Instructions
2. Logical Instructions
3. Memory Access Instructions
4. Control Flow Instructions

**For R-type:**    These cases will require loading the appropriate values into two registers
and performing the operation that is stated. The result is stored in another register.

**For I-type:**   These cases will require loading the appropriate value into one register and applying the appropriate immediate using the operation that is stated. The result is stored in another register.

### TODO:
- Need to consider pipelining to tests (Ex: Encountering a jump, need to have instructions upstream to point to NOP)
- Need to consider testing odd/edge cases

## Arithmetic Instructions

These are the cases that we will implement for testing arithmetic instructions.

      1. ADD/ADDI
         a. Add two positive numbers
         b. Add two negative numbers
         c. Add one positive and one negative number
         d. Add a number and its reciprocol (result = 0)
      2. SUB/SUBI
         a. Subtract two positive numbers
         b. Subtract two negative numbers
         c. Subtract one positive and one negative number
         d. Subtract a number and its reciprocol (result = 0)
      3. MUL/MULI
         a. Multiply two positive numbers
         b. Multiply two negative numbers
         c. Multiply one positive and one negative number
         d. Multiply a number by 0
         e. Multiply a number by 1

## Logical Instructions
      4. OR/ORI
         a. OR two numbers and check the result by a per-bit basis
         b. OR a non-zero number with all 0s to see if the same number is returned
         c. OR a non-zero number with all 1s to see if a number with all 1s is returned
      5. AND/ANDI
         a. AND two numbers and check the result by a per-bit basis
         b. AND a non-zero number with all 0s to see if 0 is returned
         c. AND a non-zero number with all 1s to see if the same number is returned
      6. XOR/XORI
         a. XOR two numbers and check the result by a per-bit basis
         b. XOR a non-zero number with all 0s to see if: 
            - If bit in register is 0, it should return 0
            - If bit in register is 1, it should return 1
         c. XOR a non-zero number with all 1s to see if:
            - If bit in register is 0, it should return 1
            - If bit in register is 1, it should return 0
 
## Memory Access Instructions
      7. LDW
         a. Add Rs to Imm to generate address
            - Maybe reuse ADDI function here?
         b. Access address in memory (32-bit) and store contents into Rt
         c. Confirm that data obtained in memory is present in Rt
      8. STW
         a. Add Rs to Imm to generate address
            - Maybe reuse ADDI function here?
         b. Load contents stored in Rt (32-bits) to memory pointed by the address
         c. Confirm that data contained in Rt is stored at the right location in memory

## Control Flow Instructions
      9.  BZ (Rt is not used)
          a.  If Rs is 0:
            - Branch to the xth instruction from current instruction
          b. If Rs is not 0:
            - Continue to next instruction as normal
      10. BEQ
          a.  If Rs and Rt are equal:
            - Branch to the xth instruction from current instruction
          b.  If Rs and Rt are not equal:
            - Continue to next instruction as normal
      1.  JR
          a.  Jump to the location pointed by contents of Rs
      2.  HALT
          a.  If halt is encountered, program should halt execution.


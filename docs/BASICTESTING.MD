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

      1. Addition (ADD/ADDI) (R/I)
         a. Add two positive numbers x x
         b. Add two negative numbers x x
         c. Add one positive and one negative number x x
         d. Add a number and its reciprocol (result = 0)
      2. Subtraction (SUB/SUBI)
         a. Subtract two positive numbers
         b. Subtract two negative numbers
         c. Subtract one positive and one negative number
         d. Subtract a number and its reciprocol (result = 0)
      3. Multiplication (MUL/MULI)
         a. Multiply two positive numbers
         b. Multiply two negative numbers
         c. Multiply one positive and one negative number
         d. Multiply a number by 0
         e. Multiply a number by 1
dd
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

## Testcases

### ARITHMETIC.txt

      Immediate Instructions

      Use ADDI to add 40 and R0 into R4 (40+0=40)
      
      Use ADDI to add R4 with 10 to store back into R4 (1a I) (40+10=50)

      Use ADDI to add R4 with -1 to store back into R4 (1c I) (50+-1=49)
      
      Use SUBI to subract -50 and R0 into R5 (-50-0=-50)

      Use SUBI to subtract R5 by -2 to store back into R5 (2b I) (-50-(-2)=-48)

      Use SUBI to subtract R5 by 3 to store back into R5 (2c I) (-48-3=-51)

      Use ADDI to add R5 by -5 to store back into R8 (1b I) (-51+-5=-56)

      Use SUBI to subtract R4 by 5 to store back into R9 (2a I) (49-5=44)

      Use MULI to multiply R4 by 2 to store into R10 (3a I) (49*2=98)

      Use MULI to multiply R5 by -2 to store into R11 (3b I) (-51*-2=102)
      
      Use MULI to multiply R4 by -3 to store into R12 (3c I) (49*-3=147)

      Use MULI to multiply R4 by 0 to store into R13 (3d I) (49*0=0)

      Use MULI to multiply R4 by 1 to store into R14 (3e I) (49*1=49)

      Use ADDI to add R0 and -49 into R6 
      
      Use ADDI to add R0 and 51 into R7

      Use ADD to add R4 and R5 to store into R15 (1c R) (49+-51=-2)

      Use ADD to add R4 and R4 to store into R16 (1a R) (49+49=98)

      Use ADD to add R5 and R5 to store into R17 (1b R) (-51+-51=-102)

      Use ADD to add R4 and R6 and store into R18 (1d R) (49+-49=0)

      Use SUB to subtract R4 and R5 to store into R19 (2c R) (49-(51)=100)

      Use SUB to subtract R4 and R6 to store into R20 (2a R) (49-(-49)=98)

      Use SUB to subtract R5 and R7 to store into R21 (2b R) (-51-(51)=-102)

      Use SUB to subtract R5 and R5 to store into R22 (2d R) (-51-(-51)=0)

      Use MUL to multiply R4 and R7 to store into R23 (3a R) (49*51=2499)
      
      Use MUL to multiply R5 and R6 to store into R24 (3b R) (-51*-49=2499)
      
      Use MUL to multiply R4 and R5 to store into R25 (3c R) (49*-51=-2499)
      
      Use MUL to multiply R4 and R0 to store into R26 (3d R) (49*0=0)
      
      Use ADDI to add R0 and 1 to store into R3

      Use MUL to multiply R4 and R3 to store into R27 (3e R) (49*1=49)


Final Register Values:

R3:   1
R4:   49
R5:   -51
R6:   -49
R7:   51
R8:   -56 
R9:   44
R10:  98
R11:  102
R12:  147
R13:  0
R14:  49
R15:  -2
R16:  98
R17:  -102
R18:  0
R19:  100
R20:  98
R21:  -102
R22:  0
R23:  2499
R24:  2499
R25:  49



### LOGICAL.txt
         Use ADDI to store -256 into R2 using R0 (FF00)

         Use ADDI to store -15415 into R3 using R0 (C939)

         Use OR to OR R2 and R3 together and store into R8 (should equal FF39)

         Use ORI to OR R3 with 0s (0) and store into R8 (should be equal to C939)

         Use ORI to OR R3 with 1s (FFFF) and store into R9 (should be FFFF)

         Use AND to AND R2 and R3 to store into R10 (should be C900)

         Use ANDI to AND R3 with 0s (0) and store into R11 (should be 0)

         Use ANDI to AND R3 with 1s (FFFF) and store into R12 (should be C939)






### RUNDOWN.txt - full rundown of all instructions (WIP)

Use ADDI to store 100 and 200 using R0 in R2 and R3, respectively (ADDI Add)
Use ADD to compute addition between R2 and R3 and store in R8 (300)
Use LDW to store all 3 registers to memory in locations 300, 304 using R8 as starting address
Use SUB to subtract R2 by R3 (100 - 200) = -100 and store into R2





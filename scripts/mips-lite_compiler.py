"""
mips-lite_compiler.py

Description: Psuedo-compiler for MIPS-lite code. Converts lines of
code into hex representations to provide as a trace file for the
MIPS-lite simulator.
"""

import argparse
import sys
from enum import Enum


# Class declaration for instruction types
class TYPE(Enum):
    R_TYPE = 0  # Format for R Type: opcode | rs | rt | rd
    # Format for R code: OPCODE RD RS RT
    I_TYPE = 1  # Format for I Type: opcode | rs | rt | immediate
    # Format for I code: OPCODE RT RS IMMEDIATE
    # *Some instructions may only use 1, 2, or 3 of the I-type fields


class stats:
    def __init__(self):
        self.instructionCount = 0
        self.arithmeticCount = 0
        self.logicCount = 0
        self.memoryAccessCount = 0
        self.ControlFlowCOunt = 0

    def incInstructionCount(self):
        self.instructionCount += 1

    def incArithmeticCount(self):
        self.instructionCount += 1
        self.arithmeticCount += 1

    def incLogicCount(self):
        self.instructionCount += 1
        self.logicCount += 1

    def incMemoryAccessCount(self):
        self.memoryAccessCount += 1

    def printStats(self):
        print("Instruction Count: " + str(self.instructionCount))
        print("Arithmetic Count: " + str(self.arithmeticCount))
        print("Logic Count: " + str(self.logicCount))
        print("Memory Access Count: " + str(self.memoryAccessCount))

    def saveStats(self, filename="outputStats.txt"):
        with open(filename, "w") as f:
            f.write("Values below only show counts within the program.")
            f.write("Instruction Count: " + str(self.instructionCount) + "\n")
            f.write("Arithmetic Count: " + str(self.arithmeticCount) + "\n")
            f.write("Logic Count: " + str(self.logicCount) + "\n")
            f.write("Memory Access Count: " + str(self.memoryAccessCount) + "\n")


# ISA Lookup Table
ISAlist = {
    # Arithmetic Instructions
    "ADD": 0b000000,
    "ADDI": 0b000001,
    "SUB": 0b000010,
    "SUBI": 0b000011,
    "MUL": 0b000100,
    "MULI": 0b000101,
    "OR": 0b000110,
    "ORI": 0b000111,
    "AND": 0b001000,
    "ANDI": 0b001001,
    "XOR": 0b001010,
    "XORI": 0b001011,
    # Memory Access Instructions
    "LDW": 0b001100,
    "STW": 0b001101,
    # Control Flow Instructions
    "BZ": 0b001110,  # Usess 3/4 sections of the instruction
    "BEQ": 0b001111,
    "JR": 0b010000,  # Uses 2/4 sections of the instruction
    "HALT": 0b010001,  # Uses 1/4 sections of the instruction
}


def twosComp(value, bits):
    if value & (1 << (bits - 1)) != 0:
        value = value - (1 << bits)
    return value


# instructtoType - takes in opcode string/int and returns the type
def instructtoType(opcode):
    opString = None

    if isinstance(opcode, int):
        if debug == True:
            print("Opcode: " + str(opcode))
        opString = next(
            (key for key, value in ISAlist.items() if value == opcode), None
        )
        if opString is None:
            print("Error: Invalid opcode provided -> " + str(opcode))
            return -1

    elif isinstance(opcode, str):
        if opcode in ISAlist:
            opString = opcode
        else:
            print("Error: Invalid opcode string provided -> " + opcode)
            return -1

    match opString:
        case "ADD" | "SUB" | "MUL" | "OR" | "AND" | "XOR":
            return TYPE.R_TYPE
        case _:
            return TYPE.I_TYPE


# converttoInt - takes in 4-length array and strips any unnecessary characters to convert to int
def converttoInt(inputLine):
    # Search Instruction dictionary for instruction and binary key
    r1 = ISAlist[inputLine[0]]
    r2 = int(inputLine[1].strip("R"))
    r3 = int(inputLine[2].strip("R"))

    # Check if 4th argument of array is RD or IMM, strip accordingly
    if inputLine[3].startswith("R"):
        r4 = int(inputLine[3].strip("R"))
    else:
        r4 = int(inputLine[3])
        temp = twosComp(r4, 16)

    return r1, r2, r3, r4


# concatInstruction - takes in 4 fields and combines them based on binary layout of opcode type.
def concatInstruction(opcode, rs, rt, rd_imm):
    check = instructtoType(opcode)
    if check == TYPE.R_TYPE:
        return (opcode << 26) | (rs << 21) | (rt << 16) | (rd_imm << 11)
    elif check == TYPE.I_TYPE:
        return (opcode << 26) | (rs << 21) | (rt << 16) | (rd_imm & 0xFFFF)


def arguments():
    inputArgs = argparse.ArgumentParser(description="MIPS-Lite Compiler")
    inputArgs.add_argument(
        "-i", "--input", type=str, required=True, help="Input assembly file"
    )
    inputArgs.add_argument(
        "-o",
        "--output",
        type=str,
        required=False,
        default="output.txt",
        help="Output binary file",
    )
    inputArgs.add_argument(
        "-d", "--debug", action="store_true", help="Print outs debug messages"
    )
    return inputArgs.parse_args()


def main():
    global debug
    counts = stats()

    # Parse input command line arguments
    args = arguments()
    inputFile = args.input
    outputFile = args.output
    debug = args.debug

    # Open input file
    with open(inputFile, "r") as rfile:
        linesArray = rfile.readlines()
        if debug == True:
            print(linesArray)

    # Make array instance to hold hex values
    hexArray = []

    # Convert each line from pseudo MIPs-Lite assembly to Hex
    # EX: R-TYPE -> (0)ADD  (1)Rd   (2)Rs   (3)Rt
    # EX: I-TYPE -> (0)ADDI (1)Rt   (2)Rs   (3)Imm
    for line in linesArray:
        # Binary Layout
        # Opcode - RS - RT - RD - (Unused)
        # Opcode - RS - RT - IMM

        # Break string into segments (line should have length of 4 items)
        line = line.split(" ")

        # Clean up strings of whitespaces/newlines
        for index, segment in enumerate(line):
            line[index] = segment.strip()

        # Determine type of instruction
        check = instructtoType(line[0])
        if check == TYPE.R_TYPE:
            # Index Concatnation: [0]/[2]/[3]/[1]
            index = [0, 2, 3, 1]
            counts.incArithmeticCount()
        elif check == TYPE.I_TYPE:
            # If the instruction is one of the special I-type
            # instructions that doesn't use all of the 4 fields,
            # append 0s till a length of 4 is acheived
            if len(line) != 4:
                while len(line) < 4:
                    line.append("0")

            # Index Concatnation: [0]/[2]/[1]/[3]
            index = [0, 2, 1, 3]
        else:
            print("Error: Instruction is not a valid instruction -> " + line[0])
            sys.exit(-1)

        # Rearrange array of strings to match binary layout
        line[:] = [line[i] for i in index]

        # Print current items
        if debug == True:
            print("Current Items in Array:")
            for item in line:
                print(item)
            print(converttoInt(line))

        # Convert items in line into binary and pass the results to be concated
        instructNum = concatInstruction(*converttoInt(line))

        if debug == True:
            print(
                "Result of Binary Conversion: "
                + bin(instructNum)
                + " (0x"
                + format(instructNum, "08x")
                + ")"
            )

        hexArray.append(format(instructNum, "08x"))

        # If HALT, exit iteration over lines
        # Allows for comments within sequences indicating expected results
        if line[0] == "HALT":
            break

    if debug == True:
        print("Contents of HexArray:")
        for item in hexArray:
            print(item)

    with open(outputFile, "w") as wfile:
        for item in hexArray:
            wfile.write(item + "\n")


if __name__ == "__main__":
    main()

import argparse
from enum import Enum

# Class declaration for instruction types
class TYPE(Enum):
    R_TYPE = 0  # Format for R Type: opcode | rs | rt | rd
                # Format for R code: OPCODE RD RS RT
    I_TYPE = 1  # Format for I Type: opcode | rs | rt | immediate
                # Format for I code: OPCODE RT RS IMMEDIATE

# ISA Lookup Table
ISAlist = {
    # Arithmetic Instructions
    "ADD"   : 0b000000, 
    "ADDI"  : 0b000001,  
    "SUB"   : 0b000010,   
    "SUBI"  : 0b000011,  
    "MUL"   : 0b000100,   
    "MULI"  : 0b000101, 

    "OR"    : 0b000110,
    "ORI"   : 0b000111,
    "AND"   : 0b001000,
    "ANDI"  : 0b001001,
    "XOR"   : 0b001010,
    "XORI"  : 0b001011,

    # Memory Access Instructions
    "LDW"   : 0b001100,
    "STW"   : 0b001101,

    # Control Flow Instructions
    "BZ"    : 0b001110,     # Usess 3/4 sections of the instruction
    "BEQ"   : 0b001111,
    "JR"    : 0b010000,     # Uses 2/4 sections of the instruction
    "HALT"  : 0b010001      # Uses 1/4 sections of the instruction
}

def instructtoType(opString):
    match opString:
        case "ADD" | "SUB" | "MUL" | "OR" | "AND" | "XOR":
            return TYPE.R_TYPE
        case _:
            return TYPE.I_TYPE

def converttoInt(inputLine):
    r1 = ISAlist[inputLine[0]]
    r2 = int(inputLine[1].strip("R"))
    r3 = int(inputLine[2].strip("R"))
    if (inputLine[3].startswith("0x")):
        r4 = int(inputLine[3].strip("0x"))
    elif (inputLine[3].startswith("R")):
        r4 = int(inputLine[3].strip("R"))
    else:
        r4 = 0

    return r1, r2, r3, r4

def concatInstruction(opcode, rs, rt, rd_imm):
    if (instructtoType(opcode) == TYPE.R_TYPE):
        return (opcode << 26) | (rs << 21) | (rt << 16) | (rd_imm << 11)
    elif (instructtoType(opcode) == TYPE.I_TYPE):   
        return (opcode << 26) | (rs << 21) | (rt << 16) | (rd_imm & 0xFFFF)

def arguements():
    inputArgs = argparse.ArgumentParser(description="MIPS-Lite Compiler")
    inputArgs.add_argument("-i", "--input", type=str, required=True, help="Input assembly file")
    inputArgs.add_argument("-o", "--output", type=str, required=False, default="output.txt", help="Output binary file")
    inputArgs.add_argument("-d", "--debug", action='store_true', help="Print outs debug messages")
    return inputArgs.parse_args()

def main():
    global debug
    
    # Parse input command line arguments
    args = arguements()
    inputFile = args.input
    outputFile = args.output
    debug = args.debug
    
    # Open input file
    with open(inputFile, "r") as rfile:
        linesArray = rfile.readlines()
        print(linesArray)

    # Convert each line from pseudo MIPs-Lite assembly to Hex
    # EX: R-TYPE -> (0)ADD  (1)Rd   (2)Rs   (3)Rt
    # EX: I-TYPE -> (0)ADDI (1)Rt   (2)Rs   (3)Imm
    
    # Binary Layout
    # Opcode - RS - RT - RD - (Unused)
    # Opcode - RS - RT - IMM
    
    hexArray = []
    
    for line in linesArray:
        # Break string into segments
        line = line.split(" ")
        
        # Determine type of instruction
        if (instructtoType(line[0]) == TYPE.R_TYPE):
            # Index Concatnation: [0]/[2]/[3]/[1]
            index = [0, 2, 3, 1]
        
        elif (instructtoType(line[0]) == TYPE.I_TYPE):
            # Index Concatnation: [0]/[2]/[1]/[3]
            if (len(line) != 4):
                while (len(line) < 4):
                    line.append("0")
            index = [0, 2, 1, 3]

        # Rearrange array of strings to match binary layout
        line[:] = [line[i] for i in index]
        
        for index, segment in enumerate(line):
            # Clean up strings
            line[index] = segment.strip()
        
        if (debug == True):
            print("Current Items in Array:")
            for item in line:
                print(item)
            print(converttoInt(line))

        # Convert items in line into binary and pass the results to be concated
        instructNum = concatInstruction(*converttoInt(line))
        
        if (debug == True):
            print("Result of Binary Conversion: " + bin(instructNum) \
                  + " (" + hex(instructNum) + ")")
        
        hexArray.append(format(instructNum, '08x'))
    
    if (debug == True):
        print("Contents of HexArray:")
        for item in hexArray:
            print(item)
    
    with open(outputFile, 'w') as wfile:
        for item in hexArray:
            wfile.write(item + "\n")


if __name__ == "__main__":
    main()

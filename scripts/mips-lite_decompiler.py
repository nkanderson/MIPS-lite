import argparse
from enum import Enum
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

def twosComp(value, bits):
    if (value & (1 << (bits - 1)) != 0):
        value = value - (1 << bits)
    return value

def arguments():
    inputArgs = argparse.ArgumentParser(description="MIPS-Lite Compiler")
    inputArgs.add_argument("-i", "--input", type=str, required=True, help="Input assembly file")
    inputArgs.add_argument("-o", "--output", type=str, required=True, help="Output binary file")
    inputArgs.add_argument("-s", "--split", type=int, default=0, help="Location to split image into instruction and memory sections")
    return inputArgs.parse_args()

def main():
    args = arguments()
    inputFile = args.input
    outputFile = args.output
    split = args.split
    
    # Check if input trace has memory section
    if split == 0:
        splitRequired = False
    else:
        splitRequired = True

    binaryArray = []
    memArray = []

    # Open input file
    with open(inputFile, "r") as rfile:
        linesArray = rfile.readlines()
        
    counter = 0    
        
    for line in linesArray:
        if (int(line, 16) == 0):
            counter += 1
            continue
        
        if (splitRequired == True):
            if (counter < split):
                binaryNum = format(int(line, 16), '032b')
            else:
                binaryNum = format(int(line, 16), 'X')
        else:     
            binaryNum = format(int(line, 16), '032b')
        
        indices = [6, 11, 16, 32]
        
        binSegments = []
        start = 0
        
        if (splitRequired == True):
            
            # Check if current line is in instruction section
            if (counter < split):
                for index in indices:
                    binSegments.append(binaryNum[start:index])
                    start = index 
                binaryArray.append("Line: " + str(counter))
                binaryArray.append(binSegments)
            else:
                memArray.append("Line " + str(counter) + ": ")
                memArray.append(binaryNum)
        else:
            for index in indices:
                binSegments.append(binaryNum[start:index])
                start = index 
            binaryArray.append("Line: " + str(counter))
            binaryArray.append(binSegments)
        counter += 1
    
    currInstruct = None
    
    with open(outputFile, 'w') as wfile:
        for index, binarySegment in enumerate(binaryArray):
            # Append line counter if index of binarySegment is 0
            if index % 2 == 0:
                wfile.write(binarySegment + "\n")
                continue
            for index, segment in enumerate(binarySegment):
                match index:
                    case 0:
                        for key, val in ISAlist.items():
                            if int(segment, 2) == val:
                                segment = currInstruct = key
                                break
                        segment = f"Opcode: {segment}" + "\n"
                    case 1:
                        segment = f"Rs: R{int(segment, 2)}  "
                    case 2:
                        segment = f"Rt: R{int(segment, 2)}" + "\n"
                    case 3:
                        match currInstruct:
                            case "ADD" | "SUB" | "MUL" | "OR" | "AND" | "XOR":
                                segment = f"Rd: R{int(segment[0:5], 2)}" + "\n\n"
                            case _:
                                temp = twosComp(int(segment, 2), 16)
                                segment = f"Immediate: {temp}" + "\n\n"
                wfile.write(segment)
                
                
        for index, memLine in enumerate(memArray):
            if index % 2 == 0:
                wfile.write(memLine + " ")
                continue
            wfile.write(memLine + "\n")


if __name__ == "__main__":
    main()
# Description of RAW Hazards
A Read After Write (RAW) hazard in MIPS pipeline processors occurs when an instruction tries to read an operand before a previous instruction has finished writing to it. This is considered a "True Dependence" in compiler terminology because there's an actual data dependency requiring communication between the instructions. RAW hazards represent one of the fundamental data hazards in pipelined processors that must be addressed to maintain pipeline speed. Forwarding is used to prevent stall penalties.

# Test Traces
Below is a list of potential test traces to confirm RAW Harzards are properly detected in our pipeline simulator.

**Important Note**: Only the first test case includes a `HALT` instruction, it can be assumed that
each pipeline execution table has an implicit `HALT` instruction although it is not written. Therefore, execution cycles is the cycles you see + 1.

### T1: Basic RAW (Stall_Penalty=2)
```mips
addi r1, r2, #100  # Writes to r1 in WB stage (cycle 5)
add r4, r1, r3     # Needs r1 in ID stage (cycle 2)
halt
```
**Expected Pipeline Execution:**
| Cycle | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|-------|---|---|---|---|---|---|---|---|---|
| addi  | IF| ID| EX| MEM| WB| - | - | - | - |
| add   | - | IF| ID| STL | STL| EX| MEM| WB| - |
| halt  | - | - | IF |STL | STL| ID | EX| MEM| WB|
**Total cycles:** 9
- 5 cycles for first instruction to complete
- 3 additional cycles for second instruction (stall penalty = 2)
- 1 for HALT to fetch

### T2: Basic RAW with Intervening Instruction (Stall_Penalty=1)
```mips
addi r1, r2, #100  # Writes to r1 in WB stage (cycle 5)
addi r5, r6, #200  # Independent instruction, no hazard
addi r3, r1, #100  # Needs r1 in ID stage (cycle 3)
```

**Expected Pipeline Execution:**
| Cycle | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 |
|-------|---|---|---|---|---|---|---|---|---|
| addi r1| IF| ID| EX| MEM| WB| - | - | - | - |
| addi r5| - | IF| ID| EX | MEM| WB| - | - | - |
| addi r3| - | - | IF| ID | STL| EX| MEM| WB| - |

**Total cycles:** 9
- No stall between first and second instructions (independent)
- 1 cycle stall between second and third instructions due to RAW on r1


Here's an extended waterfall dependency chain test case:

### T3: Dependency Chain
```mips
addi r1, r0, #10   # Initialize r1 with value 10
addi r2, r1, #20   # r2 depends on r1
add  r3, r2, r2    # r3 depends on r2
sub  r4, r3, r1    # r4 depends on both r3 and r1
slt  r5, r4, r3    # r5 depends on r4 and r3
and  r6, r5, r4    # r6 depends on r5 and r4
or   r7, r6, r1    # r7 depends on r6 and r1
```

**Expected Pipeline Execution (Stall_Penalty=2):**
| Cycle | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10| 11| 12| 13| 14| 15| 16| 17| 18| 19| 20| 21| 22|
|-------|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| addi r1| IF| ID| EX| MEM| WB| - | - | - | - | - | - | - | - | - | - | - | - | - | - | - | - | - |
| addi r2| - | IF| ID| STL| STL| EX| MEM| WB| - | - | - | - | - | - | - | - | - | - | - | - | - |
| add r3 | - | - | IF| STL| STL| ID| STL| STL| EX| MEM| WB| - | - | - | - | - | - | - | - | - | - |
| sub r4 | - | - | - | -  | -  | IF| STL| STL| ID| STL| STL| EX| MEM| WB| - | - | - | - | - | - |
| slt r5 | - | - | - | -  | -  | - | -  | -  | IF| STL| STL| ID| STL| STL| EXE| MEM| WB| - | - |
| and r6 | - | - | - | -  | -  | - | -  | -  | - | -  | -  | IF| STL| STL| ID | STL| STL| EXE| MEM| WB|
| or r7  | - | - | - | - | - | - | IF | STL| STL| ID | STL| STL| EX | MEM| WB| - | - | - | - | - |

**Total cycles:** 16
- Each instruction in the dependency chain adds multiple stall cycles
- This demonstrates the severe performance impact of long dependency chains in pipelines without forwarding
- With 7 instructions, we need 16 cycles to complete (vs. ideal 11 cycles in a perfect pipeline)
- The waterfall pattern becomes very evident in the pipeline execution table

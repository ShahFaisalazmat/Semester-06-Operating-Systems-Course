Instruction Simulator
Description:
This program simulates a simplified computer architecture.
It demonstrates the Fetch-Decode-Execute cycle.
Op Codes Implemented:
LOAD   -> Load immediate value into register
ADD    -> Add two registers
SUB    -> Subtract registers
STORE  -> Store register value into memory
LOADM  -> Load value from memory into register
HALT   -> Stop program execution
Memory Layout:
Memory is represented as an array of Instruction structures.
Each instruction contains:
    - opcode
    - operand1
    - operand2
Registers:
The CPU contains 4 general-purpose registers (R0–R3).
Fetch-Decode-Execute Cycle:
1. Fetch:
   The instruction at the current program counter is retrieved.
2. Decode:
   The opcode is examined using a switch statement.
3. Execute:
   The specified operation is performed.
   The program counter is incremented automatically.
Sample Programs:
Program 1:
- Loads 5 into R0
- Loads 10 into R1
- Adds R0 and R1
- Stores result into memory location 5
Program 2:
- Loads 20 into R0
- Loads 4 into R1
- Subtracts R1 from R0
- Stores result into memory location 6
Compilation:
gcc instruction_simulator.c -o simulator
Execution:
./simulator
Output:
The program prints:
- CPU state before instruction execution
- Instruction being executed
- CPU state after execution
This demonstrates how instructions change system state step by step.
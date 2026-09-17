/*
Name: Shah Faisal
Roll No: 23I-0058
*/
#include <stdio.h>
#define MEMORY_SIZE 20
#define NUM_REGISTERS 4
typedef enum {
    LOAD,     // LOAD R, value      -> R = value
    ADD,      // ADD R1, R2         -> R1 = R1 + R2
    SUB,      // SUB R1, R2         -> R1 = R1 - R2
    STORE,    // STORE R, addr      -> memory[addr] = R
    LOADM,    // LOADM R, addr      -> R = memory[addr]
    HALT      // Stop execution
} Opcode;
typedef struct {
    Opcode opcode;
    int operand1;
    int operand2;
} Instruction;
Instruction memory[MEMORY_SIZE];
int registers[NUM_REGISTERS] = {0};
int program_counter = 0;
void print_state() {
    printf("\n--- CPU STATE ---\n");
    printf("Program Counter: %d\n", program_counter);
    printf("Registers:\n");
    for(int i = 0; i < NUM_REGISTERS; i++) {
        printf("R%d = %d\n", i, registers[i]);
    }
    printf("Memory (Data Section First 10 cells):\n");
    for(int i = 0; i < 10; i++) {
        printf("M[%d] = %d\n", i, memory[i].operand1);
    }
    printf("-----------------\n");
}
void execute_program() {
    int running = 1;
    while(running) {
        Instruction instr = memory[program_counter];
        printf("\nFetching instruction at address %d\n", program_counter);
        print_state();
        program_counter++;
        switch(instr.opcode) {
            case LOAD:
                registers[instr.operand1] = instr.operand2;
                printf("Executing LOAD\n");
                break;
            case ADD:
                registers[instr.operand1] += registers[instr.operand2];
                printf("Executing ADD\n");
                break;
            case SUB:
                registers[instr.operand1] -= registers[instr.operand2];
                printf("Executing SUB\n");
                break;
            case STORE:
                memory[instr.operand2].operand1 = registers[instr.operand1];
                printf("Executing STORE\n");
                break;
            case LOADM:
                registers[instr.operand1] = memory[instr.operand2].operand1;
                printf("Executing LOADM\n");
                break;
            case HALT:
                printf("Executing HALT\n");
                running = 0;
                break;
            default:
                printf("Unknown instruction!\n");
                running = 0;
                break;
        }
        print_state();
    }
}
/* =========================
   Sample Program 1
   R0 = 5
   R1 = 10
   R0 = R0 + R1
   Store result in memory[5]
   ========================= */
void load_sample_program1() {
    memory[0] = (Instruction){LOAD, 0, 5};
    memory[1] = (Instruction){LOAD, 1, 10};
    memory[2] = (Instruction){ADD, 0, 1};
    memory[3] = (Instruction){STORE, 0, 5};
    memory[4] = (Instruction){HALT, 0, 0};
}

/* =========================
   Sample Program 2
   R0 = 20
   R1 = 4
   R0 = R0 - R1
   Store result in memory[6]
   ========================= */
void load_sample_program2() {
    memory[0] = (Instruction){LOAD, 0, 20};
    memory[1] = (Instruction){LOAD, 1, 4};
    memory[2] = (Instruction){SUB, 0, 1};
    memory[3] = (Instruction){STORE, 0, 6};
    memory[4] = (Instruction){HALT, 0, 0};
}
int main() {
    int choice;
    printf("Instruction Simulator\n");
    printf("1. Run Sample Program PRESS 1 (Addition)\n");
    printf("2. Run Sample Program PRESS 2 (Subtraction)\n");
    printf("Choose program: ");
    scanf("%d", &choice);
    if(choice == 1)
        load_sample_program1();
    else
        load_sample_program2();
    execute_program();
    return 0;
}
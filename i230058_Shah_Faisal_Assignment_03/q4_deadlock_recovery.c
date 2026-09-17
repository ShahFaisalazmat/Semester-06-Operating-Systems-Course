#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROCESSES 10
#define MAX_RESOURCES 5

typedef struct {
    int pid;
    int priority;
    int resources_held[MAX_RESOURCES];
    int exec_time;
    bool deadlocked;
} Process;

Process processes[MAX_PROCESSES];
int num_processes, num_resources;
int available[MAX_RESOURCES];

float calculate_cost(Process* p) {
    int total_held = 0;
    for (int i = 0; i < num_resources; i++)
        total_held += p->resources_held[i];
    return (6 - p->priority) + total_held + (p->exec_time / 10.0);
}

bool detect_deadlock(int* deadlocked, int* count) {
    bool finish[MAX_PROCESSES] = {false};
    int work[MAX_RESOURCES];
    
    for (int i = 0; i < num_resources; i++)
        work[i] = available[i];
    
    bool changed;
    do {
        changed = false;
        for (int i = 0; i < num_processes; i++) {
            if (!finish[i]) {
                bool can_finish = true;
                for (int j = 0; j < num_resources; j++) {
                    if (processes[i].resources_held[j] > work[j]) {
                        can_finish = false;
                        break;
                    }
                }
                if (can_finish) {
                    for (int j = 0; j < num_resources; j++)
                        work[j] += processes[i].resources_held[j];
                    finish[i] = true;
                    changed = true;
                }
            }
        }
    } while (changed);
    
    *count = 0;
    for (int i = 0; i < num_processes; i++) {
        if (!finish[i]) {
            deadlocked[(*count)++] = i;
            processes[i].deadlocked = true;
        } else {
            processes[i].deadlocked = false;
        }
    }
    return (*count > 0);
}

void terminate_process(int pid) {
    printf("Terminating P%d | Freed: ", pid);
    for (int i = 0; i < num_resources; i++) {
        if (processes[pid].resources_held[i] > 0) {
            printf("R%d ", i);
            available[i] += processes[pid].resources_held[i];
            processes[pid].resources_held[i] = 0;
        }
    }
    printf("\n");
    processes[pid].deadlocked = false;
}

void recover_from_deadlock(int deadlocked[], int count) {
    printf("\nDeadlocked processes: ");
    for (int i = 0; i < count; i++) {
        printf("P%d (cost=%.1f) ", deadlocked[i], 
               calculate_cost(&processes[deadlocked[i]]));
    }
    printf("\n");
    
    int terminated = 0;
    int current_deadlocked[MAX_PROCESSES];
    int current_count = count;
    
    for (int i = 0; i < count; i++)
        current_deadlocked[i] = deadlocked[i];
    
    while (current_count > 0) {
        int lowest_idx = 0;
        float lowest_cost = calculate_cost(&processes[current_deadlocked[0]]);
        
        for (int i = 1; i < current_count; i++) {
            float cost = calculate_cost(&processes[current_deadlocked[i]]);
            if (cost < lowest_cost || 
                (cost == lowest_cost && current_deadlocked[i] < current_deadlocked[lowest_idx])) {
                lowest_cost = cost;
                lowest_idx = i;
            }
        }
        
        int to_terminate = current_deadlocked[lowest_idx];
        printf("\nStep %d: Lowest cost is P%d (cost=%.1f)\n", 
               terminated + 1, to_terminate, lowest_cost);
        terminate_process(to_terminate);
        terminated++;
        
        for (int i = lowest_idx; i < current_count - 1; i++)
            current_deadlocked[i] = current_deadlocked[i + 1];
        current_count--;
        
        int new_deadlocked[MAX_PROCESSES];
        int new_count;
        if (!detect_deadlock(new_deadlocked, &new_count)) {
            printf("\nRemaining deadlocked: none -- Deadlock resolved? YES\n");
            break;
        } else {
            printf("Remaining deadlocked: ");
            for (int i = 0; i < new_count; i++)
                printf("P%d ", new_deadlocked[i]);
            printf("-- Deadlock resolved? NO\n");
            current_count = new_count;
            for (int i = 0; i < current_count; i++)
                current_deadlocked[i] = new_deadlocked[i];
        }
    }
    
    printf("\nRecovery complete. %d process(es) terminated.\n", terminated);
}

void print_state() {
    printf("\n=== CURRENT SYSTEM STATE ===\n");
    printf("Available resources: ");
    for (int i = 0; i < num_resources; i++)
        printf("R%d=%d ", i, available[i]);
    printf("\n\n");
    
    printf("PID\tPriority\tHeld Resources\tExec Time\n");
    for (int i = 0; i < num_processes; i++) {
        printf("P%d\t%d\t\t", i, processes[i].priority);
        for (int j = 0; j < num_resources; j++)
            printf("%d ", processes[i].resources_held[j]);
        printf("\t\t%d ms\n", processes[i].exec_time);
    }
}

// Predefined test case for deadlock
void run_deadlock_test() {
    printf("\n--- RUNNING PREDEFINED DEADLOCK TEST ---\n");
    
    num_processes = 2;
    num_resources = 2;
    
    // Available resources: none available (deadlock scenario)
    available[0] = 0;
    available[1] = 0;
    
    // Process P0: holds R0, needs R1
    processes[0].pid = 0;
    processes[0].priority = 1;
    processes[0].exec_time = 100;
    processes[0].resources_held[0] = 1;
    processes[0].resources_held[1] = 0;
    
    // Process P1: holds R1, needs R0
    processes[1].pid = 1;
    processes[1].priority = 2;
    processes[1].exec_time = 50;
    processes[1].resources_held[0] = 0;
    processes[1].resources_held[1] = 1;
    
    printf("\nCreated deadlock scenario:\n");
    printf("P0 holds R0, waiting for R1\n");
    printf("P1 holds R1, waiting for R0\n");
    printf("Available resources: none\n");
}

int main() {
    printf("\n========================================\n");
    printf("  DEADLOCK RECOVERY SIMULATOR\n");
    printf("========================================\n\n");
    
    int test_choice;
    printf("Select option:\n");
    printf("  1. Run predefined deadlock test (recommended)\n");
    printf("  2. Manual input\n");
    printf("  3. Random generation\n");
    printf("Choice: ");
    scanf("%d", &test_choice);
    
    if (test_choice == 1) {
        run_deadlock_test();
    }
    else if (test_choice == 3) {
        srand(time(NULL));
        printf("\nEnter number of processes (2-5): ");
        scanf("%d", &num_processes);
        printf("Enter number of resources (1-3): ");
        scanf("%d", &num_resources);
        
        int total_resources[MAX_RESOURCES];
        for (int i = 0; i < num_resources; i++) {
            total_resources[i] = rand() % 15 + 5;
            available[i] = total_resources[i];
        }
        
        for (int i = 0; i < num_processes; i++) {
            processes[i].pid = i;
            processes[i].priority = rand() % 5 + 1;
            processes[i].exec_time = rand() % 100 + 10;
            
            for (int j = 0; j < num_resources; j++) {
                int max_hold = total_resources[j] / (num_processes + 1);
                processes[i].resources_held[j] = rand() % (max_hold + 1);
                available[j] -= processes[i].resources_held[j];
            }
        }
        
        for (int i = 0; i < num_resources; i++) {
            if (available[i] < 0) available[i] = 0;
        }
        
        printf("\n--- Random System State Generated ---\n");
    }
    else {
        printf("\nEnter number of processes (2-5): ");
        scanf("%d", &num_processes);
        printf("Enter number of resources (1-3): ");
        scanf("%d", &num_resources);
        
        printf("\nEnter available resources (%d values): ", num_resources);
        for (int i = 0; i < num_resources; i++)
            scanf("%d", &available[i]);
        
        for (int i = 0; i < num_processes; i++) {
            processes[i].pid = i;
            printf("\nProcess P%d:\n", i);
            printf("  Priority (1-5, 1=highest): ");
            scanf("%d", &processes[i].priority);
            printf("  Execution time (ms): ");
            scanf("%d", &processes[i].exec_time);
            printf("  Resources held (%d values): ", num_resources);
            for (int j = 0; j < num_resources; j++)
                scanf("%d", &processes[i].resources_held[j]);
        }
    }
    
    print_state();
    
    int deadlocked[MAX_PROCESSES];
    int deadlock_count;
    
    if (detect_deadlock(deadlocked, &deadlock_count)) {
        printf("\n DEADLOCK DETECTED! %d process(es) deadlocked.\n", deadlock_count);
        recover_from_deadlock(deadlocked, deadlock_count);
        print_state();
    } else {
        printf("\n✓ NO DEADLOCK DETECTED. System is in a safe state.\n");
    }
    
    return 0;
}
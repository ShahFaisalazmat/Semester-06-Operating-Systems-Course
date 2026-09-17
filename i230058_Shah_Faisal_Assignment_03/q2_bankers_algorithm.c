#include <stdio.h>
#include <stdbool.h>

#define MAX_PROCESSES 10
#define MAX_RESOURCES 10

int n, m;
int max[MAX_PROCESSES][MAX_RESOURCES];
int allocation[MAX_PROCESSES][MAX_RESOURCES];
int need[MAX_PROCESSES][MAX_RESOURCES];
int available[MAX_RESOURCES];

void compute_need() {
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            need[i][j] = max[i][j] - allocation[i][j];
}

bool is_safe_state(int* safe_sequence) {
    int work[MAX_RESOURCES];
    bool finish[MAX_PROCESSES] = {false};
    int count = 0;
    
    for (int i = 0; i < m; i++)
        work[i] = available[i];
    
    while (count < n) {
        bool found = false;
        for (int p = 0; p < n; p++) {
            if (!finish[p]) {
                bool can_allocate = true;
                for (int j = 0; j < m; j++) {
                    if (need[p][j] > work[j]) {
                        can_allocate = false;
                        break;
                    }
                }
                if (can_allocate) {
                    for (int j = 0; j < m; j++)
                        work[j] += allocation[p][j];
                    finish[p] = true;
                    safe_sequence[count++] = p;
                    found = true;
                }
            }
        }
        if (!found) {
            printf("System is UNSAFE! No safe sequence exists.\n");
            return false;
        }
    }
    
    printf("Safe sequence: ");
    for (int i = 0; i < n; i++) {
        printf("P%d", safe_sequence[i]);
        if (i < n-1) printf(" -> ");
    }
    printf("\n");
    return true;
}

bool request_resources(int pid, int request[]) {
    // Check if request exceeds need
    for (int i = 0; i < m; i++) {
        if (request[i] > need[pid][i]) {
            printf("Request by P%d -- DENIED\n", pid);
            printf("Reason: Request exceeds declared Need for P%d.\n", pid);
            return false;
        }
    }
    
    // Check if request exceeds available
    for (int i = 0; i < m; i++) {
        if (request[i] > available[i]) {
            printf("Request by P%d -- DENIED\n", pid);
            printf("Reason: Insufficient resources available.\n");
            return false;
        }
    }
    
    // Pretend to allocate
    int temp_available[MAX_RESOURCES];
    int temp_allocation[MAX_RESOURCES];
    int temp_need[MAX_RESOURCES];
    
    for (int i = 0; i < m; i++) {
        temp_available[i] = available[i];
        temp_allocation[i] = allocation[pid][i];
        temp_need[i] = need[pid][i];
    }
    
    for (int i = 0; i < m; i++) {
        available[i] -= request[i];
        allocation[pid][i] += request[i];
        need[pid][i] -= request[i];
    }
    
    int safe_sequence[MAX_PROCESSES];
    if (is_safe_state(safe_sequence)) {
        printf("Request by P%d -- GRANTED\n", pid);
        return true;
    } else {
        // Rollback
        for (int i = 0; i < m; i++) {
            available[i] = temp_available[i];
            allocation[pid][i] = temp_allocation[i];
            need[pid][i] = temp_need[i];
        }
        printf("Request by P%d -- DENIED\n", pid);
        printf("Reason: Resulting state is unsafe.\n");
        return false;
    }
}

int main() {
    printf("Enter number of processes: ");
    scanf("%d", &n);
    printf("Enter number of resource types: ");
    scanf("%d", &m);
    
    printf("\nEnter Allocation matrix (%d x %d):\n", n, m);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            scanf("%d", &allocation[i][j]);
    
    printf("\nEnter Max matrix (%d x %d):\n", n, m);
    for (int i = 0; i < n; i++)
        for (int j = 0; j < m; j++)
            scanf("%d", &max[i][j]);
    
    printf("\nEnter Available vector (%d values):\n", m);
    for (int i = 0; i < m; i++)
        scanf("%d", &available[i]);
    
    compute_need();
    
    printf("\nNeed matrix computed.\n");
    int safe_seq[MAX_PROCESSES];
    is_safe_state(safe_seq);
    
    // Handle at least 3 runtime requests
    int choice, count = 0;
    do {
        printf("\n1. Make request\n2. Exit\nChoice: ");
        scanf("%d", &choice);
        
        if (choice == 1 && count < 3) {
            int pid, request[MAX_RESOURCES];
            printf("Enter process ID: ");
            scanf("%d", &pid);
            printf("Enter request vector (%d values): ", m);
            for (int i = 0; i < m; i++)
                scanf("%d", &request[i]);
            request_resources(pid, request);
            count++;
        } else if (choice == 1 && count >= 3) {
            printf("Already processed 3 requests. Exiting.\n");
            break;
        }
    } while (choice != 2);
    
    return 0;
}
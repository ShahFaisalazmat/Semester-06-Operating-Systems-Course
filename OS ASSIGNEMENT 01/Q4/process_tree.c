/*
Name: Shah Faisal
Roll No: 23I-0058
Question 4 - Process Hierarchy Simulation
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
void create_process(char *name, int fork_level) {
    printf("Process %s | PID: %d | Parent PID: %d\n", 
           name, getpid(), getppid());
    if (fork_level >= 5) {  // A=1, B=2, C=3, D=4, E=5
        exit(0);
    }
    pid_t pid;
    char child_name[20];
    char fork_letters[]={'A', 'B', 'C', 'D', 'E'};
    for (int i=1; i <= 2; i++) {
        pid=fork();
        
        if (pid<0) {
            printf("Fork failed!\n");
            exit(1);
        }
        if (pid==0) { 
            sprintf(child_name, "%s_%c%d", name, fork_letters[fork_level], i);
            create_process(child_name, fork_level + 1);
        }
    }
    for (int i=0; i<2; i++) {
        wait(NULL);
    }
    printf("Process %s terminating | PID: %d\n", name, getpid());
    exit(0);
}
int main() {
    printf("Root Process M | PID: %d\n", getpid());
    pid_t pid1, pid2;
    // Create C1 branch
    pid1=fork();
    if (pid1<0) {
        printf("Fork failed for C1!\n");
        return 1;
    }
    if (pid1==0) {
        create_process("C1", 0);
    }
    // Create C2 branch
    pid2=fork();
    if (pid2<0) {
        printf("Fork failed for C2!\n");
        return 1;
    }
    if (pid2==0) {
        create_process("C2", 0);
    }
    wait(NULL);
    wait(NULL);
    printf("Root Process M terminating | PID: %d\n", getpid());
    return 0;
}
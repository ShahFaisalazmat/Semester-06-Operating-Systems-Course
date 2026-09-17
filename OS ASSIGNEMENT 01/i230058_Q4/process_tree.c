/*
Name: Your Name
Roll No: Your Roll No
Question 4 - Process Hierarchy Simulation
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
void create_leaf(char *name) {
    printf("Process %s | PID: %d | Parent PID: %d\n", name, getpid(), getppid());
    exit(0);
}

void create_level_E(char *name_prefix) {
    pid_t pid;
    for (int i=1; i <= 2; i++) {
        pid=fork();
        if (pid==0) {
            char name[20];
            sprintf(name, "%s%d", name_prefix, i);
            create_leaf(name);
        }
    }
    for (int i=1; i <= 2; i++)
        wait(NULL);
    printf("Process %s | PID: %d | Parent PID: %d\n", name_prefix, getpid(), getppid());
    exit(0);
}
void create_level_D(char *name) {
    pid_t pid;
    for (int i=1; i <= 2; i++) {
        pid=fork();
        if (pid==0) {
            char child_name[20];
            sprintf(child_name, "%s_D%d", name, i);
            create_level_E(child_name);
        }
    }
    for (int i=1; i <= 2; i++)
        wait(NULL);
    printf("Process %s | PID: %d | Parent PID: %d\n", name, getpid(), getppid());
    exit(0);
}
void create_level_C(char *name) {
    pid_t pid;
    for (int i=1; i <= 2; i++) {
        pid=fork();
        if (pid==0) {
            char child_name[20];
            sprintf(child_name, "%s_C%d", name, i);
            create_level_D(child_name);
        }
    }
    for (int i=1; i <= 2; i++)
        wait(NULL);
    printf("Process %s | PID: %d | Parent PID: %d\n", name, getpid(), getppid());
    exit(0);
}
void create_level_B(char *name) {
    pid_t pid;
    for (int i=1; i <= 2; i++) {
        pid=fork();
        if (pid==0) {
            char child_name[20];
            sprintf(child_name, "%s_B%d", name, i);
            create_level_C(child_name);
        }
    }
    for (int i=1; i <= 2; i++)
        wait(NULL);
    printf("Process %s | PID: %d | Parent PID: %d\n", name, getpid(), getppid());
    exit(0);
}
int main() {
    pid_t pid;
    printf("Root Process M | PID: %d\n", getpid());
    pid=fork();
    if (pid==0) {
        create_level_B("N");
    }
    pid=fork();
    if (pid==0) {
        create_level_B("C1");
    }
    wait(NULL);
    wait(NULL);
    printf("Root Process M terminating | PID: %d\n", getpid());
    return 0;
}

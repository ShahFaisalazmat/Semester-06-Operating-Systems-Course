/*
Name: Shah Faisal
Roll No: 23I-0058
Question 5 - Pattern Printing Program
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
void print_left(int n) {
    printf("\nleft\n");
    for(int i=1;i<=n;i++) {
        for(int j=1;j<=i;j++) {
            printf("*");
        }
        printf("\n");
    }
}
void print_inverted_full(int n) {
    printf("\ninverted_full\n");
    for(int i=n;i >= 1;i--) {
        // First triangle
        for(int j=1;j<=i;j++) {
            printf("*");
        } 
        // Increasing spaces between triangles
        int spaces=2 * (n - i + 1);
        for(int j=1;j<=spaces;j++) {
            printf(" ");
        }
        // Second triangle - decreasing stars
        for(int j=1;j<=i;j++) {
            printf("*");
        }
        printf("\n");
    }
}
void print_right(int n) {
    printf("\nright\n");
    for(int i=1;i<=n;i++) {
        // Print spaces
        for(int space=1;space<=n - i;space++) {
            printf(" ");
        }
        // Print stars
        for(int j=1;j<=i;j++) {
            printf("*");
        }
        printf("\n");
    }
}
int main(int argc, char *argv[]) {
    if(argc != 3) {
        printf("Error: Invalid number of arguments\n");
        printf("Usage: %s <pattern_option> <number>\n", argv[0]);
        printf("pattern_option: left, inverted_full, right\n");
        printf("number: positive integer\n");
        return 1;
    }
    char *pattern=argv[1];
    // Convert and validate number
    char *endptr;
    int n=strtol(argv[2], &endptr, 10);
    if(*endptr != '\0') {
        printf("Error: Number must be an integer\n");
        return 1;
    }
    if(n<=0) {
        printf("Error: Number must be positive\n");
        return 1;
    }
    // Print appropriate pattern
    if(strcmp(pattern, "left") == 0) {
        print_left(n);
    }
    else if(strcmp(pattern, "inverted_full") == 0) {
        print_inverted_full(n);
    }
    else if(strcmp(pattern, "right") == 0) {
        print_right(n);
    }
    else {
        printf("Error: Invalid pattern option\n");
        printf("Valid options: left, inverted_full, right\n");
        return 1;
    }
    return 0;
}
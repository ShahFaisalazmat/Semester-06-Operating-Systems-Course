#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NUM_ACCOUNTS 5
#define NUM_THREADS 6
#define NUM_TRANSFERS 10

/*
 * COMMENT BLOCK: DEADLOCK PREVENTION EXPLANATION
 * ==============================================
 * 
 * This implementation prevents deadlock by eliminating the CIRCULAR WAIT
 * condition, which is one of the four necessary conditions for deadlock.
 * 
 * The four necessary conditions for deadlock are:
 * 1. Mutual Exclusion - Resources cannot be shared (still present)
 * 2. Hold and Wait - Process holds resources while waiting (still present)
 * 3. No Preemption - Resources cannot be forcibly taken (still present)
 * 4. Circular Wait - Cycle of processes waiting on each other (ELIMINATED)
 * 
 * Our lock-ordering strategy eliminates circular wait by enforcing that
 * every thread always locks accounts in increasing order of their IDs.
 * 
 * Without this strategy, a deadlock occurs when:
 * - Thread A: lock(Acc1) then lock(Acc3)
 * - Thread B: lock(Acc3) then lock(Acc1)
 * Both threads hold one lock and wait for the other -> circular wait!
 * 
 * With our strategy:
 * - Both threads lock the lower ID (Acc1) first, then the higher ID (Acc3)
 * - One thread acquires both locks and completes
 * - The other thread then proceeds safely
 * - No circular wait can ever form because threads never hold a lower
 *   lock while waiting for a higher lock in reverse order
 */

typedef struct {
    int id;
    int balance;
    pthread_mutex_t mutex;
} Account;

Account accounts[NUM_ACCOUNTS];

// Always lock the account with the smaller ID first
void transfer(Account* from, Account* to, int amount) {
    if (from->id < to->id) {
        pthread_mutex_lock(&from->mutex);
        pthread_mutex_lock(&to->mutex);
    } else {
        pthread_mutex_lock(&to->mutex);
        pthread_mutex_lock(&from->mutex);
    }
    
    if (from->balance >= amount) {
        from->balance -= amount;
        to->balance += amount;
    }
    
    if (from->id < to->id) {
        pthread_mutex_unlock(&to->mutex);
        pthread_mutex_unlock(&from->mutex);
    } else {
        pthread_mutex_unlock(&from->mutex);
        pthread_mutex_unlock(&to->mutex);
    }
}

void* perform_transfers(void* arg) {
    int thread_id = *(int*)arg;
    
    for (int i = 0; i < NUM_TRANSFERS; i++) {
        int from_id = rand() % NUM_ACCOUNTS;
        int to_id = rand() % NUM_ACCOUNTS;
        while (from_id == to_id)
            to_id = rand() % NUM_ACCOUNTS;
        
        int amount = (rand() % 90) + 10;
        transfer(&accounts[from_id], &accounts[to_id], amount);
        usleep(10000);
    }
    
    printf("Thread %d completed all transfers\n", thread_id);
    return NULL;
}

int main() {
    srand(time(NULL));
    
    printf("Initial Account Balances:\n");
    int initial_balances[NUM_ACCOUNTS] = {1000, 1000, 1000, 1000, 1000};
    int total_initial = 0;
    
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        accounts[i].id = i;
        accounts[i].balance = initial_balances[i];
        pthread_mutex_init(&accounts[i].mutex, NULL);
        total_initial += accounts[i].balance;
        printf("Account %d: $%d\n", i, accounts[i].balance);
    }
    printf("Total initial balance: $%d\n\n", total_initial);
    
    pthread_t threads[NUM_THREADS];
    int thread_ids[NUM_THREADS];
    
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_ids[i] = i;
        pthread_create(&threads[i], NULL, perform_transfers, &thread_ids[i]);
    }
    
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    printf("\nFinal Balances:\n");
    int total_final = 0;
    for (int i = 0; i < NUM_ACCOUNTS; i++) {
        printf("Account %d: $%d\n", i, accounts[i].balance);
        total_final += accounts[i].balance;
    }
    printf("Total final balance: $%d\n", total_final);
    
    if (total_final == total_initial)
        printf("✓ All threads completed without deadlock!\n");
    
    for (int i = 0; i < NUM_ACCOUNTS; i++)
        pthread_mutex_destroy(&accounts[i].mutex);
    
    return 0;
}
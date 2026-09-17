#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_PROC 20
#define MAX_GANTT 100000
#define MAX_IO_EVENTS 1000
#define INF INT_MAX

typedef struct {
    int pid;
    char name[32];
    int arrival;
    int burst;
    int remaining;
    int completion;
    int turnaround;
    int waiting;
    int io_burst;
    int io_after;
    int io_done;
    int in_io;
    int io_start;
    int io_end;
    int original_queue;
    int current_queue;
    int final_queue;
    int demotions;
    int promotions;
    int wait_in_queue;
    int cpu_consumed;
    int base_priority;
    int q0_quantum_used;
    int q3_quantum_used;
    int io_cpu_consumed_at_trigger;
} Process;

typedef struct {
    int start;
    int end;
    int pid;
} GanttEntry;

typedef struct {
    int start;
    int end;
    int pid;
} IOEntry;

static int gantt_count = 0;
static GanttEntry gantt[MAX_GANTT];
static int io_count = 0;
static IOEntry io_events[MAX_IO_EVENTS];

static void add_gantt(int start, int end, int pid) {
    if (start == end) return;
    if (gantt_count > 0 && gantt[gantt_count - 1].pid == pid && gantt[gantt_count - 1].end == start) {
        gantt[gantt_count - 1].end = end;
        return;
    }
    gantt[gantt_count].start = start;
    gantt[gantt_count].end = end;
    gantt[gantt_count].pid = pid;
    gantt_count++;
}

static void add_io_event(int start, int end, int pid) {
    io_events[io_count].start = start;
    io_events[io_count].end = end;
    io_events[io_count].pid = pid;
    io_count++;
}

static void print_gantt(void) {
    printf("--- GANTT CHART ---\n");
    for (int i = 0; i < gantt_count; i++) {
        printf("[%d-%d] P%d ", gantt[i].start, gantt[i].end, gantt[i].pid);
    }
    printf("\n\n");
}

static void print_process_table_mlq(Process p[], int n) {
    printf("--- PROCESS TABLE ---\n");
    printf("%-6s %-16s %-6s %-6s %-6s %-6s %-6s\n",
           "PID", "Name", "Arr", "Burst", "CT", "TAT", "WT");
    float avg_tat = 0, avg_wt = 0;
    for (int i = 0; i < n; i++) {
        printf("P%-5d %-16s %-6d %-6d %-6d %-6d %-6d\n",
               p[i].pid, p[i].name, p[i].arrival, p[i].burst,
               p[i].completion, p[i].turnaround, p[i].waiting);
        avg_tat += p[i].turnaround;
        avg_wt += p[i].waiting;
    }
    printf("\nAverage TAT : %.2f ms\n", avg_tat / n);
    printf("Average WT  : %.2f ms\n", avg_wt / n);
}

static void compute_stats(Process p[], int n) {
    for (int i = 0; i < n; i++) {
        p[i].turnaround = p[i].completion - p[i].arrival;
        p[i].waiting = p[i].turnaround - p[i].burst;
    }
}

typedef struct QNode {
    int pid;
    struct QNode *next;
} QNode;

typedef struct {
    QNode *head;
    QNode *tail;
} LinkedQueue;

static void lq_init(LinkedQueue *q) {
    q->head = q->tail = NULL;
}

static int lq_empty(LinkedQueue *q) {
    return q->head == NULL;
}

static void lq_enqueue(LinkedQueue *q, int pid) {
    QNode *node = malloc(sizeof(QNode));
    node->pid = pid;
    node->next = NULL;
    if (q->tail) q->tail->next = node;
    else q->head = node;
    q->tail = node;
}

static int lq_dequeue(LinkedQueue *q) {
    if (!q->head) return -1;
    QNode *tmp = q->head;
    int pid = tmp->pid;
    q->head = tmp->next;
    if (!q->head) q->tail = NULL;
    free(tmp);
    return pid;
}

static int lq_peek(LinkedQueue *q) {
    if (!q->head) return -1;
    return q->head->pid;
}

static void lq_remove(LinkedQueue *q, int pid) {
    QNode *prev = NULL, *cur = q->head;
    while (cur) {
        if (cur->pid == pid) {
            if (prev) prev->next = cur->next;
            else q->head = cur->next;
            if (!cur->next) q->tail = prev;
            free(cur);
            return;
        }
        prev = cur;
        cur = cur->next;
    }
}

static void lq_free(LinkedQueue *q) {
    while (!lq_empty(q)) lq_dequeue(q);
}

static Process mlq_proc[MAX_PROC];
static int mlq_n = 0;

static void init_mlq_processes(void) {
    struct { int pid; char name[32]; int arr; int burst; int io_burst; int io_after; int type; } raw[] = {
        {1,  "init_daemon",    0,  12, 5,  4,  1},
        {2,  "sys_monitor",    2,  20, 0,  0,  2},
        {3,  "net_handler",    4,  8,  10, 3,  2},
        {4,  "file_indexer",   5,  35, 8,  10, 1},
        {5,  "ui_compositor",  6,  15, 6,  5,  2},
        {6,  "log_archiver",   10, 50, 12, 15, 3},
        {7,  "crypto_engine",  12, 9,  0,  0,  2},
        {8,  "db_sync",        14, 28, 15, 8,  3},
        {9,  "media_encoder",  18, 45, 20, 12, 1},
        {10, "security_scan",  20, 18, 7,  6,  2},
        {11, "backup_service", 22, 60, 25, 18, 3},
        {12, "kernel_watchdog",25, 6,  0,  0,  1},
    };
    mlq_n = 12;
    for (int i = 0; i < mlq_n; i++) {
        mlq_proc[i].pid = raw[i].pid;
        strncpy(mlq_proc[i].name, raw[i].name, 31);
        mlq_proc[i].arrival = raw[i].arr;
        mlq_proc[i].burst = raw[i].burst;
        mlq_proc[i].remaining = raw[i].burst;
        mlq_proc[i].io_burst = raw[i].io_burst;
        mlq_proc[i].io_after = raw[i].io_after;
        mlq_proc[i].io_done = 0;
        mlq_proc[i].in_io = 0;
        mlq_proc[i].completion = 0;
        mlq_proc[i].cpu_consumed = 0;
        if (raw[i].type == 1) mlq_proc[i].original_queue = 1;
        else if (raw[i].type == 2) mlq_proc[i].original_queue = 2;
        else mlq_proc[i].original_queue = 3;
        if (raw[i].type == 3) {
            if (raw[i].pid == 6) mlq_proc[i].base_priority = 2;
            else if (raw[i].pid == 8) mlq_proc[i].base_priority = 4;
            else if (raw[i].pid == 11) mlq_proc[i].base_priority = 3;
        }
    }
}

static Process *find_proc(Process p[], int n, int pid) {
    for (int i = 0; i < n; i++)
        if (p[i].pid == pid) return &p[i];
    return NULL;
}

static int srtf_pick(Process p[], int n, LinkedQueue *q) {
    int best = -1, best_rem = INF, best_pid = INF;
    QNode *cur = q->head;
    while (cur) {
        Process *pr = find_proc(p, n, cur->pid);
        if (pr->remaining < best_rem || (pr->remaining == best_rem && pr->pid < best_pid)) {
            best_rem = pr->remaining;
            best_pid = pr->pid;
            best = cur->pid;
        }
        cur = cur->next;
    }
    return best;
}

static int q3_priority_pick(Process p[], int n, LinkedQueue *q) {
    int best = -1, best_prio = INF, best_pid = INF;
    QNode *cur = q->head;
    while (cur) {
        Process *pr = find_proc(p, n, cur->pid);
        if (pr->base_priority < best_prio || (pr->base_priority == best_prio && pr->pid < best_pid)) {
            best_prio = pr->base_priority;
            best_pid = pr->pid;
            best = cur->pid;
        }
        cur = cur->next;
    }
    return best;
}

static int rr_quantum_mlq[MAX_PROC];

void run_mlq(void) {
    printf("=============================================================\n");
    printf("MLQ SCHEDULER\n");
    printf("=============================================================\n\n");

    gantt_count = 0;
    io_count = 0;

    Process *p = mlq_proc;
    int n = mlq_n;

    LinkedQueue q1, q2, q3;
    lq_init(&q1); lq_init(&q2); lq_init(&q3);

    typedef struct { int pid; int finish; } IOSlot;
    IOSlot io_queue[MAX_PROC];
    int io_queue_size = 0;
    int io_busy_until = 0;

    int time = 0, completed = 0;
    int current_pid = -1;
    int current_queue = -1;
    int rr_slice = 0;
    int q3_running_pid = -1;

    memset(rr_quantum_mlq, 0, sizeof(rr_quantum_mlq));

    int max_time = 100000;

    while (completed < n && time < max_time) {
        for (int i = 0; i < n; i++) {
            if (p[i].arrival == time) {
                if (p[i].original_queue == 1) lq_enqueue(&q1, p[i].pid);
                else if (p[i].original_queue == 2) lq_enqueue(&q2, p[i].pid);
                else lq_enqueue(&q3, p[i].pid);
            }
        }

        for (int i = 0; i < io_queue_size; i++) {
            if (io_queue[i].finish == time) {
                int pid = io_queue[i].pid;
                Process *pr = find_proc(p, n, pid);
                pr->in_io = 0;
                add_io_event(pr->io_start, time, pid);
                if (pr->original_queue == 1) lq_enqueue(&q1, pid);
                else if (pr->original_queue == 2) lq_enqueue(&q2, pid);
                else lq_enqueue(&q3, pid);
                for (int j = i; j < io_queue_size - 1; j++)
                    io_queue[j] = io_queue[j+1];
                io_queue_size--;
                i--;
            }
        }

        if (current_pid != -1) {
            Process *pr = find_proc(p, n, current_pid);
            int higher_arrived = 0;
            if (current_queue == 2 && !lq_empty(&q1)) higher_arrived = 1;
            if (current_queue == 3 && (!lq_empty(&q1) || !lq_empty(&q2))) higher_arrived = 1;

            if (higher_arrived) {
                if (current_queue == 3) {
                    lq_enqueue(&q3, current_pid);
                } else {
                    lq_enqueue(&q2, current_pid);
                }
                rr_slice = 0;
                current_pid = -1;
                current_queue = -1;
            } else if (current_queue == 1) {
                int srtf_best = srtf_pick(p, n, &q1);
                if (srtf_best != -1 && srtf_best != current_pid) {
                    Process *other = find_proc(p, n, srtf_best);
                    if (other->remaining < pr->remaining) {
                        lq_enqueue(&q1, current_pid);
                        lq_remove(&q1, srtf_best);
                        current_pid = srtf_best;
                        rr_slice = 0;
                    }
                }
            } else if (current_queue == 2) {
                int srtf_best = srtf_pick(p, n, &q2);
                if (srtf_best != -1) {
                    Process *other = find_proc(p, n, srtf_best);
                    if (other->remaining < pr->remaining ||
                        (other->remaining == pr->remaining && other->pid < pr->pid)) {
                        lq_enqueue(&q2, current_pid);
                        lq_remove(&q2, srtf_best);
                        current_pid = srtf_best;
                        rr_slice = 0;
                    }
                }
            }
        }

        if (current_pid == -1) {
            if (!lq_empty(&q1)) {
                int best = srtf_pick(p, n, &q1);
                lq_remove(&q1, best);
                current_pid = best;
                current_queue = 1;
                rr_slice = 0;
            } else if (!lq_empty(&q2)) {
                int picked = lq_peek(&q2);
                lq_dequeue(&q2);
                current_pid = picked;
                current_queue = 2;
                rr_slice = 0;
            } else if (!lq_empty(&q3)) {
                int best = q3_priority_pick(p, n, &q3);
                lq_remove(&q3, best);
                current_pid = best;
                current_queue = 3;
                q3_running_pid = best;
                rr_slice = 0;
            }
        }

        if (current_pid != -1) {
            Process *pr = find_proc(p, n, current_pid);
            add_gantt(time, time + 1, current_pid);
            pr->remaining--;
            pr->cpu_consumed++;
            rr_slice++;

            int io_trigger = (!pr->io_done && pr->io_burst > 0 && pr->cpu_consumed == pr->io_after);
            if (io_trigger) {
                pr->io_done = 1;
                pr->in_io = 1;
                pr->io_start = time + 1;
                int io_finish = time + 1;
                if (io_busy_until > time + 1) io_finish = io_busy_until;
                io_busy_until = io_finish + pr->io_burst;
                io_queue[io_queue_size].pid = current_pid;
                io_queue[io_queue_size].finish = io_busy_until;
                io_queue_size++;
                pr->io_start = io_finish;
                current_pid = -1;
                current_queue = -1;
                rr_slice = 0;
            } else if (pr->remaining == 0) {
                pr->completion = time + 1;
                completed++;
                current_pid = -1;
                current_queue = -1;
                rr_slice = 0;
            } else if (current_queue == 2 && rr_slice == 5) {
                lq_enqueue(&q2, current_pid);
                current_pid = -1;
                current_queue = -1;
                rr_slice = 0;
            }
        }

        time++;
    }

    compute_stats(p, n);
    print_gantt();

    printf("--- I/O TIMELINE ---\n");
    for (int i = 0; i < io_count; i++) {
        printf("[%d-%d] P%d (I/O) ", io_events[i].start, io_events[i].end, io_events[i].pid);
    }
    printf("\n\n");

    print_process_table_mlq(p, n);

    int busy = 0;
    for (int i = 0; i < gantt_count; i++)
        busy += gantt[i].end - gantt[i].start;
    printf("CPU Utilization: %.2f %%\n", 100.0 * busy / time);
    printf("=============================================================\n\n");

    lq_free(&q1); lq_free(&q2); lq_free(&q3);
}

static Process mlfq_proc[MAX_PROC];
static int mlfq_n = 0;

static void init_mlfq_processes(void) {
    struct { int pid; char name[32]; int arr; int burst; int io_burst; int io_after; } raw[] = {
        {1,  "init_daemon",    0,  12, 5,  4},
        {2,  "sys_monitor",    2,  20, 0,  0},
        {3,  "net_handler",    4,  8,  10, 3},
        {4,  "file_indexer",   5,  35, 8,  10},
        {5,  "ui_compositor",  6,  15, 6,  5},
        {6,  "log_archiver",   10, 50, 12, 15},
        {7,  "crypto_engine",  12, 9,  0,  0},
        {8,  "db_sync",        14, 28, 15, 8},
        {9,  "media_encoder",  18, 45, 20, 12},
        {10, "security_scan",  20, 18, 7,  6},
        {11, "backup_service", 22, 60, 25, 18},
        {12, "kernel_watchdog",25, 6,  0,  0},
    };
    mlfq_n = 12;
    for (int i = 0; i < mlfq_n; i++) {
        mlfq_proc[i].pid = raw[i].pid;
        strncpy(mlfq_proc[i].name, raw[i].name, 31);
        mlfq_proc[i].arrival = raw[i].arr;
        mlfq_proc[i].burst = raw[i].burst;
        mlfq_proc[i].remaining = raw[i].burst;
        mlfq_proc[i].io_burst = raw[i].io_burst;
        mlfq_proc[i].io_after = raw[i].io_after;
        mlfq_proc[i].io_done = 0;
        mlfq_proc[i].in_io = 0;
        mlfq_proc[i].completion = 0;
        mlfq_proc[i].cpu_consumed = 0;
        mlfq_proc[i].current_queue = 0;
        mlfq_proc[i].final_queue = 0;
        mlfq_proc[i].demotions = 0;
        mlfq_proc[i].promotions = 0;
        mlfq_proc[i].wait_in_queue = 0;
        mlfq_proc[i].base_priority = 0;
        mlfq_proc[i].q0_quantum_used = 0;
        mlfq_proc[i].q3_quantum_used = 0;
        mlfq_proc[i].io_cpu_consumed_at_trigger = 0;
    }
}

static int q1_dynamic_priority(Process *pr) {
    int aging_bonus = pr->wait_in_queue / 5;
    return pr->base_priority - aging_bonus;
}

static int q1_pick(Process p[], int n, LinkedQueue *q) {
    int best = -1, best_prio = INF, best_arr = INF, best_pid = INF;
    QNode *cur = q->head;
    while (cur) {
        Process *pr = find_proc(p, n, cur->pid);
        int prio = q1_dynamic_priority(pr);
        if (prio < best_prio ||
            (prio == best_prio && pr->arrival < best_arr) ||
            (prio == best_prio && pr->arrival == best_arr && pr->pid < best_pid)) {
            best_prio = prio;
            best_arr = pr->arrival;
            best_pid = pr->pid;
            best = cur->pid;
        }
        cur = cur->next;
    }
    return best;
}

static int mlfq_srtf_pick(Process p[], int n, LinkedQueue *q) {
    int best = -1, best_rem = INF, best_pid = INF;
    QNode *cur = q->head;
    while (cur) {
        Process *pr = find_proc(p, n, cur->pid);
        if (pr->remaining < best_rem || (pr->remaining == best_rem && pr->pid < best_pid)) {
            best_rem = pr->remaining;
            best_pid = pr->pid;
            best = cur->pid;
        }
        cur = cur->next;
    }
    return best;
}

void run_mlfq(void) {
    printf("=============================================================\n");
    printf("MLFQ SCHEDULER\n");
    printf("=============================================================\n\n");

    gantt_count = 0;
    io_count = 0;

    Process *p = mlfq_proc;
    int n = mlfq_n;

    LinkedQueue q0, q1, q2, q3;
    lq_init(&q0); lq_init(&q1); lq_init(&q2); lq_init(&q3);

    typedef struct { int pid; int finish; int io_start_actual; } IOSlot;
    IOSlot io_slots[MAX_PROC];
    int io_slot_count = 0;
    int io_device_free_at = 0;

    int wait_timer[MAX_PROC];
    int in_ready[MAX_PROC];
    memset(wait_timer, 0, sizeof(wait_timer));
    memset(in_ready, 0, sizeof(in_ready));

    int time = 0, completed = 0;
    int current_pid = -1;
    int current_level = -1;
    int q0_slice = 0;
    int q3_slice = 0;

    int max_time = 200000;

    while (completed < n && time < max_time) {
        for (int i = 0; i < n; i++) {
            if (p[i].arrival == time) {
                p[i].current_queue = 0;
                lq_enqueue(&q0, p[i].pid);
                in_ready[i] = 1;
                wait_timer[i] = 0;
            }
        }

        for (int i = 0; i < io_slot_count; i++) {
            if (io_slots[i].finish == time) {
                int pid = io_slots[i].pid;
                Process *pr = find_proc(p, n, pid);
                add_io_event(io_slots[i].io_start_actual, time, pid);
                pr->in_io = 0;
                int ql = pr->current_queue;
                if (ql == 2 && pr->remaining > 20) {
                    pr->current_queue = 3;
                    pr->demotions++;
                    lq_enqueue(&q3, pid);
                } else if (ql == 0) {
                    lq_enqueue(&q0, pid);
                } else if (ql == 1) {
                    lq_enqueue(&q1, pid);
                } else if (ql == 2) {
                    lq_enqueue(&q2, pid);
                } else {
                    lq_enqueue(&q3, pid);
                }
                int idx = pid - 1;
                in_ready[idx] = 1;
                wait_timer[idx] = 0;
                for (int j = i; j < io_slot_count - 1; j++)
                    io_slots[j] = io_slots[j+1];
                io_slot_count--;
                i--;

                if (current_pid != -1 && current_level > 0) {
                    if (pr->current_queue == 0) {
                        Process *runner = find_proc(p, n, current_pid);
                        if (runner) {
                            if (current_level == 1) {
                                runner->current_queue = 1;
                                lq_enqueue(&q1, current_pid);
                            } else if (current_level == 2) {
                                lq_enqueue(&q2, current_pid);
                            } else if (current_level == 3) {
                                lq_enqueue(&q3, current_pid);
                                q3_slice = 0;
                            }
                            in_ready[current_pid - 1] = 1;
                            wait_timer[current_pid - 1] = 0;
                            current_pid = -1;
                            current_level = -1;
                            q0_slice = 0;
                        }
                    }
                }
            }
        }

        for (int i = 0; i < n; i++) {
            if (p[i].pid == current_pid || p[i].in_io || p[i].completion > 0 || p[i].arrival > time)
                continue;
            if (in_ready[i]) {
                wait_timer[i]++;
            }
        }

        for (int i = 0; i < n; i++) {
            if (p[i].completion > 0 || p[i].in_io || p[i].arrival > time) continue;
            if (!in_ready[i]) continue;

            int ql = p[i].current_queue;
            if (ql == 3 && wait_timer[i] > 25) {
                lq_remove(&q3, p[i].pid);
                p[i].current_queue = 2;
                p[i].promotions++;
                lq_enqueue(&q2, p[i].pid);
                wait_timer[i] = 0;
            } else if (ql == 2 && wait_timer[i] > 15) {
                lq_remove(&q2, p[i].pid);
                p[i].current_queue = 1;
                p[i].base_priority = p[i].remaining;
                p[i].promotions++;
                lq_enqueue(&q1, p[i].pid);
                wait_timer[i] = 0;
            }
        }

        if (current_pid != -1) {
            Process *runner = find_proc(p, n, current_pid);
            int preempt = 0;
            int new_level = current_level;

            if (current_level > 0 && !lq_empty(&q0)) {
                preempt = 1;
                new_level = 0;
            }

            if (!preempt && current_level == 3 && (!lq_empty(&q0) || !lq_empty(&q1) || !lq_empty(&q2))) {
                preempt = 1;
            }

            if (!preempt && current_level == 2) {
                int best = mlfq_srtf_pick(p, n, &q2);
                if (best != -1) {
                    Process *other = find_proc(p, n, best);
                    if (other->remaining < runner->remaining ||
                        (other->remaining == runner->remaining && other->pid < runner->pid)) {
                        preempt = 1;
                    }
                }
            }

            if (preempt) {
                in_ready[current_pid - 1] = 1;
                wait_timer[current_pid - 1] = 0;
                if (current_level == 0) {
                    lq_enqueue(&q0, current_pid);
                } else if (current_level == 1) {
                    lq_enqueue(&q1, current_pid);
                } else if (current_level == 2) {
                    lq_enqueue(&q2, current_pid);
                } else {
                    lq_enqueue(&q3, current_pid);
                    q3_slice = 0;
                }
                current_pid = -1;
                current_level = -1;
                q0_slice = 0;
            }
        }

        if (current_pid == -1) {
            if (!lq_empty(&q0)) {
                int pid = lq_dequeue(&q0);
                current_pid = pid;
                current_level = 0;
                q0_slice = 0;
                in_ready[pid - 1] = 0;
                wait_timer[pid - 1] = 0;
            } else if (!lq_empty(&q1)) {
                int best = q1_pick(p, n, &q1);
                lq_remove(&q1, best);
                current_pid = best;
                current_level = 1;
                in_ready[best - 1] = 0;
                wait_timer[best - 1] = 0;
            } else if (!lq_empty(&q2)) {
                int best = mlfq_srtf_pick(p, n, &q2);
                lq_remove(&q2, best);
                current_pid = best;
                current_level = 2;
                in_ready[best - 1] = 0;
                wait_timer[best - 1] = 0;
            } else if (!lq_empty(&q3)) {
                int pid = lq_dequeue(&q3);
                current_pid = pid;
                current_level = 3;
                q3_slice = 0;
                in_ready[pid - 1] = 0;
                wait_timer[pid - 1] = 0;
            }
        }

        if (current_pid != -1) {
            Process *pr = find_proc(p, n, current_pid);
            add_gantt(time, time + 1, current_pid);
            pr->remaining--;
            pr->cpu_consumed++;
            if (current_level == 0) q0_slice++;
            if (current_level == 3) q3_slice++;

            int io_trigger = (!pr->io_done && pr->io_burst > 0 && pr->cpu_consumed == pr->io_after);

            if (pr->remaining == 0) {
                pr->completion = time + 1;
                pr->final_queue = current_level;
                completed++;
                in_ready[current_pid - 1] = 0;
                current_pid = -1;
                current_level = -1;
                q0_slice = 0;
                q3_slice = 0;
            } else if (io_trigger) {
                pr->io_done = 1;
                pr->in_io = 1;
                int io_actual_start = time + 1;
                if (io_device_free_at > io_actual_start)
                    io_actual_start = io_device_free_at;
                io_device_free_at = io_actual_start + pr->io_burst;
                io_slots[io_slot_count].pid = current_pid;
                io_slots[io_slot_count].finish = io_device_free_at;
                io_slots[io_slot_count].io_start_actual = io_actual_start;
                io_slot_count++;
                in_ready[current_pid - 1] = 0;
                if (current_level == 1) {
                    pr->current_queue = 2;
                    pr->demotions++;
                }
                current_pid = -1;
                current_level = -1;
                q0_slice = 0;
                q3_slice = 0;
            } else if (current_level == 0 && q0_slice == 4) {
                pr->current_queue = 1;
                pr->base_priority = pr->remaining;
                pr->demotions++;
                lq_enqueue(&q1, current_pid);
                in_ready[current_pid - 1] = 1;
                wait_timer[current_pid - 1] = 0;
                current_pid = -1;
                current_level = -1;
                q0_slice = 0;
            } else if (current_level == 3 && q3_slice == 12) {
                lq_enqueue(&q3, current_pid);
                in_ready[current_pid - 1] = 1;
                wait_timer[current_pid - 1] = 0;
                current_pid = -1;
                current_level = -1;
                q3_slice = 0;
            }
        }

        time++;
    }

    compute_stats(p, n);
    print_gantt();

    printf("--- I/O TIMELINE ---\n");
    for (int i = 0; i < io_count; i++) {
        printf("[%d-%d] P%d (I/O) ", io_events[i].start, io_events[i].end, io_events[i].pid);
    }
    printf("\n\n");

    printf("--- PROCESS TABLE ---\n");
    printf("%-6s %-16s %-6s %-6s %-6s %-6s %-6s %-8s %-10s %-10s\n",
           "PID", "Name", "Arr", "Burst", "CT", "TAT", "WT", "FinalQ", "Demotions", "Promotions");

    float avg_tat = 0, avg_wt = 0;
    for (int i = 0; i < n; i++) {
        printf("P%-5d %-16s %-6d %-6d %-6d %-6d %-6d Q%-7d %-10d %-10d\n",
               p[i].pid, p[i].name, p[i].arrival, p[i].burst,
               p[i].completion, p[i].turnaround, p[i].waiting,
               p[i].final_queue, p[i].demotions, p[i].promotions);
        avg_tat += p[i].turnaround;
        avg_wt += p[i].waiting;
    }

    int busy = 0;
    for (int i = 0; i < gantt_count; i++)
        busy += gantt[i].end - gantt[i].start;

    printf("\nAverage TAT : %.2f ms\n", avg_tat / n);
    printf("Average WT  : %.2f ms\n", avg_wt / n);
    printf("CPU Utilization: %.2f %%\n", 100.0 * busy / time);
    printf("Throughput  : %.4f processes/ms\n", (float)n / time);
    printf("=============================================================\n\n");

    lq_free(&q0); lq_free(&q1); lq_free(&q2); lq_free(&q3);
}

int main(void) {
    init_mlq_processes();
    run_mlq();

    init_mlfq_processes();
    run_mlfq();

    return 0;
}
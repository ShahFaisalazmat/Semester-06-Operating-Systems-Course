#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#define MAX_PROC 200
#define MAX_GANTT 2000000
#define MAX_IO_EVENTS 10000
#define INF INT_MAX

typedef struct {
    int pid;
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
    int current_queue;
    int final_queue;
    int demotions;
    int promotions;
    int wait_in_queue;
    int cpu_consumed;
    int base_priority;
    int q0_quantum_used;
    int q3_quantum_used;
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

static int st_gantt_count = 0;
static GanttEntry st_gantt[MAX_GANTT];
static int st_io_count = 0;
static IOEntry st_io_events[MAX_IO_EVENTS];

static void st_add_gantt(int start, int end, int pid) {
    if (start == end) return;
    if (st_gantt_count > 0 &&
        st_gantt[st_gantt_count-1].pid == pid &&
        st_gantt[st_gantt_count-1].end == start) {
        st_gantt[st_gantt_count-1].end = end;
        return;
    }
    st_gantt[st_gantt_count].start = start;
    st_gantt[st_gantt_count].end = end;
    st_gantt[st_gantt_count].pid = pid;
    st_gantt_count++;
}

static void st_add_io(int start, int end, int pid) {
    st_io_events[st_io_count].start = start;
    st_io_events[st_io_count].end = end;
    st_io_events[st_io_count].pid = pid;
    st_io_count++;
}

typedef struct QNode {
    int pid;
    struct QNode *next;
} QNode;

typedef struct {
    QNode *head;
    QNode *tail;
} LQ;

static void lq_init(LQ *q) { q->head = q->tail = NULL; }
static int lq_empty(LQ *q) { return q->head == NULL; }

static void lq_enqueue(LQ *q, int pid) {
    QNode *node = malloc(sizeof(QNode));
    node->pid = pid;
    node->next = NULL;
    if (q->tail) q->tail->next = node;
    else q->head = node;
    q->tail = node;
}

static int lq_dequeue(LQ *q) {
    if (!q->head) return -1;
    QNode *tmp = q->head;
    int pid = tmp->pid;
    q->head = tmp->next;
    if (!q->head) q->tail = NULL;
    free(tmp);
    return pid;
}

static void lq_remove(LQ *q, int pid) {
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

static void lq_free(LQ *q) {
    while (!lq_empty(q)) lq_dequeue(q);
}

static Process *find_proc(Process p[], int n, int pid) {
    for (int i = 0; i < n; i++)
        if (p[i].pid == pid) return &p[i];
    return NULL;
}

static int q1_prio(Process *pr) {
    return pr->base_priority - (pr->wait_in_queue / 5);
}

static int q1_pick(Process p[], int n, LQ *q) {
    int best = -1, bp = INF, ba = INF, bpid = INF;
    QNode *cur = q->head;
    while (cur) {
        Process *pr = find_proc(p, n, cur->pid);
        int prio = q1_prio(pr);
        if (prio < bp || (prio == bp && pr->arrival < ba) ||
            (prio == bp && pr->arrival == ba && pr->pid < bpid)) {
            bp = prio; ba = pr->arrival; bpid = pr->pid; best = cur->pid;
        }
        cur = cur->next;
    }
    return best;
}

static int srtf_pick(Process p[], int n, LQ *q) {
    int best = -1, br = INF, bpid = INF;
    QNode *cur = q->head;
    while (cur) {
        Process *pr = find_proc(p, n, cur->pid);
        if (pr->remaining < br || (pr->remaining == br && pr->pid < bpid)) {
            br = pr->remaining; bpid = pr->pid; best = cur->pid;
        }
        cur = cur->next;
    }
    return best;
}

static void run_mlfq_stress(Process p[], int n, int *out_time) {
    st_gantt_count = 0;
    st_io_count = 0;

    LQ q0, q1, q2, q3;
    lq_init(&q0); lq_init(&q1); lq_init(&q2); lq_init(&q3);

    typedef struct { int pid; int finish; int io_start_actual; } IOSlot;
    IOSlot io_slots[MAX_PROC];
    int io_slot_count = 0;
    int io_device_free_at = 0;

    int wait_timer[MAX_PROC];
    int in_ready[MAX_PROC];
    memset(wait_timer, 0, sizeof(int) * n);
    memset(in_ready, 0, sizeof(int) * n);

    int time = 0, completed = 0;
    int current_pid = -1;
    int current_level = -1;
    int q0_slice = 0;
    int q3_slice = 0;
    int max_time = 10000000;

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
                st_add_io(io_slots[i].io_start_actual, time, pid);
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
            if (in_ready[i]) wait_timer[i]++;
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
            if (current_level > 0 && !lq_empty(&q0)) preempt = 1;
            if (!preempt && current_level == 3 &&
                (!lq_empty(&q0) || !lq_empty(&q1) || !lq_empty(&q2))) preempt = 1;
            if (!preempt && current_level == 2) {
                int best = srtf_pick(p, n, &q2);
                if (best != -1) {
                    Process *other = find_proc(p, n, best);
                    if (other->remaining < runner->remaining ||
                        (other->remaining == runner->remaining && other->pid < runner->pid))
                        preempt = 1;
                }
            }
            if (preempt) {
                in_ready[current_pid - 1] = 1;
                wait_timer[current_pid - 1] = 0;
                if (current_level == 0) lq_enqueue(&q0, current_pid);
                else if (current_level == 1) lq_enqueue(&q1, current_pid);
                else if (current_level == 2) lq_enqueue(&q2, current_pid);
                else { lq_enqueue(&q3, current_pid); q3_slice = 0; }
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
                int best = srtf_pick(p, n, &q2);
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
            st_add_gantt(time, time + 1, current_pid);
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

    for (int i = 0; i < n; i++) {
        p[i].turnaround = p[i].completion - p[i].arrival;
        p[i].waiting = p[i].turnaround - p[i].burst;
    }

    *out_time = time;

    lq_free(&q0); lq_free(&q1); lq_free(&q2); lq_free(&q3);
}

static void generate_processes(Process p[], int n) {
    for (int i = 0; i < n; i++) {
        p[i].pid = i + 1;
        p[i].arrival = rand() % 51;
        p[i].burst = rand() % 60 + 1;
        p[i].io_burst = rand() % 21;
        if (p[i].io_burst > 0 && p[i].burst > 1) {
            p[i].io_after = rand() % (p[i].burst - 1) + 1;
        } else {
            p[i].io_burst = 0;
            p[i].io_after = 0;
        }
        p[i].remaining = p[i].burst;
        p[i].completion = 0;
        p[i].turnaround = 0;
        p[i].waiting = 0;
        p[i].io_done = 0;
        p[i].in_io = 0;
        p[i].current_queue = 0;
        p[i].final_queue = 0;
        p[i].demotions = 0;
        p[i].promotions = 0;
        p[i].wait_in_queue = 0;
        p[i].cpu_consumed = 0;
        p[i].base_priority = 0;
        p[i].q0_quantum_used = 0;
        p[i].q3_quantum_used = 0;
    }
}

static void run_stress_test(int n, int seed) {
    printf("=============================================================\n");
    printf("STRESS TEST: N=%d  SEED=%d\n", n, seed);
    printf("=============================================================\n");

    srand((unsigned int)seed);
    Process *p = malloc(sizeof(Process) * n);
    if (!p) { printf("Memory allocation failed\n"); return; }
    generate_processes(p, n);

    int total_time = 0;
    run_mlfq_stress(p, n, &total_time);

    int pass_no_neg_wt = 1;
    int pass_tat = 1;
    int pass_wt = 1;
    int pass_no_time_loss = 1;
    int pass_gantt_coverage = 1;
    int pass_no_overlap = 1;
    int pass_io_duration = 1;

    for (int i = 0; i < n; i++) {
        if (p[i].waiting < 0) pass_no_neg_wt = 0;
        if (p[i].turnaround != p[i].completion - p[i].arrival) pass_tat = 0;
        if (p[i].waiting != p[i].turnaround - p[i].burst) pass_wt = 0;
    }

    int total_burst = 0, total_io = 0;
    for (int i = 0; i < n; i++) {
        total_burst += p[i].burst;
        total_io += p[i].io_burst;
    }
    if (total_burst + total_io > total_time) pass_no_time_loss = 0;

    int *pid_cpu_time = calloc(n + 1, sizeof(int));
    for (int i = 0; i < st_gantt_count; i++)
        pid_cpu_time[st_gantt[i].pid] += st_gantt[i].end - st_gantt[i].start;
    for (int i = 0; i < n; i++) {
        if (pid_cpu_time[p[i].pid] != p[i].burst) pass_gantt_coverage = 0;
    }
    free(pid_cpu_time);

    for (int i = 0; i < st_gantt_count; i++) {
        for (int j = i + 1; j < st_gantt_count; j++) {
            if (st_gantt[i].end > st_gantt[j].start && st_gantt[j].end > st_gantt[i].start) {
                pass_no_overlap = 0;
                break;
            }
        }
        if (!pass_no_overlap) break;
    }

    int *pid_io_time = calloc(n + 1, sizeof(int));
    for (int i = 0; i < st_io_count; i++)
        pid_io_time[st_io_events[i].pid] += st_io_events[i].end - st_io_events[i].start;
    for (int i = 0; i < n; i++) {
        if (p[i].io_burst > 0 && pid_io_time[p[i].pid] != p[i].io_burst)
            pass_io_duration = 0;
    }
    free(pid_io_time);

    printf("ASSERTION 1 - No negative waiting time        : %s\n", pass_no_neg_wt     ? "PASS" : "FAIL");
    printf("ASSERTION 2 - TAT = CT - Arrival              : %s\n", pass_tat            ? "PASS" : "FAIL");
    printf("ASSERTION 3 - WT = TAT - Burst                : %s\n", pass_wt             ? "PASS" : "FAIL");
    printf("ASSERTION 4 - No time loss (burst+IO<=total)  : %s\n", pass_no_time_loss   ? "PASS" : "FAIL");
    printf("ASSERTION 5 - Gantt coverage = burst per PID  : %s\n", pass_gantt_coverage ? "PASS" : "FAIL");
    printf("ASSERTION 6 - No overlapping CPU execution    : %s\n", pass_no_overlap     ? "PASS" : "FAIL");
    printf("ASSERTION 7 - I/O duration correctness        : %s\n", pass_io_duration    ? "PASS" : "FAIL");

    float avg_tat = 0, avg_wt = 0;
    for (int i = 0; i < n; i++) { avg_tat += p[i].turnaround; avg_wt += p[i].waiting; }
    printf("Average TAT : %.2f ms\n", avg_tat / n);
    printf("Average WT  : %.2f ms\n", avg_wt / n);

    int all_pass = pass_no_neg_wt && pass_tat && pass_wt && pass_no_time_loss &&
                   pass_gantt_coverage && pass_no_overlap && pass_io_duration;
    if (all_pass) {
        printf("SUMMARY: ALL ASSERTIONS PASSED\n");
    } else {
        printf("SUMMARY: FAILURES DETECTED - ");
        if (!pass_no_neg_wt)     printf("[Negative WT] ");
        if (!pass_tat)           printf("[TAT mismatch] ");
        if (!pass_wt)            printf("[WT mismatch] ");
        if (!pass_no_time_loss)  printf("[Time loss] ");
        if (!pass_gantt_coverage) printf("[Gantt coverage] ");
        if (!pass_no_overlap)    printf("[Overlap] ");
        if (!pass_io_duration)   printf("[IO duration] ");
        printf("\n");
    }
    printf("=============================================================\n\n");

    free(p);
}

int main(int argc, char *argv[]) {
    if (argc == 3) {
        int n = atoi(argv[1]);
        int seed = atoi(argv[2]);
        run_stress_test(n, seed);
        return 0;
    }

    run_stress_test(20, 42);
    run_stress_test(50, 99);
    run_stress_test(100, 7);

    return 0;
}
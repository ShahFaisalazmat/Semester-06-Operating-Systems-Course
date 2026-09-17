#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_NODES 100
#define MAX_PROCESSES 10
#define MAX_RESOURCES 10

typedef struct Edge {
    int to;
    struct Edge* next;
} Edge;

typedef struct Graph {
    int num_processes;
    int num_resources;
    int num_nodes;
    Edge* adj_list[MAX_NODES];
    char node_name[MAX_NODES][10];
} Graph;

void add_edge(Graph* g, int from, int to) {
    Edge* new_edge = (Edge*)malloc(sizeof(Edge));
    new_edge->to = to;
    new_edge->next = g->adj_list[from];
    g->adj_list[from] = new_edge;
}

// DFS with visited and recursion stack
bool dfs_cycle(Graph* g, int node, bool* visited, bool* rec_stack, 
               int* parent, int* cycle_nodes, int* cycle_length) {
    if (!visited[node]) {
        visited[node] = true;
        rec_stack[node] = true;
        
        Edge* curr = g->adj_list[node];
        while (curr != NULL) {
            if (!visited[curr->to]) {
                parent[curr->to] = node;
                if (dfs_cycle(g, curr->to, visited, rec_stack, parent, 
                              cycle_nodes, cycle_length))
                    return true;
            } else if (rec_stack[curr->to]) {
                // Cycle detected
                *cycle_length = 0;
                int current = node;
                cycle_nodes[(*cycle_length)++] = curr->to;
                while (current != curr->to) {
                    cycle_nodes[(*cycle_length)++] = current;
                    current = parent[current];
                }
                cycle_nodes[(*cycle_length)++] = curr->to;
                return true;
            }
            curr = curr->next;
        }
    }
    rec_stack[node] = false;
    return false;
}

void print_cycle(Graph* g, int* cycle_nodes, int cycle_length) {
    printf("\nDeadlock Detected!\n");
    printf("Cycle: ");
    for (int i = cycle_length - 1; i >= 0; i--) {
        printf("%s", g->node_name[cycle_nodes[i]]);
        if (i > 0) printf(" -> ");
    }
    printf("\n");
}

bool detect_deadlock(Graph* g) {
    bool visited[MAX_NODES] = {false};
    bool rec_stack[MAX_NODES] = {false};
    int parent[MAX_NODES];
    int cycle_nodes[MAX_NODES];
    int cycle_length = 0;
    
    for (int i = 0; i < g->num_nodes; i++) {
        if (!visited[i]) {
            memset(parent, -1, sizeof(parent));
            if (dfs_cycle(g, i, visited, rec_stack, parent, cycle_nodes, &cycle_length)) {
                print_cycle(g, cycle_nodes, cycle_length);
                return true;
            }
        }
    }
    printf("\nNo deadlock detected.\nSystem is in a safe state.\n");
    return false;
}

int main() {
    Graph g;
    memset(&g, 0, sizeof(g));
    
    printf("Enter number of processes: ");
    scanf("%d", &g.num_processes);
    printf("Enter number of resources: ");
    scanf("%d", &g.num_resources);
    
    g.num_nodes = g.num_processes + g.num_resources;
    
    for (int i = 0; i < g.num_processes; i++)
        sprintf(g.node_name[i], "P%d", i);
    for (int i = 0; i < g.num_resources; i++)
        sprintf(g.node_name[g.num_processes + i], "R%d", i);
    
    int num_allocation;
    printf("\nEnter number of allocation edges (R->P): ");
    scanf("%d", &num_allocation);
    printf("Enter allocation edges (resource_id process_id):\n");
    for (int i = 0; i < num_allocation; i++) {
        int r_id, p_id;
        scanf("%d %d", &r_id, &p_id);
        int from = g.num_processes + r_id;
        int to = p_id;
        add_edge(&g, from, to);
    }
    
    int num_request;
    printf("\nEnter number of request edges (P->R): ");
    scanf("%d", &num_request);
    printf("Enter request edges (process_id resource_id):\n");
    for (int i = 0; i < num_request; i++) {
        int p_id, r_id;
        scanf("%d %d", &p_id, &r_id);
        int from = p_id;
        int to = g.num_processes + r_id;
        add_edge(&g, from, to);
    }
    
    detect_deadlock(&g);
    return 0;
}
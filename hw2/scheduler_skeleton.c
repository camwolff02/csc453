/**
 * CPU Scheduler Simulator
 *
 * Simulates multiple CPU scheduling algorithms:
 * - First-Come, First-Served (FCFS)
 * - Round Robin (RR)
 * - Shortest Remaining Time First (SRTF)
 * - Shortest Job First (SJF)
 * 
 * Features:
 * - Multiple CPU support
 * - Visual timeline of execution
 * - Process and CPU statistics
 * - CSV output for automated testing
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>

/************************* CONSTANTS & DEFINITIONS *************************/

// Scheduling algorithm identifiers
typedef enum {
    FCFS = 0,  // First-Come, First-Served
    RR   = 1,  // Round Robin
    SRTF = 2,  // Shortest Remaining Time First (preemptive)
    SJF  = 3   // Shortest Job First (non-preemptive)
} Algorithm;

// Process states
typedef enum {
    WAITING    = 0,  // Ready to run but not yet scheduled or arrived
    RUNNING    = 1,  // Currently executing on a CPU
    COMPLETED  = 2,  // Finished execution
    READY      = 3   // In the ready queue (specifically for RR)
} ProcessState;

// Configuration constants
#define DEFAULT_TIME_QUANTUM 2
#define MAX_PROCESSES 500
#define INITIAL_TIMELINE_CAPACITY 1000
#define MAX_LINE_LENGTH 256

// Display settings
#define TIMELINE_WIDTH 80
#define TIME_UNIT_WIDTH 5

// Color codes for visualization
#define COLOR_RESET  "\033[0m"
#define COLOR_BOLD   "\033[1m"
#define COLOR_RED    "\033[31m"
#define COLOR_GREEN  "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE   "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN   "\033[36m"
#define COLOR_WHITE  "\033[37m"

const char *PROCESS_COLORS[] = {
    COLOR_RED, COLOR_GREEN, COLOR_YELLOW, COLOR_BLUE,
    COLOR_MAGENTA, COLOR_CYAN, COLOR_WHITE
};
#define NUM_PROCESS_COLORS (sizeof(PROCESS_COLORS) / sizeof(PROCESS_COLORS[0]))

/************************* TYPE DEFINITIONS *************************/

/**
 * Process data structure containing all information about a process
 */
typedef struct {
    int pid;              // Process ID
    int arrival_time;     // Time when process becomes available
    int burst_time;       // Total CPU time required
    int priority;         // Priority (higher value = higher priority)
    int remaining_time;   // Remaining CPU time needed
    ProcessState state;   // Current state (WAITING, RUNNING, etc.)
    int start_time;       // When process first started (-1 if not started)
    int finish_time;      // When process completed (-1 if not finished)
    int waiting_time;     // Total time spent waiting
    int quantum_used;     // Time units used in current quantum (for RR)
    int response_time;    // Time between arrival and first execution
} Process;

/**
 * CPU data structure representing a processor
 */
typedef struct {
    int id;               // CPU identifier
    Process *current_process; // Process currently running (NULL if idle)
    int idle_time;        // Total time CPU was idle
    int busy_time;        // Total time CPU was busy
} CPU;

/**
 * Simple circular queue for RR scheduling
 */
typedef struct {
    int process_indices[MAX_PROCESSES]; // Array of process indices
    int front;            // Index of front element
    int rear;             // Index of rear element
    int size;             // Current queue size
} ReadyQueue;

/************************* FUNCTION PROTOTYPES *************************/

// File operations
void load_processes(const char *filename, Process **processes_ptr, int *count);

// Scheduling functions
void simulate(Process *processes, int process_count, int cpu_count, Algorithm algorithm, int time_quantum);
void handle_arrivals(Process *processes, int process_count, int current_time, Algorithm algorithm, 
                    int *arrived_indices, int *arrival_count);
void handle_rr_quantum_expiry(Process *processes, CPU *cpus, int cpu_count, int time_quantum, 
                             ReadyQueue *ready_queue, int current_time);
void handle_srtf_preemption(Process *processes, int process_count, CPU *cpus, int cpu_count, int current_time);
void assign_processes_to_idle_cpus(Process *processes, int process_count, CPU *cpus, int cpu_count, 
                                 Algorithm algorithm, ReadyQueue *ready_queue, int current_time);
void execute_processes(Process *processes, int process_count, CPU *cpus, int cpu_count, 
                      int current_time, int *completed_count);
void update_waiting_times(Process *processes, int process_count, int current_time);

// Output and visualization
void print_results(Process *processes, int process_count, CPU *cpus, int cpu_count, int **timeline, int total_time);
void print_timeline(int **timeline, int total_time, Process *processes, int process_count, int cpu_count);
void print_process_stats(Process *processes, int process_count);
void print_cpu_stats(CPU *cpus, int cpu_count);
void print_average_stats(Process *processes, int process_count);
void print_csv_output(Process *processes, int process_count, CPU *cpus, int cpu_count);

// Queue operations
void init_queue(ReadyQueue *q);
void enqueue(ReadyQueue *q, int process_idx);
int dequeue(ReadyQueue *q);

// Timeline management
void init_timeline(int ***timeline_ptr, int capacity, int cpu_count);
void expand_timeline(int ***timeline_ptr, int *capacity_ptr, int new_capacity, int cpu_count);
void cleanup_timeline(int **timeline, int capacity);

// Helper functions
const char* get_color_for_pid(int pid);
const char* algorithm_name(Algorithm algorithm);
void parse_arguments(int argc, char *argv[], Algorithm *algorithm, int *cpu_count, 
                    int *time_quantum, char **input_file);

/************************* QUEUE OPERATIONS *************************/

/**
 * Initialize a ready queue
 */
void init_queue(ReadyQueue *q) {
    q->front = 0;
    q->rear = -1;
    q->size = 0;
}

/**
 * Add a process index to the ready queue
 */
void enqueue(ReadyQueue *q, int process_idx) {
    if (q->size >= MAX_PROCESSES) {
        fprintf(stderr, "Error: Ready queue overflow!\n");
        return;
    }
    q->rear = (q->rear + 1) % MAX_PROCESSES;
    q->process_indices[q->rear] = process_idx;
    q->size++;
}

/**
 * Remove and return the next process index from the ready queue
 * Returns -1 if queue is empty TODO handle case where queue is empty
 */
int dequeue(ReadyQueue *q) {
    if (q->size <= 0) return -1; // Queue empty
    int process_idx = q->process_indices[q->front];
    q->front = (q->front + 1) % MAX_PROCESSES;
    q->size--;
    return process_idx;
}

/************************* TIMELINE MANAGEMENT *************************/

/**
 * Initialize the simulation timeline data structure
 */
void init_timeline(int ***timeline_ptr, int capacity, int cpu_count) {
    *timeline_ptr = (int **)malloc(capacity * sizeof(int *));
    if (!(*timeline_ptr)) {
        perror("Failed to allocate timeline");
        exit(EXIT_FAILURE);
    }
    
    for (int t = 0; t < capacity; t++) {
        (*timeline_ptr)[t] = (int *)malloc(cpu_count * sizeof(int));
        if (!(*timeline_ptr)[t]) {
            perror("Failed to allocate timeline row");
            // Clean up already allocated rows
            for (int k = 0; k < t; k++) free((*timeline_ptr)[k]);
            free(*timeline_ptr);
            exit(EXIT_FAILURE);
        }
        for (int c = 0; c < cpu_count; c++) {
            (*timeline_ptr)[t][c] = -1; // -1 indicates idle
        }
    }
}

/**
 * Expand timeline capacity when needed
 */
void expand_timeline(int ***timeline_ptr, int *capacity_ptr, int new_capacity, int cpu_count) {
    int **temp = (int **)realloc(*timeline_ptr, new_capacity * sizeof(int *));
    if (!temp) {
        perror("Failed to expand timeline");
        exit(EXIT_FAILURE);
    }
    *timeline_ptr = temp;

    for (int t = *capacity_ptr; t < new_capacity; t++) {
        (*timeline_ptr)[t] = (int *)malloc(cpu_count * sizeof(int));
        if (!(*timeline_ptr)[t]) {
            perror("Failed to allocate new timeline row during expansion");
            exit(EXIT_FAILURE);
        }
        for (int c = 0; c < cpu_count; c++) {
            (*timeline_ptr)[t][c] = -1;
        }
    }
    *capacity_ptr = new_capacity;
}

/**
 * Clean up the timeline data structure
 */
void cleanup_timeline(int **timeline, int capacity) {
    if (timeline) {
        for (int t = 0; t < capacity; t++) {
            free(timeline[t]);
        }
        free(timeline);
    }
}

/************************* HELPER FUNCTIONS *************************/

/**
 * Get a color code for a process ID for colorized output
 */
const char* get_color_for_pid(int pid) {
    if (pid < 0) return COLOR_RESET; // Idle
    return PROCESS_COLORS[pid % NUM_PROCESS_COLORS];
}

/**
 * Get the algorithm name as a string
 */
const char* algorithm_name(Algorithm algorithm) {
    switch (algorithm) {
        case FCFS: return "First-Come, First-Served";
        case RR:   return "Round Robin";
        case SRTF: return "Shortest Remaining Time First";
        case SJF:  return "Shortest Job First";
        default:   return "Unknown Algorithm";
    }
}

/**
 * Parse command line arguments
 */
void parse_arguments(int argc, char *argv[], Algorithm *algorithm, int *cpu_count, 
                    int *time_quantum, char **input_file) {
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            i++;
            if (strcmp(argv[i], "FCFS") == 0) *algorithm = FCFS;
            else if (strcmp(argv[i], "RR") == 0) *algorithm = RR;
            else if (strcmp(argv[i], "SRTF") == 0) *algorithm = SRTF;
            else if (strcmp(argv[i], "SJF") == 0) *algorithm = SJF;
            // Default is FCFS
        } else if (strcmp(argv[i], "-c") == 0 && i + 1 < argc) {
            *cpu_count = atoi(argv[++i]);
            if (*cpu_count <= 0) *cpu_count = 1; // Ensure at least 1 CPU
        } else if (strcmp(argv[i], "-q") == 0 && i + 1 < argc) {
            *time_quantum = atoi(argv[++i]);
            if (*time_quantum <= 0) *time_quantum = DEFAULT_TIME_QUANTUM;
        } else if (strcmp(argv[i], "-f") == 0 && i + 1 < argc) {
            *input_file = argv[++i];
        } else {
            fprintf(stderr, "Usage: %s -f <file> [-a <FCFS|RR|SRTF|SJF>] [-c <cpus>] [-q <quantum>]\n", argv[0]);
            exit(EXIT_FAILURE);
        }
    }

    if (!(*input_file)) {
        fprintf(stderr, "Error: Input file required. Use -f <filename>\n");
        exit(EXIT_FAILURE);
    }
}

/************************* PROCESS LOADING *************************/

/**
 * Load processes from a file
 * 
 * Expected format:
 * <PID> <arrival_time> <burst_time> [priority]
 * 
 * Lines starting with # are treated as comments
 */
void load_processes(const char *filename, Process **processes_ptr, int *count) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Error opening process file");
        exit(EXIT_FAILURE);
    }

    // Count valid lines first
    int process_count = 0;
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] != '#' && line[0] != '\n' && strspn(line, " \t\n\r") != strlen(line)) {
            int pid, arrival, burst;
            if (sscanf(line, "%d %d %d", &pid, &arrival, &burst) == 3) {
                process_count++;
            }
        }
    }

    if (process_count == 0) {
        *processes_ptr = NULL;
        *count = 0;
        fclose(file);
        printf("Warning: No valid processes found in %s\n", filename);
        return;
    }

    // Allocate memory
    *processes_ptr = (Process *)malloc(process_count * sizeof(Process));
    if (!(*processes_ptr)) {
        perror("Memory allocation failed for processes");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // Read process data
    rewind(file);
    int i = 0;
    while (fgets(line, sizeof(line), file) && i < process_count) {
        if (line[0] == '#' || line[0] == '\n' || strspn(line, " \t\n\r") == strlen(line)) continue;

        int pid, arrival, burst, priority = 0; // Default priority
        int items_read = sscanf(line, "%d %d %d %d", &pid, &arrival, &burst, &priority);

        if (items_read >= 3) { // Need at least PID, arrival, burst
            Process *p = &(*processes_ptr)[i];
            p->pid = pid;
            p->arrival_time = arrival;
            p->burst_time = burst;
            p->priority = (items_read == 4) ? priority : 0; // Assign priority if read
            p->remaining_time = burst;
            p->state = WAITING;
            p->start_time = -1;
            p->finish_time = -1;
            p->waiting_time = 0;
            p->quantum_used = 0;
            p->response_time = -1;
            i++;
        }
    }
    fclose(file);

    *count = i; // Actual number of processes successfully read
    printf("Loaded %d processes from %s\n", *count, filename);
}

/************************* SIMULATION COMPONENTS *************************/

/**
 * Handle process arrivals at the current time
 */
void handle_arrivals(Process *processes, int process_count, int current_time, Algorithm algorithm, 
                   int *arrived_indices, int *arrival_count) {
    // DONE: Implement process arrival handling
    // 
    // This function should:
    // 1. Set *arrival_count to 0
    // 2. Check all processes to find any that arrived at 'current_time'
    // 3. For RR and SRTF algorithms, mark these processes as READY
    // 4. For all algorithms, add the indices of newly arrived processes to arrived_indices[]
    // 5. Increment *arrival_count for each arrived process
    //
    // Hint: processes[i].arrival_time == current_time indicates a process has just arrived
	
    *arrival_count = 0; // Initialize arrival count

	for (int i = 0; i < process_count; i++) {
		if (processes[i].arrival_time == current_time) {
			if (algorithm == RR || algorithm == SRTF) {
				processes[i].state = READY;
			}							
			arrived_indices[*arrival_count] = i;
			(*arrival_count)++;
		}
	}
}

/**
 * Handle quantum expiration for Round Robin scheduling
 */
void handle_rr_quantum_expiry(Process *processes, CPU *cpus, int cpu_count, int time_quantum, 
                           ReadyQueue *ready_queue, int current_time) {
    // DONE: Implement the Round Robin quantum expiration logic
    //
    // This function should:
    // 1. Check all CPUs for processes that have used up their time quantum
    // 2. For processes that have used up their quantum:
    //    - Change their state to READY
    //    - Remove them from their CPU (set CPU's current_process to NULL)
    //    - Add them to the ready_queue
    // 3. A process has used up its quantum when its quantum_used >= time_quantum
    //
    // Note: The current_time parameter is not used but kept for API consistency
    (void)current_time; // Explicitly mark as unused
	(void)processes; // TODO should this be used?

	for (int i = 0; i < cpu_count; i++) {
		Process *process = cpus[i].current_process;

		if (process != NULL && process->quantum_used >= time_quantum) {
			process->state = READY;
			cpus[i].current_process = NULL;	
            enqueue(ready_queue, process->pid);
		}
	}
}


/**
 * Implement preemptive scheduling for SRTF
 */
void handle_srtf_preemption(Process *processes, int process_count, CPU *cpus, int cpu_count, int current_time) {
    // DONE: Implement Shortest Remaining Time First preemptive logic
    //
    // This function should:
    // 1. Find the process with the shortest remaining time that's ready to run
    //    (not completed, not running, and has arrived by current_time)
    // 2. If multiple processes tie for shortest time, prioritize by priority value
    // 3. For each CPU, check if its running process should be preempted:
    //    - If the shortest ready process has less remaining time than 
    //      a currently running process, preempt the running process
    //    - When preempting, consider preempting the running process with lowest priority
    // 4. When a preemption occurs:
    //    - Change preempted process state to WAITING
    //    - Change new process state to RUNNING
    //    - Update CPU's current_process pointer
    //    - Set start_time and response_time for the new process if this is its first run
    //
    // Hint: You may need to repeat this until no more preemptions occur

	CPU *preempt_cpu = NULL;

	do {
		// Decide which process is ready to run next
		Process *min_process = NULL;
		for (int i = 0; i < process_count; i++) {
			Process *process = &processes[i];

			if (process->arrival_time <= current_time 
				&& process->state == READY 
				&& (min_process == NULL 
					|| process->remaining_time < min_process->remaining_time
					|| (process->remaining_time == min_process->remaining_time 
						&& process->priority > min_process->priority))) {
				min_process = process;
			} 		
		}

		if (min_process == NULL) {
			break;  // We can't preempt if there's no processes to run
		}

		// Decide which CPU is ready to kick out its process
		preempt_cpu = NULL;

		for (int i = 0; i < cpu_count; i++) {
			Process *curr_process = cpus[i].current_process;  
			
			if (curr_process == NULL 
				|| (min_process->remaining_time < curr_process->remaining_time 
				&& (preempt_cpu == NULL 
					|| preempt_cpu->current_process->priority < curr_process->priority))) {

				preempt_cpu = &cpus[i];

				// Perform preemption
				if (curr_process != NULL) {
					preempt_cpu->current_process->state = WAITING;	
				}
				min_process->state = RUNNING;
				preempt_cpu->current_process = min_process;

				if (min_process->start_time == -1) {
					min_process->start_time = current_time;
					min_process->response_time = current_time - min_process->arrival_time;
				}
				break;
			}
		}
	} while (preempt_cpu != NULL);
}

Process *tie_breaker(Process *p1, Process *p2) {
	if (p1->priority > p2->priority) {
		return p1;
	} else if (p2->priority > p1->priority) {
		return p2;
	} else if (p1->arrival_time < p2->arrival_time) {
		return p1;
	} else if (p2->arrival_time < p1->arrival_time) {
		return p2;
	} else {
		return p1;
	}
}

/**
 * Assign processes to idle CPUs based on the current scheduling algorithm
 */
void assign_processes_to_idle_cpus(Process *processes, int process_count, CPU *cpus, int cpu_count, 
                                Algorithm algorithm, ReadyQueue *ready_queue, int current_time) {
    // TODO: Implement process assignment to idle CPUs for all scheduling algorithms
    //
    // This function should:
    // 1. For each idle CPU (where current_process is NULL):
    //    a. Find the next process to run based on the scheduling algorithm:
    //       - FCFS: Select process with earliest arrival time that is waiting
    //       - RR: Take the next process from the ready_queue
    //       - SJF: Select waiting process with shortest burst_time
    //       - SRTF: Select waiting process with shortest remaining_time
    //    b. If multiple processes tie in selection criteria, prioritize by:
    //       - Higher priority value first
    //       - Earlier arrival time second
    // 2. For the selected process:
    //    - Change state to RUNNING
    //    - Assign it to the CPU
    //    - Reset its quantum_used (for RR)
    //    - Update start_time and response_time if this is first execution
    //
    // TODO Hint: Use a boolean array to track which processes are scheduled in this time step
    //       to avoid scheduling the same process on multiple CPUs

	bool scheduled[process_count];
	for (int i = 0; i < process_count; i++) {
		scheduled[i] = 0;
	}

	for (int i = 0; i < cpu_count; i++) {
		Process *new_process = NULL;  // try and find the next process to run

		if (cpus[i].current_process != NULL) {
			continue;  // we only care about idle CPUS
		}
		
		if (algorithm == RR) {
			// Round robin just runs the next process in the queue
			int idx = dequeue(ready_queue);

			if (idx > -1) {
				new_process = &processes[idx];	
				new_process->quantum_used = 0;
			} else {
				break;
			}
		} 

		// All other algorithms we need to check all processes
		for (int i = 0; i < process_count; i++) {
			if (scheduled[i] // NOTE do we need this?
				|| processes[i].state != WAITING 
				|| processes[i].arrival_time > current_time) {
				// we only want to look at unscheduled,  waiting processes that have arrived
			} else if (new_process == NULL) {
				new_process = &processes[i];
				continue;
			} else {
				switch (algorithm) {
				case FCFS:
					if (processes[i].arrival_time < new_process->arrival_time) {
						new_process = &processes[i];
					} else if (processes[i].arrival_time == new_process->arrival_time) {
						new_process = tie_breaker(new_process, &processes[i]);
					}
					break;
				case SJF:	
					if (processes[i].burst_time < new_process->burst_time) {
						new_process = &processes[i];
					} else if (processes[i].burst_time == new_process->burst_time) {
						new_process = tie_breaker(new_process, &processes[i]);
					}
					break; 							
				case SRTF:
					if (processes[i].remaining_time < new_process->remaining_time) {
						new_process = &processes[i];
					} else if (processes[i].remaining_time == new_process->remaining_time) {
						new_process = tie_breaker(new_process, &processes[i]);
					}
					break;
				case RR:  
					break;   // do nothing
				}
			}
		}

		if (new_process != NULL) {
			new_process->state = RUNNING;
			cpus[i].current_process = new_process;
			if (new_process->response_time == -1) {
				new_process->start_time = current_time;
				new_process->response_time = current_time - new_process->arrival_time;
			}
		}
	}
}

/**
 * Update waiting times for all waiting processes
 */
void update_waiting_times(Process *processes, int process_count, int current_time) {
    // DONE: Implement waiting time tracking
    //
    // This function should:
    // 1. Iterate through all processes
    // 2. For each process that:
    //    - Has arrived (arrival_time <= current_time), AND
    //    - Is not completed (state != COMPLETED), AND
    //    - Is not currently running on a CPU (state != RUNNING)
    //    Increment its waiting_time by 1
    //
    // Hint: Waiting time is used to calculate performance metrics
	
	for (int i = 0; i < process_count; i++) {
		if (processes[i].arrival_time <= current_time 
			&& processes[i].state != COMPLETED
			&& processes[i].state != RUNNING) {
			
			processes[i].waiting_time++;
		}
	}
}

/**
 * Execute processes on CPUs for the current time step
 */
// TODO process is not being set to completed correctly and process is not getting 
// kicked out when its done
void execute_processes(Process *processes, int process_count, CPU *cpus, int cpu_count, 
                     int current_time, int *completed_count) {
    // DONE: Implement CPU execution of processes for current time step
    //
    // This function should:
    // 1. For each CPU:
    //    a. If it has a process (current_process != NULL):
    //       - Decrease the process's remaining_time by 1
    //       - Increase the process's quantum_used by 1 (for RR)
    //       - Increment the CPU's busy_time
    //       - If remaining_time reaches 0:
    //         * Mark process as COMPLETED
    //         * Record finish_time as current_time + 1
    //         * Set CPU's current_process to NULL
    //         * Increment *completed_count
    //    b. If it has no process:
    //       - Increment the CPU's idle_time
    //
    // Note: processes and process_count parameters are not used in this implementation
    (void)processes;
    (void)process_count;

	for (int i = 0; i < cpu_count; i++) {
		Process *process = cpus[i].current_process;

		if (process != NULL) {
			process->remaining_time--;
			process->quantum_used++;  // only used by RR
			cpus[i].busy_time++;

			if (process->remaining_time <= 0) {
				process->state = COMPLETED;	
				process->finish_time = current_time + 1;
				cpus[i].current_process = NULL;
				(*completed_count)++;
			}
		} else {
			cpus[i].idle_time++;
		}
	}
}

/************************* MAIN SIMULATION *************************/

/**
 * Run the entire CPU scheduling simulation
 */
void simulate(Process *processes, int process_count, int cpu_count, Algorithm algorithm, int time_quantum) {
    // Initialize simulation components
    ReadyQueue ready_queue_rr; 
    init_queue(&ready_queue_rr);

    CPU *cpus = (CPU *)calloc(cpu_count, sizeof(CPU)); 
    if (!cpus) {
        perror("Failed to allocate CPUs");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < cpu_count; i++) cpus[i].id = i;

    int timeline_capacity = INITIAL_TIMELINE_CAPACITY;
    int **timeline = NULL;
    init_timeline(&timeline, timeline_capacity, cpu_count);

    int current_time = 0;
    int completed_count = 0;
    
    // Display simulation header
    printf("\nStarting simulation with %s on %d CPU(s)%s\n", 
           algorithm_name(algorithm),
           cpu_count, 
           algorithm == RR ? ", Quantum=" : "");
    if (algorithm == RR) printf("%d", time_quantum);
    printf("\n");

    // Main Simulation Loop
    while (completed_count < process_count) {
        // TODO: Implement the simulation main loop
        // 
        // This is the framework for the simulation loop. You need to:
        // 1. Handle new process arrivals via handle_arrivals()
        // 2. For Round Robin: enqueue newly arrived processes and handle quantum expiry
        // 3. For SRTF: check for preemptions
        // 4. Assign processes to idle CPUs based on the scheduling algorithm
        // 5. Update the timeline data structure
        // 6. Update waiting times
        // 7. Execute processes on CPUs
        // 8. Advance time

        // Handle new process arrivals
        int arrived_indices[MAX_PROCESSES];
        int arrival_count = 0;
        handle_arrivals(processes, process_count, current_time, algorithm, arrived_indices, &arrival_count);

        // Enqueue newly arrived processes for Round Robin
        if (algorithm == RR) {
            for (int i = 0; i < arrival_count; i++) {
                enqueue(&ready_queue_rr, arrived_indices[i]);
            }
            handle_rr_quantum_expiry(processes, cpus, cpu_count, time_quantum, &ready_queue_rr, current_time);
        }

        // Handle SRTF preemption
        if (algorithm == SRTF) {
            handle_srtf_preemption(processes, process_count, cpus, cpu_count, current_time);
        }

        // Assign processes to idle CPUs
        assign_processes_to_idle_cpus(processes, process_count, cpus, cpu_count, algorithm, 
                                   &ready_queue_rr, current_time);

        // Update timeline
        if (current_time >= timeline_capacity) {
            expand_timeline(&timeline, &timeline_capacity, timeline_capacity * 2, cpu_count);
        }
        for (int c = 0; c < cpu_count; c++) {
            timeline[current_time][c] = (cpus[c].current_process != NULL) ? cpus[c].current_process->pid : -1;
        }

        // Update waiting times for processes
        update_waiting_times(processes, process_count, current_time);

        // Execute processes on CPUs
        execute_processes(processes, process_count, cpus, cpu_count, current_time, &completed_count);

        // Advance time
        current_time++;

        // Safety break to prevent infinite loops
        if (current_time > timeline_capacity * 5 && completed_count < process_count) {
            fprintf(stderr, "Warning: Simulation exceeded maximum expected time. Aborting.\n");
            break;
        }
    }

    int total_time = current_time; // Record total simulation time
    print_results(processes, process_count, cpus, cpu_count, timeline, total_time);

    // Cleanup
    cleanup_timeline(timeline, timeline_capacity);
    free(cpus);
}

/************************* RESULTS DISPLAY *************************/

/**
 * Print the execution timeline visualization
 */
void print_timeline(int **timeline, int total_time, Process *processes, int process_count, int cpu_count) {
    printf("\nExecution Timeline:\n");
    int time_units_per_line = (TIMELINE_WIDTH - 5) / TIME_UNIT_WIDTH;
    if (time_units_per_line <= 0) time_units_per_line = 1; // Ensure at least 1 unit per line
    int time_segments = (total_time + time_units_per_line - 1) / time_units_per_line;

    // Print color key
    printf("\nColor Key:\n");
    for (int i = 0; i < process_count; i++) {
        printf("%sPID %-2d%s ", get_color_for_pid(processes[i].pid), processes[i].pid, COLOR_RESET);
        if ((i + 1) % 8 == 0 && i + 1 < process_count) printf("\n");
    }
    printf("\n");

    // Print timeline in segments
    for (int segment = 0; segment < time_segments; segment++) {
        int start_t = segment * time_units_per_line;
        int end_t = start_t + time_units_per_line;
        if (end_t > total_time) end_t = total_time;

        printf("\nTime %d to %d:\n", start_t, end_t - 1);

        // Time markers
        printf("Time: ");
        for (int t = start_t; t < end_t; t++) {
            printf("%-5d", t); // Print time marker for each unit
        }
        printf("\n");

        // CPU timelines
        for (int c = 0; c < cpu_count; c++) {
            printf("CPU%-2d ", c);
            for (int t = start_t; t < end_t; t++) {
                int pid = timeline[t][c];
                if (pid == -1) {
                    printf("%-*s", TIME_UNIT_WIDTH, "."); // Idle marker
                } else {
                    printf("%s%-*d%s", get_color_for_pid(pid), TIME_UNIT_WIDTH, pid, COLOR_RESET);
                }
            }
            printf("\n");
        }
    }
}

/**
 * Print detailed process statistics
 */
void print_process_stats(Process *processes, int process_count) {
    printf("\nProcess Statistics:\n");
    printf("%-6s %-7s %-7s %-7s %-7s %-7s %-7s %-7s\n",
           "PID", "Arrival", "Burst", "Start", "Finish", "Turn.", "Waiting", "Resp.");
    printf("----------------------------------------------------------------\n");

    for (int i = 0; i < process_count; i++) {
        Process *p = &processes[i];
        if (p->finish_time != -1) { // Only calculate for completed processes
            int turnaround = p->finish_time - p->arrival_time;
            int waiting = turnaround - p->burst_time;
            if (waiting < 0) waiting = 0; // Cannot be negative

            printf("%-6d %-7d %-7d %-7d %-7d %-7d %-7d %-7d\n",
                   p->pid, p->arrival_time, p->burst_time,
                   p->start_time, p->finish_time, turnaround, waiting, p->response_time);
        } else {
            printf("%-6d %-7d %-7d %-7s %-7s %-7s %-7s %-7s\n",
                   p->pid, p->arrival_time, p->burst_time,
                   (p->start_time == -1 ? "N/A" : "-"), "N/A", "N/A", "N/A",
                   (p->response_time == -1 ? "N/A" : "-"));
        }
    }
    printf("----------------------------------------------------------------\n");
}

/**
 * Print CPU usage statistics
 */
void print_cpu_stats(CPU *cpus, int cpu_count) {
    printf("\nCPU Statistics:\n");
    printf("%-6s %-9s %-9s %-12s\n", "CPU ID", "Busy Time", "Idle Time", "Utilization");
    printf("------------------------------------------\n");
    for (int i = 0; i < cpu_count; i++) {
        double utilization = 0.0;
        int cpu_total_time = cpus[i].busy_time + cpus[i].idle_time;
        if (cpu_total_time > 0) {
            utilization = 100.0 * cpus[i].busy_time / cpu_total_time;
        }
        printf("%-6d %-9d %-9d %-11.2f%%\n", cpus[i].id, cpus[i].busy_time, cpus[i].idle_time, utilization);
    }
    printf("------------------------------------------\n");
}

/**
 * Print average performance metrics
 */
void print_average_stats(Process *processes, int process_count) {
    double total_turnaround = 0.0, total_waiting = 0.0, total_response = 0.0;
    int valid_stats_count = 0;

    for (int i = 0; i < process_count; i++) {
        Process *p = &processes[i];
        if (p->finish_time != -1) { // Only calculate for completed processes
            int turnaround = p->finish_time - p->arrival_time;
            int waiting = turnaround - p->burst_time;
            if (waiting < 0) waiting = 0;

            total_turnaround += turnaround;
            total_waiting += waiting;
            total_response += p->response_time;
            valid_stats_count++;
        }
    }

    if (valid_stats_count > 0) {
        printf("\nAverage Statistics (for %d completed processes):\n", valid_stats_count);
        printf("  Average Turnaround Time: %.2f\n", total_turnaround / valid_stats_count);
        printf("  Average Waiting Time:    %.2f\n", total_waiting / valid_stats_count);
        printf("  Average Response Time:   %.2f\n", total_response / valid_stats_count);
    } else {
        printf("\nNo processes completed. Cannot calculate average statistics.\n");
    }
}

/**
 * Generate CSV output for automated testing
 */
void print_csv_output(Process *processes, int process_count, CPU *cpus, int cpu_count) {
    printf("\n\n--- CSV Output ---\n");
    
    // Process stats CSV
    printf("\nProcess Stats (CSV):\n");
    printf("PID,Arrival,Burst,Priority,Start,Finish,Turnaround,Waiting,Response\n");
    for (int i = 0; i < process_count; i++) {
        Process *p = &processes[i];
        if (p->finish_time != -1) {
            int turnaround = p->finish_time - p->arrival_time;
            int waiting = turnaround - p->burst_time;
            if (waiting < 0) waiting = 0;
            printf("%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                   p->pid, p->arrival_time, p->burst_time, p->priority,
                   p->start_time, p->finish_time, turnaround, waiting, p->response_time);
        } else {
             printf("%d,%d,%d,%d,%s,%s,%s,%s,%s\n",
                   p->pid, p->arrival_time, p->burst_time, p->priority,
                   "N/A", "N/A", "N/A", "N/A", "N/A");
        }
    }

    // CPU stats CSV
    printf("\nCPU Stats (CSV):\n");
    printf("CPU_ID,BusyTime,IdleTime,Utilization%%\n");
    for (int i = 0; i < cpu_count; i++) {
        double utilization = 0.0;
        int cpu_total_time = cpus[i].busy_time + cpus[i].idle_time;
        if (cpu_total_time > 0) {
            utilization = 100.0 * cpus[i].busy_time / cpu_total_time;
        }
        printf("%d,%d,%d,%.2f\n", cpus[i].id, cpus[i].busy_time, cpus[i].idle_time, utilization);
    }

    // Average stats CSV
    double total_turnaround = 0.0, total_waiting = 0.0, total_response = 0.0;
    int valid_stats_count = 0;
    for (int i = 0; i < process_count; i++) {
        Process *p = &processes[i];
        if (p->finish_time != -1) {
            int turnaround = p->finish_time - p->arrival_time;
            int waiting = turnaround - p->burst_time;
            if (waiting < 0) waiting = 0;
            
            total_turnaround += turnaround;
            total_waiting += waiting;
            total_response += p->response_time;
            valid_stats_count++;
        }
    }

    printf("\nAverage Stats (CSV):\n");
    printf("AvgTurnaround,AvgWaiting,AvgResponse\n");
    if (valid_stats_count > 0) {
        printf("%.2f,%.2f,%.2f\n",
               total_turnaround / valid_stats_count,
               total_waiting / valid_stats_count,
               total_response / valid_stats_count);
    } else {
        printf("N/A,N/A,N/A\n");
    }
    printf("--- End CSV Output ---\n");
}

/**
 * Display all simulation results
 */
void print_results(Process *processes, int process_count, CPU *cpus, int cpu_count, int **timeline, int total_time) {
    printf("\n--- Simulation Results ---\n");

    // Print visual timeline
    print_timeline(timeline, total_time, processes, process_count, cpu_count);
    print_process_stats(processes, process_count);
    print_cpu_stats(cpus, cpu_count);
    print_average_stats(processes, process_count);
    
    // Print CSV output for automated testing
    print_csv_output(processes, process_count, cpus, cpu_count);
}

/************************* MAIN FUNCTION *************************/

int main(int argc, char *argv[]) {
    Algorithm algorithm = FCFS;
    int cpu_count = 1;
    int time_quantum = DEFAULT_TIME_QUANTUM;
    char *input_file = NULL;

    // Parse command line arguments
    parse_arguments(argc, argv, &algorithm, &cpu_count, &time_quantum, &input_file);

    // Load processes
    Process *processes = NULL;
    int process_count = 0;
    load_processes(input_file, &processes, &process_count);

    // Run simulation if processes were loaded successfully
    if (process_count > 0) {
        simulate(processes, process_count, cpu_count, algorithm, time_quantum);
    } else {
        printf("No processes loaded or simulation not possible.\n");
    }

    // Clean up
    free(processes);
    return EXIT_SUCCESS;
}

/*
 * Thread Lab Assignment: Comparing Kernel Threads vs Simulated User-Level Tasks
 * with mutex synchronization for shared array access
 *
 * OBJECTIVES:
 * 1. Implement and use POSIX threads (pthreads)
 * 2. Use mutex locks for synchronizing access to shared data
 * 3. Compare performance between kernel-level threads and simulated user-level tasks
 */

#define _GNU_SOURCE

#include <sched.h>
#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
 
/* Configuration */
int NUM_WORKERS = 4;        // Default worker count (threads and tasks)
long ARRAY_SIZE = 10000000; // Default array size
#define WORK_SLICE 10000    // User task work slice size
#define LOCK_GRANULARITY 10 // Number of elements to process per lock acquisition
 
/* Global data */
int *global_array = NULL;
long long *kernel_thread_sums = NULL;
long long *user_task_sums = NULL;
pthread_mutex_t array_mutex = PTHREAD_MUTEX_INITIALIZER;
int mutex_contentions = 0;  // Counter for mutex contentions
 
/* Type definitions */
typedef struct {
    int thread_id;
    long start_index;
    long end_index;
} KernelThreadArgs;
 
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_DONE
} TaskState;
 
typedef struct {
    int task_id;
    TaskState state;
    long start_index;
    long end_index;
    long current_index;
    long long local_sum;
} UserTask;
 
UserTask *user_tasks = NULL;
int active_user_tasks = 0;
 
/* Helper functions */
long long get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}
 
void cleanup_resources(void) {
    free(global_array);
    free(kernel_thread_sums);
    free(user_task_sums);
    free(user_tasks);
    pthread_mutex_destroy(&array_mutex);
}
 
/* Set thread affinity to specific CPU core */
void set_thread_affinity(int core_id) {
    // Only attempt if we have at least as many cores as threads
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id % sysconf(_SC_NPROCESSORS_ONLN), &cpuset);
    
    pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpuset);
}
 
/* Kernel thread worker function with mutex synchronization */
void *kernel_thread_worker(void *arg) {
	/* Implement thread worker function
     * 1. Cast arg to KernelThreadArgs pointer and extract work assignment
     * 2. Initialize a local sum variable
     * 3. Set thread affinity based on thread_id
     * 4. Implement mutex-protected access to the global array
     *    (You decide the locking strategy - coarse or fine-grained)
     * 5. Calculate sum of array elements in the assigned range
     * 6. Store the result in kernel_thread_sums[thread_id]
     * 7. Return NULL
     */
     KernelThreadArgs *k_arg = (KernelThreadArgs *) arg;
	 int sum = 0;
	 set_thread_affinity(k_arg->thread_id);  // always run on the same CPU

	 for (int i = k_arg->start_index; i <= k_arg->end_index; i++) {
		 sum += global_array[i];
	 }
	 
	 kernel_thread_sums[k_arg->thread_id] = sum;  // safe because we know we have unique access to this memory address
     return NULL;
}
 
 /* Simulated user-level thread scheduler with mutex synchronization */
 void run_cooperative_scheduler(void) {
     /* Initialize tasks */
     user_tasks = malloc(NUM_WORKERS * sizeof(UserTask));
     if (!user_tasks) {
         perror("Failed to allocate memory for user tasks");
         return;
     }
     
     user_task_sums = calloc(NUM_WORKERS, sizeof(long long));
     if (!user_task_sums) {
         perror("Failed to allocate memory for user task sums");
         free(user_tasks);
         return;
     }
 
     /* Distribute work among tasks */
     long items_per_task = ARRAY_SIZE / NUM_WORKERS;
     for (int i = 0; i < NUM_WORKERS; ++i) {
         user_tasks[i].task_id = i;
         user_tasks[i].state = TASK_READY;
         user_tasks[i].start_index = i * items_per_task;
         user_tasks[i].end_index = (i == NUM_WORKERS - 1) ? ARRAY_SIZE : (i + 1) * items_per_task;
         user_tasks[i].current_index = user_tasks[i].start_index;
         user_tasks[i].local_sum = 0;
     }
     active_user_tasks = NUM_WORKERS;
 
     /* Run scheduler until all tasks complete */
     int current_task_idx = 0;
     while (active_user_tasks > 0) {
         if (NUM_WORKERS == 0) break;
         
         UserTask *task = &user_tasks[current_task_idx];
 
         if (task->state == TASK_READY || task->state == TASK_RUNNING) {
             task->state = TASK_RUNNING;
             
             /* Execute a slice of work */
             int work_done = 0;
             while (work_done < WORK_SLICE && task->current_index < task->end_index) {
                 // Calculate chunk end (with boundary check)
                 long chunk_end = task->current_index + LOCK_GRANULARITY;
                 if (chunk_end > task->end_index)
                     chunk_end = task->end_index;
                 
                 // Lock mutex for this chunk
                 pthread_mutex_lock(&array_mutex);
                 
                 // Process chunk
                 for (long j = task->current_index; j < chunk_end; j++) {
                     task->local_sum += global_array[j];
                     work_done++;
                 }
                 
                 // Update current index
                 task->current_index = chunk_end;
                 
                 // Release mutex
                 pthread_mutex_unlock(&array_mutex);
             }
 
             // Add artificial context switch delay for tasks
             usleep(1);  // Small sleep to simulate context switch overhead
 
             /* Check if task completed */
             if (task->current_index >= task->end_index) {
                 task->state = TASK_DONE;
                 user_task_sums[task->task_id] = task->local_sum;
                 active_user_tasks--;
             } else {
                 task->state = TASK_READY;
             }
         }
         
         /* Move to next task (round-robin) */
         current_task_idx = (current_task_idx + 1) % NUM_WORKERS;
     }
 }
 
 int main(int argc, char *argv[]) {
     /* Parse command line arguments */
     if (argc != 3) {
         fprintf(stderr, "Usage: %s <num_workers> <array_size>\n", argv[0]);
         fprintf(stderr, "  num_workers: Number of threads/tasks (e.g., 4)\n");
         fprintf(stderr, "  array_size: Elements in array (e.g., 10000000)\n");
         return EXIT_FAILURE;
     }
 
     /* Parse num_workers */
     char *endptr;
     errno = 0;
     long val = strtol(argv[1], &endptr, 10);
     if (errno != 0 || *endptr != '\0' || val <= 0 || val > INT_MAX) {
         fprintf(stderr, "Error: Invalid number of workers '%s'\n", argv[1]);
         return EXIT_FAILURE;
     }
     NUM_WORKERS = (int)val;
 
     /* Parse array_size */
     errno = 0;
     val = strtol(argv[2], &endptr, 10);
     if (errno != 0 || *endptr != '\0' || val <= 0) {
         fprintf(stderr, "Error: Invalid array size '%s'\n", argv[2]);
         return EXIT_FAILURE;
     }
     ARRAY_SIZE = val;
 
     /* Display configuration */
     printf("Configuration:\n");
     printf("  Workers: %d\n", NUM_WORKERS);
     printf("  Array Size: %ld\n", ARRAY_SIZE);
     printf("  Work Slice: %d\n", WORK_SLICE);
     printf("  Lock Granularity: %d elements\n", LOCK_GRANULARITY);
     printf("----------------------------------------\n");
 
     /* Initialize array */
     printf("Initializing array...\n");
     global_array = malloc(ARRAY_SIZE * sizeof(int));
     if (!global_array) {
         perror("Failed to allocate memory for array");
         return EXIT_FAILURE;
     }
     
     for (long i = 0; i < ARRAY_SIZE; ++i) {
         global_array[i] = i % 10;
     }
     printf("Initialization complete.\n");
     printf("----------------------------------------\n");

     /* Initialize mutex 
      * Initialize the mutex here using pthread_mutex_init
      */
	 // pthread_mutex_init(&array_mutex, NULL);

     /* Kernel thread benchmark */
     printf("Running Kernel Thread Benchmark (%d threads) with mutex synchronization...\n", NUM_WORKERS);
     long long start_time = get_time_us();
 
     /* Implement thread creation and execution
      * 1. Allocate memory for thread handles, arguments, and results
      * 2. Create threads using pthread_create and provide them with work assignments
      * 3. Wait for all threads to complete using pthread_join
      * 4. Calculate the total sum from all threads
      */
	 // 1
	 pthread_t threads[NUM_WORKERS];
	 KernelThreadArgs args[NUM_WORKERS];
 	 kernel_thread_sums = (long long *)malloc(sizeof(long long) * NUM_WORKERS);
	 int stride = ARRAY_SIZE / NUM_WORKERS;
	 int remainder = ARRAY_SIZE % NUM_WORKERS;
	 size_t start_idx = 0, end_idx = 0;
	
	 printf("STRIDE: %d\n", stride);
	 printf("REMAINDER: %d\n", remainder);

	 // 2
	 for (int i = 0; i < NUM_WORKERS; i++) {
		 // assign work (mutually exclusive array slice) to thread
		 end_idx = start_idx + stride - 1;

		 if (remainder) {
			 end_idx++;  
			 remainder--;
		 }

		 args[i].thread_id = i;
		 args[i].start_index = start_idx;
		 args[i].end_index = end_idx;

		 start_idx = end_idx + 1;

		 // start thread
	 	 pthread_create(&threads[i], NULL, kernel_thread_worker, (void *)&args[i]);
	 }

	 // 3 and 4
     long long total_kernel_sum = 0;
	 for (int i = 0; i < NUM_WORKERS; i++) {
		 pthread_join(threads[i], NULL);
		 total_kernel_sum += kernel_thread_sums[i];
	 }

     long long kernel_duration = get_time_us() - start_time;
     printf("Kernel Thread Time: %lld microseconds\n", kernel_duration);
     printf("Kernel Thread Sum: %lld\n", total_kernel_sum);
 
     printf("----------------------------------------\n");

     /* User task benchmark */
     printf("Running User-Level Task Benchmark (%d tasks) with mutex synchronization...\n", NUM_WORKERS);
     start_time = get_time_us();
 
     run_cooperative_scheduler();
 
     long long total_user_sum = 0;
     if (user_task_sums) {
         for (int i = 0; i < NUM_WORKERS; ++i) {
             total_user_sum += user_task_sums[i];
         }
     } else {
         fprintf(stderr, "User task benchmark failed (memory allocation error)\n");
     }
 
     long long user_duration = get_time_us() - start_time;
     printf("User Task Time: %lld microseconds\n", user_duration);
     printf("User Task Sum: %lld\n", total_user_sum);
     printf("----------------------------------------\n");
 
    /* Compare results */
    printf("\nResults Comparison (with mutex synchronization):\n");
    if (kernel_duration > 0 && user_duration > 0) {
        double speedup = (double)user_duration / kernel_duration;
        printf("Kernel threads %s user tasks by %.2fx\n", 
               (speedup > 1.0) ? "faster than" : "slower than",
               (speedup > 1.0) ? speedup : 1.0/speedup);
    }
    
    if (total_kernel_sum == total_user_sum) {
        printf("Sums match: %lld\n", total_kernel_sum);
    } else {
        printf("Warning: sums don't match! Kernel: %lld, User: %lld\n", 
               total_kernel_sum, total_user_sum);
    }

    /* Cleanup */
    cleanup_resources();
 
     return EXIT_SUCCESS;
 }

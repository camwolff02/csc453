#include<stdio.h>
#include<stdlib.h>
#include<time.h>
#include<unistd.h>
#include<string.h>
// Recursive function to consume stack space
void use_stack(int depth, int array_size) {
	// Base case to prevent infinite recursion
	if (depth <= 0) return;
	// Allocate array on stack
	char stack_array[array_size];
	// Fill array to ensure it's actually used (prevents optimization)
	memset(stack_array, 1, array_size);
	// Recurse to next level
	use_stack(depth - 1, array_size);
}

int main(int argc, char* argv[]) {
	// Check if we have enough arguments
	if (argc < 3) {
		printf("Usage: %s <memory_in_MB> <seconds_to_run> [stack_depth_in_KB]\n", argv[0]);
		printf("Example: %s 100 10 50 (use 100MB heap, run for 10 seconds, use ~50KB stack)\n", argv[0]);
		return 1;
	}
	printf("Current Process ID = %d\n", getpid());
	// Convert and validate first argument (memory size)
	long long int size;
	char *endptr;
	size = strtoll(argv[1], &endptr, 10);
	if (*endptr != '\0' || size <= 0) {
		printf("Error: Invalid memory size '%s'. Please provide a positive integer.\n", argv[1]);
		return 1;
	}
	size *= 1024 * 1024; // Convert to bytes
	// Allocate memory and check if allocation was successful
	int* buffer = (int*)malloc(size);
	if (buffer == NULL) {
		printf("Error: Failed to allocate %lld bytes of memory.\n", size);
		return 1;
	}
	// Convert and validate second argument (seconds)
	time_t endwait, seconds, start;
	seconds = strtol(argv[2], &endptr, 10);
	if (*endptr != '\0' || seconds <= 0) {
		printf("Error: Invalid time value '%s'. Please provide a positive integer.\n", argv[2]);
		free(buffer); // Free memory before exiting
	return 1;
	}
	// Handle optional stack usage parameter
	int stack_kb = 0;
	if (argc > 3) {
		stack_kb = strtol(argv[3], &endptr, 10);
		if (*endptr != '\0' || stack_kb < 0) {
			printf("Error: Invalid stack size '%s'. Please provide a non-negative integer.\n", argv[3]);
			free(buffer);
			return 1;
		}
		// Limit stack usage to prevent crashes
		if (stack_kb > 1000) {
			printf("Warning: Requested stack size is very large. Limiting to 1000KB.\n");
			stack_kb = 1000;
		}
		if (stack_kb > 0) {
			printf("Using approximately %dKB of stack space...\n", stack_kb);
			// Use 1KB per recursion level with a small array
			int array_size = 1024; // 1KB per level
			int depth = stack_kb;
			use_stack(depth, array_size);
			printf("Stack usage complete.\n");
		}
	}
	start = time(NULL);
	endwait = start + seconds;
	while (start < endwait) {
		printf(".");
		fflush(stdout);
		size_t elements = (size_t)(size / sizeof(int));
		for (size_t i = 0; i < elements; i++) {
			buffer[i] = (int)i;
		}
		start = time(NULL);
	}
	printf("(done)\n");
	free(buffer); // Free allocated memory
	return 0;
}

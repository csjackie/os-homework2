#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstring>
#include <signal.h>
#include <vector>
#include <algorithm>

/*
 * SimulatedClock structure
 * This is the shared memory structure accessed by OSS and all workers.
 * It represents a logical clock.
 */
struct SimulatedClock {
        unsigned int seconds;
        unsigned int nanoseconds;
};

/* 
 * PCB (Process Control Block)
 * Stores minimal bookkeeping information for each worker process.
 */
struct PCB {
	// Process ID of worker
        pid_t pid;
	// Time worker started
        unsigned int startSec;
        unsigned int startNano;
};

// Track active child PIDs
std::vector<pid_t> childPids;
// Shared memory ID
int shmid_global;
// Pointer to shared memory
SimulatedClock* clock_global = nullptr;

/*
 * Signal handler for 60 second real time timeout.
 * If OSS runs too long: all children are terminated, shared 
 * memory is cleaned up, and program exits.
 */
void signal_handler(int sig) {

	std::cerr << "\nOSS: Timeout reached. Killing children...\n";
	
	// Send SIGTERM to all active children
	for (pid_t pid : childPids) {
		kill(pid, SIGTERM);
	}

	// Detach shared memory
	if (clock_global)
		shmdt(clock_global);

	// Remove shared memory segment
	if (shmid_global > 0)
		shmctl(shmid_global, IPC_RMID, NULL);
	
	exit(1);
}

int main(int argc, char **argv) {

	// Default values for command line parameters
        int n = 1;
	int s = 1;
	float t = 1;
	float i = 1;
	
	int opt;
	// Currently running children
	int current = 0;
	// Total children ever launched
	int totalLaunched = 0;
	// Set up real time safety timeout (60 seconds)
	signal(SIGALRM, signal_handler);
	alarm(60);

	// Parse command line options
	while ((opt = getopt(argc, argv, "hn:s:t:i:")) != -1) {
		switch (opt) {
			case 'h':
				std::cout << "To run program:\n\t ./oss -n # -s # -t # -i #\n";
				std::cout << "Replace # with an integer for n and s, and a float of t and i.\n";
				return 0;
			// Total number of children to launch
			case 'n': n = atoi(optarg); break;
			// Maximum simultaneous children 
			case 's': s = atoi(optarg); break;
			// Simulated time limit
			case 't': t = atof(optarg); break;
			// Minimum interval between launches
			case 'i': i = atof(optarg); break;
		}
	}

	// prints error message and exits program if the value of n, s, or t are out of range
	if (n <= 0 || n > 100 || s <= 0 || s > 15 || t <= 0 || i < 0 || s > n) {
		std::cout << "Invalid argument values\n";
		exit(1);
	}

	// Shared memory setup
	
	// Generate unique key
	key_t key = ftok(".", 'x');
	int shmid = shmget(key, sizeof(SimulatedClock), IPC_CREAT | 0666);
	
	if (shmid < 0) {
		perror("shmget failed");
		exit(1);
	}

	SimulatedClock* clock = (SimulatedClock*) shmat(shmid, NULL, 0);
	
	if (clock == (void*) -1) {
		perror("shmat failed");
		exit(1);
	}
	
	// Save globals for cleanup
	shmid_global = shmid;
	clock_global = clock;

	// Initialize simulated clock
	clock->seconds = 0;
	clock->nanoseconds = 0;
	
	// Process table to track active workers
	PCB processTable[100] = {0};

	// Track launch time
	unsigned int lastLaunchSec = 0;
	unsigned int lastLaunchNano = 0;
	// Track last time process table was printed
	unsigned int lastPrintSec = 0;
        unsigned int lastPrintNano = 0;
	// Convert time limit into seconds + nanoseconds
	unsigned int timeLimitSec = (unsigned int)t;
	unsigned int timeLimitNano = (t - timeLimitSec) * 1000000000;
	
	int status;
	bool stopLaunching = false;

	// Main scheduling loop
	while (!stopLaunching || current > 0) {
	
		// Increase simulated clock by 10ms
		clock->nanoseconds += 10000000;
		
		if (clock->nanoseconds >= 1000000000) {
			clock->seconds++;
			clock->nanoseconds -= 1000000000;
		}
		
		// Print process table every half a second
		unsigned int elapsedSec = clock->seconds - lastPrintSec;
		unsigned int elapsedNano = (clock->nanoseconds >= lastPrintNano) ? clock->nanoseconds -
			lastPrintNano : 1000000000 + clock->nanoseconds - lastPrintNano;
		
		if (elapsedSec > 0 || elapsedNano >= 500000000) {
			std::cout << "\nOSS Table at " << clock->seconds << ":"
				<< clock->nanoseconds << "\n";

			for (int j = 0; j < totalLaunched; j++) {
				if (processTable[j].pid != 0) {
					std::cout << "PID: " << processTable[j].pid << " Start: "
						<< processTable[j].startSec << ":"
						<< processTable[j].startNano << "\n";
				}
			}

			lastPrintSec = clock->seconds;
			lastPrintNano = clock->nanoseconds;
		}

		// Stop launching if simulated time exceeded
		if (clock->seconds > timeLimitSec ||
				(clock->seconds == timeLimitSec && 
				 clock->nanoseconds >= timeLimitNano)){
			
			stopLaunching = true;
		}
				
		// Check for terminated children
		pid_t pid = waitpid(-1, &status, WNOHANG);

		while (pid > 0) {
			current--;
		
			// Clear PCB entry	
			for (int j = 0; j < totalLaunched; j++) {
				if (processTable[j].pid == pid) {
					processTable[j].pid = 0;
					break;
				}
			}
			// Remove from active PID list
			childPids.erase(std::remove(childPids.begin(), childPids.end(), pid), childPids.end());

			pid = waitpid(-1, &status, WNOHANG);
		}

		// Determine if minimum launch interval has passed
		unsigned int intervalSec = (unsigned int)i;
		unsigned int intervalNano = (i - intervalSec) * 1000000000;

		unsigned int elapsedLaunchSec = clock->seconds - lastLaunchSec;
		unsigned int elapsedLaunchNano;
		if (clock->nanoseconds >= lastLaunchNano) {
			elapsedLaunchNano = clock->nanoseconds - lastLaunchNano;
		} else {
			elapsedLaunchSec--;
			elapsedLaunchNano = 1000000000 + clock->nanoseconds - lastLaunchNano;
		}

		bool intervalPassed =
			(elapsedLaunchSec > intervalSec) ||
			(elapsedLaunchSec == intervalSec && elapsedNano >= intervalNano);

		// Launch new worker if allowed
		if (!stopLaunching && current < s && totalLaunched < n && intervalPassed) {

			pid_t childPid = fork();
		
			if (childPid == 0) {
				execl("./worker", "worker", "1", "0", NULL);
				perror("execl failed");
				exit(1);
			}

			if (childPid > 0) {
				childPids.push_back(childPid);
				current++;
				totalLaunched++;
				
				// Find empty PCB slot
				for (int j = 0; j < 100; j++) {
					if (processTable[j].pid == 0) {
						processTable[j].pid = childPid;
						processTable[j].startSec = clock->seconds;
						processTable[j].startNano = clock->nanoseconds;
						break;
					}
				}
				
				lastLaunchSec = clock->seconds;
				lastLaunchNano = clock->nanoseconds;
			}
		}
	}

	// Wait for any remaining children
	while (wait(NULL) > 0);

	std::cout << "OSS terminating\n";
	std::cout << totalLaunched << " workers were launched\n";

	// Cleanup shared memory
	shmdt(clock);
	shmctl(shmid, IPC_RMID, NULL);

	return 0;	
}

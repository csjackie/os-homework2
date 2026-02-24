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

struct SimulatedClock {
        unsigned int seconds;
        unsigned int nanoseconds;
};

struct PCB {
        pid_t pid;
        unsigned int startSec;
        unsigned int startNano;
};

std::vector<pid_t> childPids;
int shmid_global;
SimulatedClock* clock_global = nullptr;

void signal_handler(int sig) {

	std::cerr << "\nOSS: Timeout reached. Killing children...\n";
	
	for (pid_t pid : childPids) {
		kill(pid, SIGTERM);
	}

	if (clock_global)
		shmdt(clock_global);

	if (shmid_global > 0)
		shmctl(shmid_global, IPC_RMID, NULL);
	
	exit(1);
}

int main(int argc, char **argv) {

        int n = 1;
	int s = 1;
	float t = 1;
	float i = 1;
	
	int opt;
	int current = 0;
	int total = 0;
	int totalLaunched = 0;
	
	signal(SIGALRM, signal_handler);
	alarm(60);

	while ((opt = getopt(argc, argv, "hn:s:t:i:")) != -1) {
		switch (opt) {
			case 'h':
				std::cout << "To run program:\n\t ./oss -n # -s # -t # -i #\n";
				std::cout << "Replace # with an integer for n and s, and a float of t and i.\n";
				return 0;
			case 'n': n = atoi(optarg); break;
			case 's': s = atoi(optarg); break;
			case 't': t = atof(optarg); break;
			case 'i': i = atof(optarg); break;
		}
	}

	// prints error message and exits program if the value of n, s, or t are out of range
	if (n <= 0 || n > 100 || s <= 0 || s > 15 || t <= 0 || i < 0 || s > n) {
		std::cout << "Invalid argument values\n";
		exit(1);
	}

	// Shared memory setup
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
	
	shmid_global = shmid;
	clock_global = clock;

	clock->seconds = 0;
	clock->nanoseconds = 0;
	
	PCB processTable[100] = {0};

	unsigned int lastLaunchSec = 0;
	unsigned int lastLaunchNano = 0;
	unsigned int lastPrintSec = 0;
        unsigned int lastPrintNano = 0;

	unsigned int timeLimitSec = (unsigned int)t;
	unsigned int timeLimitNano = (t - timeLimitSec) * 1000000000;
	
	int status;
	bool stopLaunching = false;

	while (!stopLaunching || current > 0) {
	
		// Increment simulated clock
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

			for (int j = 0; j < total; j++) {
				if (processTable[j].pid != 0) {
					std::cout << "PID: " << processTable[j].pid << " Start: "
						<< processTable[j].startSec << ":"
						<< processTable[j].startNano << "\n";
				}
			}

			lastPrintSec = clock->seconds;
			lastPrintNano = clock->nanoseconds;
		}

		// Check simulated time
		if (clock->seconds > timeLimitSec ||
				(clock->seconds == timeLimitSec && 
				 clock->nanoseconds >= timeLimitNano)){
			
			stopLaunching = true;
		}
				
		// check terminated children
		pid_t pid = waitpid(-1, &status, WNOHANG);
		while (pid > 0) {
			current--;
			
			for (int j = 0; j < total; j++) {
				if (processTable[j].pid == pid) {
					processTable[j].pid = 0;
					break;
				}
			}
			childPids.erase(std::remove(childPids.begin(), childPids.end(), pid), childPids.end());

			pid = waitpid(-1, &status, WNOHANG);
		}

		// check launch interval
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

		// Launch new child
		if (!stopLaunching && current < s && total < n && intervalPassed) {

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
				
				int slot = -1;
				
				for (int j = 0; j < 100; j++) {
					if (processTable[j].pid == 0) {
						slot = j;
						break;
					}
				}
				
				if (slot != -1) {
					processTable[slot].pid = childPid;
					processTable[slot].startSec = clock->seconds;
					processTable[slot].startNano = clock->nanoseconds;
				}

				lastLaunchSec = clock->seconds;
				lastLaunchNano = clock->nanoseconds;
			}
		}
	}

	// wait for remaining children
	while (wait(NULL) > 0);

	std::cout << "OSS terminating\n";
	std::cout << totalLaunched << " workers were launched\n";

	shmdt(clock);
	shmctl(shmid, IPC_RMID, NULL);

	return 0;	
}

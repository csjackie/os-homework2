#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <cstring>

struct SimulatedClock {
	unsigned int seconds;
	unsigned int nanoseconds;
};

int main(int argc, char **argv) {

	// sets default of n, s, t, and i to 1 if value not given
        int n = 1;
	int s = 1;
	float t = 1;
	float i = 1;
	
	// initializing opt
	int opt;
	// accumulator to track current child processes being completed, set at 0
	int current = 0;
	// accumulator to track the total number of child processes launched, set at 0
	int total = 0;

	// getopt(3) parses options
	while ((opt = getopt(argc, argv, "hn:s:t:i:")) != -1) {
		switch (opt) {
			// outputs a help message explaining how to run the program, then exits
			case 'h':
				std::cout << "To run program:\n\t ./oss -n # -s # -t # -i #\n";
				std::cout << "Replace # with an integer for n and s, and a float of t and i.\n";
				return 0;
			// number of total child processes
			case 'n':
				// converts the string input for n to int and assigns it to n
				n = atoi(optarg);
				break;
			// number of allowed simultaneous child processes running at one time
			case 's':
				// converts the string input for s to int and assigns it to s
				s = atoi(optarg);
				break;
			// amount of simulated time
			case 't':
				// converts the string input for t to int and assigns it to t
				t = atof(optarg);
				break;
			// minimum interval between launching child processes
			case 'i':
				// converts the float input for i to float and assigns it to i
				i = atof(optarg);
				break;
		}
	}

	// prints error message and exits program if the value of n, s, or t are out of range
	if (n <= 0 || n > 100) {
		std::cout << "Invalid value for n\n";
		exit(1);
	}

	if (s <= 0 || s > 15) {
		std::cout << "Invalid value for s\n";
		exit(1);
	}

	if (t <= 0){
		std::cout << "Invalid value for t\n";
		exit(1);
	}

	if (i < 0) {
		std::cout << "Invalid value for i\n";
		exit(1);
	}
	if (s > n){
		exit(1);
	}

	// Shared memory setup
	key_t key = ftok(".", 'x');
	int shmid = shmget(key, sizeof(SimulatedClock), IPC_CREAT | 0666);
	SimulatedClock* clock = (SimulatedClock*) shmat(shmid, NULL, 0);

	clock->seconds = 0;
	clock->nanoseconds = 0;

	unsigned int lastLaunchSec = 0;
	unsigned int lastLaunchNano = 0;

	unsigned int timeLimitSec = (unsigned int)t;
	unsigned int timeLimitNano = (t - timeLimitSec) * 1000000000;

	int status;
	
	bool stopLaunching = false;

	while (total < n || current > 0) {
		//incrementClock();
		clock->nanoseconds += 10000000;
		
		if (clock->nanoseconds >= 1000000000) {
			clock->seconds++;
			clock->nanoseconds -= 100000000;
		}
		// check simulated time limit
		if (clock->seconds > timeLimitSec ||
				(clock->seconds == timeLimitSec &&
				 clock->nanoseconds >= timeLimitNano)){
			stopLaunching = true;
		}
				
		// check terminated children
		pid_t pid = waitpid(-1, &status, WNOHANG);
		if (pid > 0) {
			current--;
			// clear PCB entry
		}
		// check launch interval
		unsigned int intervalSec = (unsigned int)i;
		unsigned int intervalNano = (i - intervalSec) * 1000000000;

		unsigned int elapsedSec = clock->seconds - lastLaunchSec;
		unsigned int elapsedNano;

		if (clock->nanoseconds >= lastLaunchNano) {
			elapsedNano = clock->nanoseconds - lastLaunchNano;
		} else {
			elapsedSec--;
			elapsedNano = 1000000000 + clock->nanoseconds - lastLaunchNano;
		}
		bool intervalPassed =
			(elapsedSec > intervalSec) ||
			(elapsedSec == intervalSec && elapsedNano >= intervalNano);

		// possibly launch new child
		if (!stopLaunching && current < s && total < n && intervalPassed) {

			pid_t childPid = fork();

			if (childPid == 0) {
				char secStr[10];
				char nanoStr[10];

				sprintf(secStr, "%d", 1);
				sprintf(nanoStr, "%d", 0);

				execl("./worker", "worker", secStr, nanoStr, NULL);
				exit(1);
			}

			if (childPid > 0) {
				current++;
				total++;

				lastLaunchSec = clock->seconds;
				lastLaunchNano = clock->nanoseconds;
			}
		}
	}
	// wait for remaining children
	while (wait(NULL) > 0);

	std::cout << "OSS terminating\n";
	std::cout << total << " workers were launched\n";

	shmdt(clock);
	shmctl(shmid, IPC_RMID, NULL);

	return 0;	
}

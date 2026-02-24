#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>

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
	
	int status;

	key_t key = ftok(".", 'x');
	int shmid = shmget(key, sizeof(SimulatedClock), IPC_CREAT | 0666);
	
	SimulatedClock* clock = (SimulatedClock*) shmat(shmid, NULL, 0);

	clock->seconds = 0;
	clock->nanoseconds = 0;

	// getopt(3) parses options
	while ((opt = getopt(argc, argv, "hn:s:t:i:")) != -1) {
		switch (opt) {
			// outputs a help message explaining how to run the program, then exits
			case 'h':
				std::cout << "To run program:\n\t ./oss -n # -s # -t # -i #\n";
				std::cout << "Replace # with an integer for n and s, and a float of t and i.\n";
				exit(0);
				break;
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

	if (s > n){
		exit(1);
	}
}

// launch system clock


	// while loop launches as many child processes as it can, ensuring the number of current running 
	// child processes are less than the allowed simultaneous number of processes, and that the total 
	// number of processes complete is less than the number of total child processes needed
	while (total < n || current > 0) {
		//incrementClock();
		clock->nanoseconds += 10000000;
		
		if (clock->nanoseconds >= 100000000) {
			clock->seconds++;
			clock->nanoseconds -= 100000000;
		}
				
		// check terminated children
		pid_t pid = waitpid(-1, &status, WNOHANG);
		if (pid > 0) {
			current--;
			// clear PCB entry
		}

		// possibly launch new child
		if (current < s && total < n && inter

		// maybe print process table
	}
	
}
return 0;

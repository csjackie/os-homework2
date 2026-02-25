#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>

/*
 * Structure representing the simulated system clock.
 */
struct SimulatedClock {
	unsigned int seconds;
	unsigned int nanoseconds;
};

int main(int argc, char *argv[]) {

	// Ensure the worker was called with the correct arguments.
	// The worker takes two arguments: argv[1] and argv[2].
	if (argc < 3) {
		std::cerr << "Not enough arguments\n";
		return 1;
	}

	// Convert the arguments from strings to integers.
	int addSec = atoi(argv[1]);
	int addNano = atoi(argv[2]);

	// Display process id information.
	// PID = process ID of this worker
	// PPID = parent process ID (oss)
	std::cout << "Worker starting, PID:" << getpid()
              << " PPID:" << getppid() << std::endl;
	std::cout << "Called with:\n";
    	std::cout << "Interval: " << addSec
              << " seconds, " << addNano
              << " nanoseconds\n";

	// Generates shared memory key.
	key_t key = ftok(".", 'x');
	// Shared memory ID.
	int shmid = shmget(key, sizeof(SimulatedClock), 0666);

	// Attach the shared memory segment to this process and treat it as 
	// a simulatedClock pointer.
	SimulatedClock* clock = (SimulatedClock*) shmat(shmid, NULL, 0);

	// Compute the worker's termination time.
	unsigned int termSec = clock->seconds + addSec;
	unsigned int termNano = clock->nanoseconds + addNano;

	// Handle nanosecond overflow
	if (termNano >= 1000000000) {
		termSec++;
		termNano -= 1000000000;
	}

	// Print initial system clock state and calculated termination time.
	std::cout << "WORKER PID:" << getpid()
                << " PPID:" << getppid() << std::endl;
	std::cout << "SysClockS: " << clock->seconds 
		<< " SysClockNano: " << clock->nanoseconds 
		<< " TermTimeS: " << termSec 
		<< " TermTimeNano: " << termNano 
		<< "\n--Just Starting\n";

	// Track the start time and last time output printed.
	unsigned int startSec = clock->seconds;
	unsigned int lastPrintedSec = startSec;

	while(true) {

		// Check if termination time reached
		if (clock->seconds > termSec ||
			(clock->seconds == termSec &&
			 clock->nanoseconds >= termNano)) {
			// If termination time reached, print termination message and exit.
			std::cout << "WORKER PID:" << getpid()
				<< " PPID:" << getppid() << std::endl;
			std::cout << "SysClockS: " << clock->seconds
				<< " SysClockNano: " << clock->nanoseconds
				<< " TermTimeS: " << termSec
				<< " TermTimeNano: " << termNano
				<< "\n--Terminating\n";
			break;
		}
		// Print message each one full simulated second has passed.
		if (clock->seconds > lastPrintedSec) {
			
			std::cout << "WORKER PID:" << getpid()
				<< " PPID:" << getppid() << std::endl;
			std::cout << "SysClockS: " << clock->seconds
				<< " SysClockNano: " << clock->nanoseconds
				<< " TermTimeS: " << termSec
				<< " TermTimeNano: " << termNano
				<< "\n--"
				<< (clock->seconds - startSec)
				<< " seconds have passed since starting\n";
			
			lastPrintedSec = clock->seconds;
		}
	}

	// Detach from shared memory before exiting.
	shmdt(clock);

	return 0;
}

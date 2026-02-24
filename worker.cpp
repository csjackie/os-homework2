#include <iostream>
#include <unistd.h>
#include <cstdlib>
#include <sys/ipc.h>
#include <sys/shm.h>

struct SimulatedClock {
	unsigned int seconds;
	unsigned int nanoseconds;
};

int main(int argc, char *argv[]) {

	if (argc < 3) {
		std::cerr << "Not enough arguments\n";
		return 1;
	}

	int addSec = atoi(argv[1]);
	int addNano = atoi(argv[2]);

	std::cout << "Worker starting, PID:" << getpid()
              << " PPID:" << getppid() << std::endl;
	std::cout << "Called with:\n";
    	std::cout << "Interval: " << addSec
              << " seconds, " << addNano
              << " nanoseconds\n";

	// Attach to shared memory
	key_t key = ftok(".", 'x');
	int shmid = shmget(key, sizeof(SimulatedClock), 0666);

	SimulatedClock* clock = (SimulatedClock*) shmat(shmid, NULL, 0);

	// Calculate termination time
	unsigned int termSec = clock->seconds + addSec;
	unsigned int termNano = clock->nanoseconds + addNano;

	if (termNano >= 1000000000) {
		termSec++;
		termNano -= 1000000000;
	}

	std::cout << "WORKER PID:" << getpid()
                << " PPID:" << getppid() << std::endl;
	std::cout << "SysClockS: " << clock->seconds 
		<< " SysClockNano: " << clock->nanoseconds 
		<< " TermTimeS: " << termSec 
		<< " TermTimeNano: " << termNano 
		<< "\n--Just Starting\n";

	unsigned int startSec = clock->seconds;
	unsigned int lastPrintedSec = startSec;

	while(true) {

		// Check if termination time reached
		if (clock->seconds > termSec ||
			(clock->seconds == termSec &&
			 clock->nanoseconds >= termNano)) {

			std::cout << "WORKER PID:" << getpid()
				<< " PPID:" << getppid() << std::endl;
			std::cout << "SysClockS: " << clock->seconds
				<< " SysClockNano: " << clock->nanoseconds
				<< " TermTimeS: " << termSec
				<< " TermTimeNano: " << termNano
				<< "\n--Terminating\n";
			break;
		}
		// Print every second change
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

	shmdt(clock);
	return 0;
}

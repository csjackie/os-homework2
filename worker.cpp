#include <iostream>
#include <unistd.h>
#include <cstdlib>

/*
 * This function displays its PID and PPID, as well as what it was called with.
 */

int main(int argc, char *argv[]) {
	// converts string argv to int t
	int t = atoi(argv[1]);
	// for loop starts at 1 and continues to iterate through loop until it reaches the number t
	for (int i = 1; i <= t; i++) {
		// outputs the child pid, the parent pid, and the number of iteration ran before sleeping
		std::cout << "WORKER PID: " << getpid() << " PPID: " << getppid();
		std::cout << "Called with: " << " ";
		//  
	}
	return 0;
}


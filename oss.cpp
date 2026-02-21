#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>
#include <stdio.h>

int main(int argc, char **argv) {
	
	// sets default of n, s, and t to 1 if value not given
        int n = 1;
	int s = 1;
	int t = 1;
	// initializing opt
	int opt;
	// accumulator to track current child processes being completed, set at 0
	int current = 0;
	// accumulator to track the total number of child processes launched, set at 0
	int total = 0;

	// getopt(3) parses options
	while ((opt = getopt(argc, argv, "hn:s:t:")) != -1) {
		switch (opt) {
			// outputs a help message explaining how to run the program, then exits
			case 'h':
				std::cout << "To run program:\n\t ./oss -n # -s # -t #\n";
				std::cout << "Replace # with an integer.\n";
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
			// number of iterations for each child process
			case 't':
				// converts the string input for t to int and assigns it to t
				t = atoi(optarg);
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

	if (s > n){
		exit(1);
	}
	
	return 0;
}


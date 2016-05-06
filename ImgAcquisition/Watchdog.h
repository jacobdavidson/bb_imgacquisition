#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <memory>
#include <iostream>


class Watchdog 
{


public:

	int 			camActive[6];			//[x]==1 means cam x is active. x==5 is the analysis thread.
	unsigned long int 	timestamps[6];			//Timestamp when x was seen alive last time.

	void check()
	{
		//Find processes which might be dead
		//Iff the process didn't deliver something for analysis in the
		//last 60 seconds it's considered dead.
		for (int i=0; i<=5; i++){
			if (camActive[i] == 1){
				unsigned long int now = time(NULL);
				if (now - timestamps[i] > 60){
					std::cout << "Error: process dead! " << i << std::endl;
					std::exit(1);
				}
			}
		}
	}

	//Signal from a process that it's alive
	void pulse(int id)
	{
		camActive[id] = 1;
		timestamps[id] = time(NULL);
	}
};

#endif // WATCHDOG_H


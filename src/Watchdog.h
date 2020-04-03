#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <memory>
#include <iostream>

class Watchdog
{

public:
    //! [x]==1 means cam x is active. x==5 is the analysis thread.
    int _camActive[6];

    //! Timestamp when x was seen alive last time.
    unsigned long int _timestamps[6];

    /**
     * @brief Check all slots for a timeout.
     */
    void check()
    {
        // Find processes which might be dead
        // Iff the process didn't deliver something for analysis in the
        // last 60 seconds it's considered dead.
        for (int i = 0; i <= 5; i++)
        {
            if (_camActive[i] == 1)
            {
                unsigned long int now = time(NULL);
                if (now - _timestamps[i] > 60)
                {
                    std::cout << "Error: process dead! " << i << std::endl;
                    std::exit(1);
                }
            }
        }
    }

    /**
     * @brief As a process, signal that it's alive
     *
     * @param the process id. Has to be 0 ~ 5
     */
    void pulse(int id)
    {
        _camActive[id]  = 1;
        _timestamps[id] = time(NULL);
    }
};

#endif // WATCHDOG_H

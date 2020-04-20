// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <memory>
#include <iostream>

class Watchdog
{

public:
    //! [x]==1 means cam x is active. x==5 is the shared memory thread.
    int _camActive[6];

    //! Timestamp when x was seen alive last time.
    uint64_t _timestamps[6];

    /**
     * @brief Check all slots for a timeout.
     */
    void check()
    {
        // Find processes which might be dead
        // If the process didn't deliver something in the
        // last 60 seconds it's considered dead.
        for (int i = 0; i <= 5; i++)
        {
            if (_camActive[i] == 1)
            {
                uint64_t now = time(NULL);
                if (now - _timestamps[i] > 60)
                {
                    std::cerr << "Error: process dead! " << i << std::endl;
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

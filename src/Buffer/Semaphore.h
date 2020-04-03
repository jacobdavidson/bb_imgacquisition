/*
 * Semaphore.h
 *
 *  Created on: Feb 25, 2016
 *      Author: hauke
 */

#ifndef SEMAPHORE_H_
#define SEMAPHORE_H_

#include <mutex>
#include <condition_variable>

//Courtesy: http://stackoverflow.com/questions/4792449/c0x-has-no-semaphores-how-to-synchronize-threads
//by: Tsuneo Yoshioka
class Semaphore {
public:
    Semaphore (int count_ = 0)
        : count(count_) {}

    inline void notify()
    {
        std::unique_lock<std::mutex> lock(mtx);
        count++;
        cv.notify_one();
    }

    inline void wait()
    {
        std::unique_lock<std::mutex> lock(mtx);

        while(count == 0){
            cv.wait(lock);
        }
        count--;
    }

private:
    std::mutex mtx;
    std::condition_variable cv;
    int count;
};

#endif /* SEMAPHORE_H_ */

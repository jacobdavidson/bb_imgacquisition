//
#include "ImgAcquisitionApp.h"
#include "settings/Settings.h"
#include "settings/utility.h"

#ifdef WITH_DEBUG_IMAGE_OUTPUT
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#endif

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

/*
 * If the segfault is caused from parsing config, well...
 * -> infinte loop.
 */
int handlecounter = 0;

/**
 * @brief handles segmentation faults
 */
void handler(int sig) {
    if (handlecounter>0) exit(1);
    handlecounter++;

    void *array[10];
    size_t size;

    // get void*'s for all entries on the stack
    size = backtrace(array, 10);

    // print out all the frames to stderr
    fprintf(stderr, "Error: signal %d:\n", sig);
    backtrace_symbols_fd(array, size, STDERR_FILENO);

    exit(1);
}

int main(int argc, char *argv[]) {
    signal(SIGSEGV, handler);   // install exception handler
    SettingsIAC::setConf("configImAcq.json");
    SettingsIAC::getInstance();
#ifdef WITH_DEBUG_IMAGE_OUTPUT
    cv::namedWindow("Display window", cv::WINDOW_AUTOSIZE);
    cv::waitKey(100);
#endif
    ImgAcquisitionApp a(argc, argv);  //An instance is initialized

    return a.exec();
}

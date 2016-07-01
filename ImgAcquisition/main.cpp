//
#include "ImgAcquisitionApp.h"
#include "settings/Settings.h"

#include <stdio.h>
#include <execinfo.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * @brief handles segmentation faults
 */
void handler(int sig) {
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

    ImgAcquisitionApp a(argc, argv);  //An instance is initialized

    return a.exec();
}

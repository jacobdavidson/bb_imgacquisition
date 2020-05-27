// SPDX-License-Identifier: GPL-3.0-or-later

#include "PlatformAdapter.hpp"

#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <thread>
#include <array>

#include <QMetaObject>

#if defined(_WIN32) || defined(_WIN64)
    #define WindowsImplementation 1
#elif __has_include(<unistd.h>)
extern "C"
{
    #include <unistd.h>
}

    #if defined(_POSIX_VERSION)
        #define PosixImplementation 1
    #else
        #error Platform unsupported
    #endif
#else
    #error Platform unsupported
#endif

#if PosixImplementation
extern "C"
{
    #include <signal.h>
    #include <sys/socket.h>
    #include <poll.h>
}
#elif WindowsImplementation
    #include <Windows.h>
#else
    #error Platform unsupported
#endif

#include "util/log.hpp"

class Implementation final
{
public:
    static Implementation& instance()
    {
        static Implementation instance;
        return instance;
    }

    void add(PlatformAdapter* adapter)
    {
        auto lock = std::unique_lock(_mutex);
        _adapters.insert(adapter);
    }

    void remove(PlatformAdapter* adapter)
    {
        auto lock = std::unique_lock(_mutex);
        _adapters.erase(adapter);
    }

    Implementation(const Implementation&)     = delete;
    Implementation(Implementation&&) noexcept = delete;
    Implementation& operator=(const Implementation&) = delete;
    Implementation& operator=(Implementation&&) noexcept = delete;

private:
#if PosixImplementation
    static constexpr auto _signalNumbers = std::array{// Interrupt process
                                                      SIGINT,
                                                      // Terminate process
                                                      SIGHUP,
                                                      SIGQUIT,
                                                      SIGTERM};
    static std::unordered_map<int, std::array<int, 2>> _signalSocketPairs;
#endif
    Implementation()
    {
#if PosixImplementation

        for (const auto signalNumber : _signalNumbers)
        {
            if (socketpair(AF_UNIX, SOCK_STREAM, 0, _signalSocketPairs.at(signalNumber).data()))
            {
                throw std::runtime_error("Could not create unix socket pair for signal handling");
            }

            struct sigaction signalAction;
            sigemptyset(&signalAction.sa_mask);
            signalAction.sa_flags = SA_RESTART;

            signalAction.sa_handler = [](int signalNumber) {
                char                        a = 1;
                [[maybe_unused]] const auto r = write(_signalSocketPairs.at(signalNumber)[0],
                                                      &a,
                                                      1);
            };
            if (sigaction(signalNumber, &signalAction, NULL))
            {
                throw std::runtime_error("Could not set signal action for signal handling");
            }
        }

        auto* signalThread = new std::thread([]() {
            std::vector<struct pollfd> sockets;
            std::vector<int>           signalNumbers;
            for (const auto [signalNumber, socketPair] : _signalSocketPairs)
            {
                struct pollfd fd;
                fd.fd     = socketPair[1];
                fd.events = POLLIN;
                sockets.push_back(fd);
                signalNumbers.push_back(signalNumber);
            }

            const auto emitSignal = [](int signalNumber) {
                const auto signalName = [&]() -> std::string {
                    switch (signalNumber)
                    {
                    case SIGINT:
                        return "interruptReceived";
                    case SIGHUP:
                    case SIGQUIT:
                    case SIGTERM:
                        return "terminateReceived";
                    default:
                        return "";
                    }
                }();

                if (signalName.empty())
                {
                    return;
                }

                auto& instance = Implementation::instance();
                auto  lock     = std::unique_lock(instance._mutex);

                for (auto* adapter : instance._adapters)
                {
                    QMetaObject::invokeMethod(adapter, signalName.c_str(), Qt::QueuedConnection);
                }
            };

            while (true)
            {
                auto r = poll(sockets.data(), sockets.size(), -1);
                if (r > 0)
                {
                    for (auto i = 0; i < sockets.size(); ++i)
                    {
                        if (sockets[i].revents & POLLIN)
                        {
                            char                        a;
                            [[maybe_unused]] const auto r = read(sockets[i].fd, &a, 1);
                            emitSignal(signalNumbers[i]);
                        }
                    }
                }
                else if (r < 0)
                {
                    logCritical("Failed to poll signal handling sockets");
                }
            }
        });
#elif WindowsImplementation
        SetConsoleCtrlHandler(
            [](DWORD event) -> BOOL {
                // This lambda runs in a temporary thread created by Windows

                const auto signalName = [&]() -> std::string {
                    switch (event)
                    {
                    case CTRL_C_EVENT:
                        return "interruptReceived";
                    case CTRL_BREAK_EVENT:
                        return "interruptReceived";
                    default:
                        return "";
                    }
                }();

                if (signalName.empty())
                {
                    return FALSE;
                }

                auto& instance = Implementation::instance();
                auto  lock     = std::unique_lock(instance._mutex);

                for (auto* adapter : instance._adapters)
                {
                    QMetaObject::invokeMethod(adapter, signalName.c_str(), Qt::QueuedConnection);
                }

                return TRUE;
            },
            TRUE);
#else
    #error Platform unsupported
#endif
    }

    ~Implementation() noexcept = default;

    std::mutex                           _mutex;
    std::unordered_set<PlatformAdapter*> _adapters;
};

#if PosixImplementation
std::unordered_map<int, std::array<int, 2>> Implementation::_signalSocketPairs = []() {
    std::unordered_map<int, std::array<int, 2>> map;
    for (auto signalNumber : Implementation::_signalNumbers)
    {
        map.insert({signalNumber, {}});
    }
    return map;
}();
#endif

PlatformAdapter::PlatformAdapter(QObject* parent)
: QObject(parent)
{
    Implementation::instance().add(this);
}

PlatformAdapter::~PlatformAdapter()
{
    try
    {
        Implementation::instance().remove(this);
    }
    catch (const std::exception& e)
    {
        // noexcept will force termination if this throws
        logCritical(e.what());
        std::terminate();
    }
}

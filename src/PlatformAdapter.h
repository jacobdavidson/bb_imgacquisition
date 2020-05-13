// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QObject>

// Interfaces with platform signals not sufficiently abstracted by Qt.
// Requires at least one instance to be created before spawning any additional threads.
class PlatformAdapter final : public QObject
{
    Q_OBJECT

public:
    PlatformAdapter(QObject* parent = nullptr);
    ~PlatformAdapter() noexcept;

    PlatformAdapter(const PlatformAdapter&)     = delete;
    PlatformAdapter(PlatformAdapter&&) noexcept = delete;
    PlatformAdapter& operator=(const PlatformAdapter&) = delete;
    PlatformAdapter& operator=(PlatformAdapter&&) noexcept = delete;
signals:
    // Supported platforms: POSIX, Windows
    void interruptReceived();
    // Supported platforms: POSIX
    void terminateReceived();
};

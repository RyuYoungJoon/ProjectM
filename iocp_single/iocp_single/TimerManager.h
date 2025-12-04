#pragma once
#include "pch.h"
#include "event.h"

class TimerManager {
public:
    TimerManager();
    ~TimerManager();

    void AddTimer(int objId, EVENT_TYPE evType, std::chrono::system_clock::time_point time);
    void Start(HANDLE iocpHandle);
    void Stop();

    std::optional<timer_event> PopEvent();
    bool HasPendingEvents() const;

private:
    void TimerThreadFunc();

    std::priority_queue<timer_event> m_timerQueue;
    std::mutex m_timerLock;
    std::atomic<bool> m_running;
    std::unique_ptr<std::thread> m_timerThread;
    HANDLE m_iocpHandle;
};

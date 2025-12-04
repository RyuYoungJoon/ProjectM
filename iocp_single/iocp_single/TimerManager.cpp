#include "pch.h"
#include "TimerManager.h"

TimerManager::TimerManager() 
    : m_running(false)
    , m_iocpHandle(nullptr)
{
}

TimerManager::~TimerManager() {
    Stop();
}

void TimerManager::AddTimer(int objId, EVENT_TYPE evType, std::chrono::system_clock::time_point time) {
    std::lock_guard<std::mutex> lock(m_timerLock);
    timer_event ev{ objId, time, evType, 0 };
    m_timerQueue.emplace(ev);
}

void TimerManager::Start(HANDLE iocpHandle) {
    m_iocpHandle = iocpHandle;
    m_running = true;
    m_timerThread = std::make_unique<std::thread>(&TimerManager::TimerThreadFunc, this);
}

void TimerManager::Stop() {
    m_running = false;
    if (m_timerThread && m_timerThread->joinable()) {
        m_timerThread->join();
    }
}

bool TimerManager::HasPendingEvents() const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_timerLock));
    return !m_timerQueue.empty();
}

std::optional<timer_event> TimerManager::PopEvent() {
    std::lock_guard<std::mutex> lock(m_timerLock);
    if (m_timerQueue.empty()) {
        return std::nullopt;
    }

    timer_event ev = m_timerQueue.top();
    if (ev.start_time > std::chrono::system_clock::now()) {
        return std::nullopt;
    }

    m_timerQueue.pop();
    return ev;
}

void TimerManager::TimerThreadFunc() {
    using namespace std::chrono;

    while (m_running) {
        auto event = PopEvent();
        if (event.has_value()) {
            Exp_Over* exp_over = new Exp_Over;
            
            switch (event->ev) {
            case EVENT_NPC_MOVE:
                exp_over->_comp_op = COMP_OP::OP_NPC_MOVE;
                break;
            case EVENT_ATTACK:
                exp_over->_comp_op = COMP_OP::OP_ATTACK;
                break;
            case EVENT_REVIVE:
                exp_over->_comp_op = COMP_OP::OP_REVIVE;
                break;
            case EVENT_HEAL:
                exp_over->_comp_op = COMP_OP::OP_HEAL;
                break;
            case EVENT_NPC_ATTACK:
                exp_over->_comp_op = COMP_OP::OP_PLAYER_MOVE;
                break;
            default:
                delete exp_over;
                continue;
            }

            PostQueuedCompletionStatus(m_iocpHandle, 1, event->obj_id, &exp_over->_wsa_over);
        }

        std::this_thread::sleep_for(1ms);
    }
}

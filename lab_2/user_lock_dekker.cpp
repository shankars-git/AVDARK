#include "user_locks.hpp"

user_lock_dekker::user_lock_dekker()
    : user_lock() {
    m_flag[0] = m_flag[1] = false;
    m_turn = 0;
}

void user_lock_dekker::lock(int thread_id) {
    m_flag[thread_id].store(true);
    // Need memory fence here
    while (m_flag[(thread_id+1)%2].load() == true) {
        if (m_turn.load() != thread_id) {
            m_flag[thread_id].store(false);
            while (m_turn.load() != thread_id) {}
            m_flag[thread_id].store(true);
            // need memory fence here
        }
    }
}

void user_lock_dekker::unlock(int thread_id) {
    m_turn.store((thread_id+1)%2);
    m_flag[thread_id].store(false);
}

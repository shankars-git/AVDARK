#include "user_locks.hpp"

user_lock_dekker::user_lock_dekker()
    : user_lock() {
    m_flag[0] = m_flag[1] = false;
    m_turn = 0;
}

void user_lock_dekker::lock(int thread_id) {
    // TODO: Implement the lock acquire part of the Dekker algorithm here
    int other_thread_id = !thread_id; //LD ST
    // false = 0, true = 1
    int8_t turn = thread_id; //LD ST
    m_flag[thread_id] = true;//LD ST
    
    while(m_flag[other_thread_id]){ //LD LD
        if(m_turn != turn ){//LD LD 
            m_flag[thread_id] = false; //LD ST
            while (m_turn != turn){} //LD LD
            m_flag[thread_id] = true; //LD ST
        }
    }
}

void user_lock_dekker::unlock(int thread_id) {

    m_turn = !thread_id; //LD ST 
    m_flag[thread_id] = false;//LD ST
    
}

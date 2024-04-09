/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_TIMEOUT_QUEUE_H
#define SKEL_TIMEOUT_QUEUE_H

#include <queue>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <thread>

#include "api/ax_skel_def.h"
#include "utils/frame_utils.hpp"

namespace skel {
    namespace utils {
        template <typename T>
        class TimeoutQueue {
        public:
            explicit TimeoutQueue(int max_len = -1):
                    m_max_len(max_len),
                    m_hasClosed(false) {

            }
            ~TimeoutQueue() = default;

            // un-copyable or moveable
            TimeoutQueue(const TimeoutQueue&) = delete;
            TimeoutQueue& operator = (const TimeoutQueue&) = delete;

            inline void SetCapacity(int max_len) {
                std::lock_guard<std::mutex> lock(m_lock);
                m_max_len = max_len;
            }

            inline void Close() {
                std::lock_guard<std::mutex> lock(m_lock);
                m_hasClosed = true;
                m_new_item.notify_all();
            }

            inline size_t Size() {
                std::lock_guard<std::mutex> lock(m_lock);
                return m_queue.size();
            }

            AX_S32 Push(T& item, int timeout = -1) {
                std::unique_lock<std::mutex> lock(m_lock);

                if (timeout == 0) {
                    if (IsFull())   return AX_ERR_SKEL_QUEUE_FULL;
                } else {
                    if (timeout > 0) {
                        if (!m_remove_item.wait_for(lock, std::chrono::milliseconds(timeout), [this](){ return !(m_queue.size() >= m_max_len);}))
                            return AX_ERR_SKEL_TIMEOUT;
                    } else {    // block
                        while (IsFull())
                            m_remove_item.wait(lock);
                    }
                }

                m_queue.push(item);
                lock.unlock();
                m_new_item.notify_one();

                return AX_SKEL_SUCC;
            }

            AX_S32 Pop(T& item, int timeout = -1) {
                std::unique_lock<std::mutex> lock(m_lock);

                if (timeout == 0) {
                    if (IsEmpty())  return AX_ERR_SKEL_QUEUE_EMPTY;
                } else {
                    if (timeout > 0) {
                        if (!m_new_item.wait_for(lock, std::chrono::milliseconds(timeout), [this](){ return !m_queue.empty() || !m_hasClosed; }))
                            return AX_ERR_SKEL_TIMEOUT;
                    } else {
                        while (IsEmpty() && !m_hasClosed)
                            m_new_item.wait(lock);
                    }
                }

                if (!IsEmpty()) {
                    item = m_queue.front();
                    m_queue.pop();
                    lock.unlock();

                    m_remove_item.notify_one();

                    return AX_SKEL_SUCC;
                }
                else {
                    return AX_ERR_SKEL_QUEUE_EMPTY;
                }
            }

            inline bool IsFull() {
//                std::lock_guard<std::mutex> lock(m_lock);
                if (m_max_len <= 0) {
                    return false;
                } else {
                    return m_queue.size() >= m_max_len;
                }
            }

            inline bool IsEmpty() {
//                std::lock_guard<std::mutex> lock(m_lock);
                return m_queue.empty();
            }

            inline bool IsClosed() {
                return m_hasClosed;
            }

        protected:
            int m_max_len;
            std::queue<T> m_queue;
            mutable std::mutex m_lock;
            std::condition_variable m_new_item;
            std::condition_variable m_remove_item;
            bool m_hasClosed;
        };
    }
}

#endif //SKEL_TIMEOUT_QUEUE_H

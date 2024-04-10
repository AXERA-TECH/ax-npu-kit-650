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
                    m_max_len(max_len) {

            }
            ~TimeoutQueue() = default;

            // un-copyable or moveable
            TimeoutQueue(const TimeoutQueue&) = delete;
            TimeoutQueue& operator = (const TimeoutQueue&) = delete;

            inline bool IsFull() {
                std::lock_guard<std::mutex> lock(m_lock);
                if (m_max_len <= 0) {
                    return false;
                } else {
                    return m_queue.size() >= m_max_len;
                }
            }

            inline bool IsEmpty() {
                std::lock_guard<std::mutex> lock(m_lock);
                return m_queue.empty();
            }

            inline void SetCapacity(int max_len) {
                m_max_len = max_len;
            }

            inline size_t Size() {
                std::lock_guard<std::mutex> lock(m_lock);
                return m_queue.size();
            }

            inline void Close() {
                std::lock_guard<std::mutex> lock(m_lock);
                m_new_item.notify_all();
            }

            AX_S32 Push(T& item, int timeout = -1) {
                if (timeout == 0) {
                    if (IsFull())   return AX_ERR_SKEL_QUEUE_FULL;
                } else {
                    if (timeout > 0) {
                        auto start = std::chrono::steady_clock::now();
                        auto end = start;
                        int elapsed = 0;

                        while (IsFull()) {
                            // 100us
                            std::this_thread::sleep_for(std::chrono::microseconds(100));

                            end = std::chrono::steady_clock::now();
                            elapsed = (int)(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

                            if (elapsed > timeout) {
                                return AX_ERR_SKEL_TIMEOUT;
                            }
                        }
                    } else {    // block
                        while (IsFull())
                            std::this_thread::sleep_for(std::chrono::microseconds(100));
                    }
                }

                std::unique_lock<std::mutex> lock(m_lock);
                m_queue.push(item);
                lock.unlock();
                m_new_item.notify_one();

                return AX_SKEL_SUCC;
            }

            AX_S32 Pop(T& item, int timeout = -1) {
                if (timeout == 0) {
                    if (IsEmpty())  return AX_ERR_SKEL_QUEUE_EMPTY;
                } else {
                    if (timeout > 0) {
                        auto start = std::chrono::steady_clock::now();
                        auto end = start;
                        int elapsed = 0;

                        while (IsEmpty()) {
                            // 100us
                            std::this_thread::sleep_for(std::chrono::microseconds(100));

                            end = std::chrono::steady_clock::now();
                            elapsed = (int)(std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count());

                            if (elapsed > timeout) {
                                return AX_ERR_SKEL_TIMEOUT;
                            }
                        }
                    } else {
                        if (IsEmpty()) {
                            std::unique_lock<std::mutex> lock(m_lock);
                            m_new_item.wait(lock);
                        }

                    }
                }

                std::unique_lock<std::mutex> lock(m_lock);
                if (m_queue.empty()) {
                    return AX_ERR_SKEL_UNEXIST;
                }

                item = m_queue.front();
                m_queue.pop();
                return AX_SKEL_SUCC;
            }

        protected:
            int m_max_len;
            std::queue<T> m_queue;
            std::mutex m_lock;
            std::condition_variable m_new_item;
        };
    }
}

#endif //SKEL_TIMEOUT_QUEUE_H

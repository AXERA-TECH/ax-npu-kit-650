/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_FRAME_QUEUE_H
#define SKEL_FRAME_QUEUE_H

#include "timeout_queue.h"

namespace skel {
    namespace utils {
        class FrameQueue : public TimeoutQueue<AX_SKEL_FRAME_T*> {
        public:
            explicit FrameQueue(int max_len):
                    TimeoutQueue<AX_SKEL_FRAME_T*>(max_len) {}

            ~FrameQueue() {
                while (!m_queue.empty()) {
                    AX_SKEL_FRAME_T* frame = m_queue.front();
                    m_queue.pop();
                    if (frame) {
//                        FreeFrame(frame->stFrame);
                        free(frame);
                    }
                }
            }
        };
    }
}

#endif //SKEL_FRAME_QUEUE_H

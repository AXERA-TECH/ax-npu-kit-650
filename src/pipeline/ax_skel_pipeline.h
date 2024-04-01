/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_AX_SKEL_PIPELINE_H
#define SKEL_AX_SKEL_PIPELINE_H

#include "api/ax_skel_def.h"

#include "utils/timeout_queue.h"
#include "utils/frame_queue.h"

namespace skel {
    namespace ppl {
        class PipelineBase {
        public:
            PipelineBase():
                m_input_queue(SKEL_DEFAULT_QUEUE_LEN),
                m_originSize({0, 0}) {

            }
            virtual ~PipelineBase() = default;

            virtual AX_SKEL_PPL_E Type() = 0;
            virtual AX_S32 Init(const AX_SKEL_HANDLE_PARAM_T *pstParam) = 0;
            virtual AX_S32 DeInit() = 0;
            virtual AX_S32 GetConfig(const AX_SKEL_CONFIG_T **ppstConfig) = 0;
            virtual AX_S32 SetConfig(const AX_SKEL_CONFIG_T *pstConfig) = 0;
            virtual AX_S32 GetResult(AX_SKEL_RESULT_T **ppstResult, AX_S32 nTimeout) = 0;

            virtual AX_S32 RegisterResultCallback(AX_SKEL_RESULT_CALLBACK_FUNC callback, AX_VOID *pUserData);
            // Must implement this
            virtual AX_S32 ResultCallbackThread();
            virtual AX_S32 SendFrame(const AX_SKEL_FRAME_T *pstFrame, AX_S32 nTimeout);

            virtual AX_S32 Start();
            // Must implement this
            virtual AX_S32 Run();
            virtual AX_VOID Stop();
            inline bool IsRunning() const {
                return m_isRunning;
            }

        protected:
            AX_SKEL_HANDLE_PARAM_T m_stHandleParam;
            AX_SKEL_RESULT_CALLBACK_FUNC m_callback{nullptr};
            AX_VOID* m_userData{nullptr};
            skel::utils::FrameQueue m_input_queue;
            volatile bool m_isRunning{false};
            std::array<int, 2> m_originSize;    // height, width
        };
    }
}

#endif //SKEL_AX_SKEL_PIPELINE_H

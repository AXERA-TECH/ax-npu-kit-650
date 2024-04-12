/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "pipeline/ax_skel_pipeline.h"
#include "utils/frame_utils.hpp"

#include <stdlib.h>
#include <thread>

AX_S32 skel::ppl::PipelineBase::SendFrame(const AX_SKEL_FRAME_T *pstFrame, AX_S32 nTimeout) {
    AX_S32 ret = AX_SKEL_SUCC;

    if (m_originSize[0] == 0 || m_originSize[1] == 0) {
        m_originSize[0] = pstFrame->stFrame.u32Height;
        m_originSize[1] = pstFrame->stFrame.u32Width;

        ALOGD("origin size fallback to %d %d\n", m_originSize[0], m_originSize[1]);
    }

    auto *pstNewFrame = (AX_SKEL_FRAME_T*)malloc(sizeof(AX_SKEL_FRAME_T));
    pstNewFrame->nFrameId = pstFrame->nFrameId;
    pstNewFrame->nStreamId = pstFrame->nStreamId;
    pstNewFrame->pUserData = pstFrame->pUserData;
    memcpy(&pstNewFrame->stFrame, &pstFrame->stFrame, sizeof(AX_VIDEO_FRAME_T));
    utils::IncFrameRefCnt(*pstNewFrame);

    ret = m_input_queue.Push(pstNewFrame, nTimeout);
    if (AX_SKEL_SUCC != ret) {
        ALOGE("Push frame failed! ret= 0x%x\n", ret);
        free(pstNewFrame);
        return ret;
    }

    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineBase::RegisterResultCallback(AX_SKEL_RESULT_CALLBACK_FUNC callback, AX_VOID *pUserData) {
    if (m_callback) {
        ALOGE("Already registered callback\n");
        return AX_ERR_SKEL_INITED;
    }

    m_callback = callback;
    m_userData = pUserData;

    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineBase::Start() {
    if (m_isRunning) {
        ALOGE("Pipeline is still running!\n");
        return AX_ERR_SKEL_INITED;
    }

    m_isRunning = true;

    std::thread run_thread( [this] {
        ALOGD("run thread start\n");
        while (IsRunning()) {
            Run();
        }
    });

    std::thread result_thread( [this] {
        ALOGD("result thread start\n");
        while (IsRunning()) {
            if (m_callback)
                ResultCallbackThread();
        }
    });

    run_thread.detach();
    result_thread.detach();

    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineBase::Run() {
    // You should implement this function.

    // 1. Retrieve frame from input queue
    // 2. Process frame
    // 3. Push result to result queue or do nothing

    ALOGE("You shouldn't see this message, please implement Run function in your pipeline class.\n");
    std::this_thread::sleep_for(std::chrono::seconds(60));
    return 0;
}

AX_VOID skel::ppl::PipelineBase::Stop() {
    m_isRunning = false;
}

AX_S32 skel::ppl::PipelineBase::ResultCallbackThread() {
    // You should implement this function.

    ALOGE("You shouldn't see this message, please implement ResultCallbackThread in your pipeline class.\n");
    std::this_thread::sleep_for(std::chrono::seconds(60));
    return 0;
}

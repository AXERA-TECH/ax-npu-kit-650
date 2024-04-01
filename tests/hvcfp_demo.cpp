/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "ax_skel_api.h"
#include "ax_engine_api.h"

#include <cstdio>
#include <thread>
#include <chrono>

#include "test_utils.h"

AX_VOID HvcfpResultCallback(AX_SKEL_HANDLE pHandle, AX_SKEL_RESULT_T *pstResult, AX_VOID *pUserData) {
    printf("Detected %d objects\n", pstResult->nObjectSize);

    const char* filename = (const char*)pUserData;
    cv::Mat result_img = DrawResult(filename, pstResult);
    if (result_img.empty()) {
        printf("DrawResult failed!\n");
    } else {
        cv::imwrite("result.jpg", result_img);
        printf("Saved result to result.jpg\n");
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printf("Usage: %s [image path]\n");
        return -1;
    }

    AX_S32 ret = AX_SKEL_SUCC;
    AX_SKEL_INIT_PARAM_T stInitParam = {0};
    const AX_SKEL_VERSION_INFO_T *pstVersionInfo = nullptr;
    AX_SKEL_HANDLE_PARAM_T stHandleParam;
    AX_SKEL_HANDLE handle;
    AX_SKEL_FRAME_T frame = {0};
    const char* filename = argv[1];
    AX_SKEL_RESULT_T *pstResult = nullptr;
    AX_SKEL_CONFIG_T stConfig = {0};
    bool bUseCallback = true;

    memset(&stHandleParam, 0, sizeof(AX_SKEL_HANDLE_PARAM_T));

    ret = AX_SYS_Init();
    if (0 != ret) {
        printf("AX_SYS_Init failed! ret = 0x%x\n", ret);
        return -1;
    }

    AX_ENGINE_NPU_ATTR_T attr;
    memset(&attr, 0, sizeof(AX_ENGINE_NPU_ATTR_T));
    attr.eHardMode = AX_ENGINE_VIRTUAL_NPU_DISABLE;
    ret = AX_ENGINE_Init(&attr);
    if (ret != 0)
    {
        printf("AXEngine init failed! ret={0x%8x}\n", ret);
        return -1;
    }

    frame.nStreamId = 0;
    frame.nFrameId = 0;
    frame.pUserData = NULL;
    ret = ReadFrame(frame.stFrame, filename, "hvcfp", AX_FORMAT_YUV420_SEMIPLANAR);
    if (AX_SKEL_SUCC != ret) {
        printf("ReadFrame failed! ret = 0x%x\n", ret);
        FreeFrame(frame.stFrame);
        AX_SKEL_DeInit();
        return -1;
    }

    stInitParam.pStrModelDeploymentPath = "../../../models";
    ret = AX_SKEL_Init(&stInitParam);
    if (AX_SKEL_SUCC != ret) {
        printf("AX_SKEL_Init failed! ret = 0x%x\n", ret);
        FreeFrame(frame.stFrame);
        AX_SKEL_DeInit();
        return -1;
    }

    stHandleParam.ePPL = AX_SKEL_PPL_HVCFP;
    ret = AX_SKEL_Create(&stHandleParam, &handle);
    if (AX_SKEL_SUCC != ret) {
        printf("AX_SKEL_Create failed! ret = 0x%x\n", ret);
        FreeFrame(frame.stFrame);
        AX_SKEL_DeInit();
        return -1;
    }

    printf("create handle success\n");

    {
        // set config
        MakeConfig(&stConfig, "track_disable", false);

        ret = AX_SKEL_SetConfig(handle, &stConfig);
        if (AX_SKEL_SUCC != ret) {
            printf("AX_SKEL_SetConfig failed! ret = 0x%x\n", ret);
            FreeFrame(frame.stFrame);
            AX_SKEL_Destroy(handle);
            AX_SKEL_DeInit();
            return -1;
        }
    }

    {
        // Get config
        AX_SKEL_CONFIG_T *pstConfig = nullptr;
        ret = AX_SKEL_GetConfig(handle, (const AX_SKEL_CONFIG_T**)&pstConfig);
        if (AX_SKEL_SUCC != ret) {
            printf("AX_SKEL_GetConfig failed! ret = 0x%x\n", ret);
            FreeFrame(frame.stFrame);
            AX_SKEL_Destroy(handle);
            AX_SKEL_DeInit();
            return -1;
        }
    }

    if (bUseCallback) {
        ret = AX_SKEL_RegisterResultCallback(handle, HvcfpResultCallback, (void*)filename);
        if (AX_SKEL_SUCC != ret) {
            printf("AX_SKEL_RegisterResultCallback failed! ret = 0x%x\n", ret);
            FreeFrame(frame.stFrame);
            AX_SKEL_Destroy(handle);
            AX_SKEL_DeInit();
            return -1;
        }
    }

    ret = AX_SKEL_SendFrame(handle, &frame, -1);
    if (AX_SKEL_SUCC != ret) {
        printf("AX_SKEL_SendFrame failed! ret = 0x%x\n", ret);
        FreeFrame(frame.stFrame);
        AX_SKEL_Destroy(handle);
        AX_SKEL_DeInit();
        return -1;
    }

    FreeFrame(frame.stFrame);

    if (!bUseCallback) {
        ret = AX_SKEL_GetResult(handle, &pstResult, -1);
        if (AX_SKEL_SUCC != ret) {
            printf("AX_SKEL_GetResult failed! ret = 0x%x\n", ret);
            AX_SKEL_Destroy(handle);
            AX_SKEL_DeInit();
            return -1;
        }

        printf("Detected %d objects\n", pstResult->nObjectSize);

        cv::Mat result_img = DrawResult(filename, pstResult);
        if (result_img.empty()) {
            printf("DrawResult failed!\n");
        } else {
            cv::imwrite("result.jpg", result_img);
            printf("Saved result to result.jpg\n");
        }

        ret = AX_SKEL_Release(pstResult);
        if (AX_SKEL_SUCC != ret) {
            printf("AX_SKEL_Release failed! ret = 0x%x\n", ret);
            AX_SKEL_Destroy(handle);
            AX_SKEL_DeInit();
            return -1;
        }
    }

    AX_SKEL_Destroy(handle);

    AX_SKEL_DeInit();

    AX_ENGINE_Deinit();

    AX_SYS_Deinit();

    return 0;
}
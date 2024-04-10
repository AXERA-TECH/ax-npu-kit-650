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

#include "mgr/model_mgr.h"
#include "mgr/ppl_mgr.h"
#include "mgr/mem_mgr.h"

#include "utils/logger.h"
#include "utils/checker.h"

#include "api/ax_skel_version.h"

#include "pipeline/ax_skel_pipeline.h"

#include <cstring>
#include <mutex>
#include <stdlib.h>

#define SKEL_API extern "C" __attribute__((visibility("default")))

static AX_SKEL_VERSION_INFO_T g_axSkelVersionInfo = {0};

static std::mutex gSkelApiMutex;
#define DECLARE_SKEL_THREAD_SAFE_API std::lock_guard<std::mutex> _ApiLck(gSkelApiMutex);

//////////////////////////////////////////////////////////////////////////////////////
/// @brief Initialize skel sdk
///
/// @param pstParam   [I]: initialize parameter
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
SKEL_API AX_S32 AX_SKEL_Init(const AX_SKEL_INIT_PARAM_T *pstParam) {
    CHECK_PTR(pstParam);

    AX_S32 ret = AX_SKEL_SUCC;

    ret = MODELMGR->Init(pstParam);
    if (AX_SKEL_SUCC != ret) {
        ALOGE("Model manager init failed! ret = 0x%x", ret);
        return ret;
    }

    ret = PPLMGR->Init(pstParam);
    if (AX_SKEL_SUCC != ret) {
        ALOGE("Pipeline manager init failed! ret = 0x%x", ret);
        return ret;
    }

    return ret;
}

AX_VOID AX_SKEL_FreeVersion() {
    if (g_axSkelVersionInfo.pstrVersion != nullptr) {
        free(g_axSkelVersionInfo.pstrVersion);
    }
}

//////////////////////////////////////////////////////////////////////////////////////
/// @brief uninitialize skel sdk
///
/// @param NA
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
SKEL_API AX_S32 AX_SKEL_DeInit() {
    MODELMGR->DeInit();
    PPLMGR->DeInit();

    AX_SKEL_FreeVersion();

    return AX_SKEL_SUCC;
}

//////////////////////////////////////////////////////////////////////////////////////
/// @brief get capability
///
/// @param ppstCapability [O]: Capability
///
/// @return version info
//////////////////////////////////////////////////////////////////////////////////////
SKEL_API AX_S32 AX_SKEL_GetCapability(const AX_SKEL_CAPABILITY_T **ppstCapability) {
    DECLARE_SKEL_THREAD_SAFE_API

    CHECK_INITED(MODELMGR);
    CHECK_INITED(PPLMGR);

    return PPLMGR->GetCapability(ppstCapability);
}

//////////////////////////////////////////////////////////////////////////////////////
/// @brief get version
///
/// @param NA
///
/// @return version info
//////////////////////////////////////////////////////////////////////////////////////
SKEL_API AX_S32 AX_SKEL_GetVersion(const AX_SKEL_VERSION_INFO_T **ppstVersion) {
    DECLARE_SKEL_THREAD_SAFE_API

    CHECK_PTR(ppstVersion);

    if (g_axSkelVersionInfo.pstrVersion == nullptr) {
        g_axSkelVersionInfo.pstrVersion = (char*)malloc(AX_SKEL_VERSION_MAXLEN);
        strncpy(g_axSkelVersionInfo.pstrVersion, AX_SKEL_VERSION, AX_SKEL_VERSION_MAXLEN);
    }

    *ppstVersion = &g_axSkelVersionInfo;

    return AX_SKEL_SUCC;
}

//////////////////////////////////////////////////////////////////////////////////////
/// @brief create handle
///
/// @param pstParam   [I]: handle parameter
/// @param handle     [O]: handle
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
SKEL_API AX_S32 AX_SKEL_Create(const AX_SKEL_HANDLE_PARAM_T *pstParam, AX_SKEL_HANDLE *pHandle) {
    DECLARE_SKEL_THREAD_SAFE_API

    CHECK_INITED(MODELMGR);
    CHECK_INITED(PPLMGR);

    CHECK_PTR(pstParam);

    AX_S32 ret = PPLMGR->Create(pstParam, pHandle);
    if (AX_SKEL_SUCC != ret) {
        ALOGE("AX_SKEL_Create failed! ret = 0x%x\n", ret);
        return ret;
    }

    return ret;
}

//////////////////////////////////////////////////////////////////////////////////////
/// @brief destroy handle
///
/// @param pHandle    [I]: handle
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
SKEL_API AX_S32 AX_SKEL_Destroy(AX_SKEL_HANDLE handle) {
    DECLARE_SKEL_THREAD_SAFE_API

    CHECK_INITED(PPLMGR);
    CHECK_PTR(handle);

    return PPLMGR->Destroy(handle);
}

//////////////////////////////////////////////////////////////////////////////////////
/// @brief get config
///
/// @param pstConfig           [I/O]: config
///
/// @return version info
//////////////////////////////////////////////////////////////////////////////////////
SKEL_API AX_S32 AX_SKEL_GetConfig(AX_SKEL_HANDLE handle, const AX_SKEL_CONFIG_T **ppstConfig) {
    DECLARE_SKEL_THREAD_SAFE_API

    CHECK_INITED(PPLMGR);
    CHECK_PTR(handle);
    CHECK_PTR(ppstConfig);

    auto* ppl = (skel::ppl::PipelineBase*)handle;
    return ppl->GetConfig(ppstConfig);
}

//////////////////////////////////////////////////////////////////////////////////////
/// @brief set config
///
/// @param pstConfig           [I]: config
///
/// @return version info
//////////////////////////////////////////////////////////////////////////////////////
SKEL_API AX_S32 AX_SKEL_SetConfig(AX_SKEL_HANDLE handle, const AX_SKEL_CONFIG_T *pstConfig) {
    DECLARE_SKEL_THREAD_SAFE_API

    CHECK_INITED(PPLMGR);
    CHECK_PTR(handle);
    CHECK_PTR(pstConfig);

    auto* ppl = (skel::ppl::PipelineBase*)handle;
    return ppl->SetConfig(pstConfig);
}

//////////////////////////////////////////////////////////////////////////////////////
/// @brief register get algorithm result callback
///
/// @param pHandle    [I]: handle
/// @param callback   [I]: callback function
/// @param pUserData  [I]: private user data
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
SKEL_API AX_S32 AX_SKEL_RegisterResultCallback(AX_SKEL_HANDLE handle, AX_SKEL_RESULT_CALLBACK_FUNC callback, AX_VOID *pUserData) {
    DECLARE_SKEL_THREAD_SAFE_API

    CHECK_INITED(PPLMGR);
    CHECK_PTR(handle);
    CHECK_PTR(callback);

    auto* ppl = (skel::ppl::PipelineBase*)handle;
    return ppl->RegisterResultCallback(callback, pUserData);
}

//////////////////////////////////////////////////////////////////////////////////////
/// @brief send frame
///
/// @param pHandle    [I]: handle
/// @param pstImage   [I]: image
/// @param ppstResult [O]: algorithm result
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
SKEL_API AX_S32 AX_SKEL_SendFrame(AX_SKEL_HANDLE handle, const AX_SKEL_FRAME_T *pstFrame, AX_S32 nTimeout) {
    CHECK_INITED(PPLMGR);
    CHECK_PTR(handle);
    CHECK_PTR(pstFrame);

    auto* ppl = (skel::ppl::PipelineBase*)handle;
    return ppl->SendFrame(pstFrame, nTimeout);
}

//////////////////////////////////////////////////////////////////////////////////////
/// @brief process image
///
/// @param pHandle    [I]: handle
/// @param ppstResult [O]: algorithm result
/// @param nTimeout   [I]: timeout
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
SKEL_API AX_S32 AX_SKEL_GetResult(AX_SKEL_HANDLE handle, AX_SKEL_RESULT_T **ppstResult, AX_S32 nTimeout) {
    CHECK_INITED(PPLMGR);
    CHECK_PTR(handle);
    CHECK_PTR(ppstResult);

    auto* ppl = (skel::ppl::PipelineBase*)handle;
    AX_S32 ret = ppl->GetResult(ppstResult, nTimeout);
    if (AX_SKEL_SUCC == ret) {
        MEMMGR->Add(*ppstResult, AX_SKEL_MEM_RESULT);
    }
    return ret;
}

//////////////////////////////////////////////////////////////////////////////////////
/// @brief free memory
///
/// @param p           [I]: memory address
///
/// @return 0 if success, otherwise failure
//////////////////////////////////////////////////////////////////////////////////////
SKEL_API AX_S32 AX_SKEL_Release(AX_VOID *p) {
    CHECK_PTR(p);

    SKEL_MEM_TYPE mem_type;
    if (MEMMGR->Find(p, mem_type)) {
        if (mem_type == AX_SKEL_MEM_RESULT) {
            auto* result = (AX_SKEL_RESULT_T*)p;
            if (result->nObjectSize > 0) {
                for (int i = 0; i < result->nObjectSize; i++) {
                    if (result->pstObjectItems[i].pstPointSet) {
                        free(result->pstObjectItems[i].pstPointSet);
                    }
                }
                free(result->pstObjectItems);
            }
            if (result->nCacheListSize > 0) {
                free(result->pstCacheList);
            }
            free(result);
        }

        MEMMGR->Erase(p);
    }

    return AX_SKEL_SUCC;
}
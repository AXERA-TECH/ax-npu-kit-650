/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "mgr/ppl_mgr.h"
#include "mgr/model_mgr.h"

#include "utils/logger.h"
#include "utils/checker.h"

#include "ax_skel_err.h"

#include "pipeline/hvcfp/pipeline_hvcfp.h"

#include <cstring>

AX_S32 skel::PipelineMgr::Init(const AX_SKEL_INIT_PARAM_T *pstParam) {
    if (!MODELMGR->HasInit()) {
        ALOGE("Please init model manager first!\n");
        return AX_ERR_SKEL_NOT_INIT;
    }

    if (HasInit()) {
        ALOGE("PipelineMgr has already init.\n");
        return AX_ERR_SKEL_INITED;
    }

    std::unordered_map<AX_SKEL_PPL_E, std::string> valid_ppl;
    for (const auto& kv : PipelineModelBindings) {
        AX_SKEL_PPL_E ppl_type = kv.first;
        size_t require_model_num = kv.second.size();
        size_t found_model_num = 0;
        for (const auto& keyword : kv.second) {
            MODEL_INFO_T tmp;
            if (MODELMGR->Find(keyword, tmp)) {
                found_model_num++;
            }
        }

        // match pipeline need
        if (require_model_num > 0 && require_model_num == found_model_num) {
            MODEL_INFO_T model_info;
            MODELMGR->Find(kv.second[0], model_info);
            valid_ppl.insert({ppl_type, model_info.keyword + ":" + model_info.version});
        }
    }

    m_pstCap = (AX_SKEL_CAPABILITY_T*)malloc(sizeof(AX_SKEL_CAPABILITY_T));
    memset(m_pstCap, 0, sizeof(AX_SKEL_CAPABILITY_T));

    m_pstCap->nPPLConfigSize = valid_ppl.size();
    m_pstCap->pstPPLConfig = (AX_SKEL_PPL_CONFIG_T*)malloc(valid_ppl.size() * sizeof(AX_SKEL_PPL_CONFIG_T));
    size_t nConfigIndex = 0;
    for (const auto& kv : valid_ppl) {
        m_pstCap->pstPPLConfig[nConfigIndex].ePPL = kv.first;
        m_pstCap->pstPPLConfig[nConfigIndex].pstrPPLConfigKey = (char*)malloc(kv.second.size() + 1);
        std::strcpy(m_pstCap->pstPPLConfig[nConfigIndex].pstrPPLConfigKey, kv.second.c_str());

        nConfigIndex++;
    }

    m_hasInit = true;

    return AX_SKEL_SUCC;
}

AX_S32 skel::PipelineMgr::DeInit(AX_VOID) {
    free_cap();

    return AX_SKEL_SUCC;
}

AX_S32 skel::PipelineMgr::GetCapability(const AX_SKEL_CAPABILITY_T **ppstCapability) {
    CHECK_PTR(ppstCapability);

    if (!m_hasInit) {
        ALOGE("PipelineMgr has not init!\n");
        return AX_ERR_SKEL_NOT_INIT;
    }

    *ppstCapability = m_pstCap;

    return AX_SKEL_SUCC;
}

void skel::PipelineMgr::free_cap() {
    if (m_pstCap) {
        if (m_pstCap->pstPPLConfig) {
            for (int i = 0; i < m_pstCap->nPPLConfigSize; i++) {
                if (m_pstCap->pstPPLConfig[i].pstrPPLConfigKey) {
                    free(m_pstCap->pstPPLConfig[i].pstrPPLConfigKey);
                }
            }

            free(m_pstCap->pstPPLConfig);
        }

        free(m_pstCap);
    }
}

AX_S32 skel::PipelineMgr::Create(const AX_SKEL_HANDLE_PARAM_T *pstParam, AX_SKEL_HANDLE *pHandle) {
    CHECK_PTR(pstParam);

    skel::ppl::PipelineBase *ppl = nullptr;
    switch (pstParam->ePPL) {
        case AX_SKEL_PPL_HVCFP:
            ppl = static_cast<skel::ppl::PipelineBase *>(new skel::ppl::PipelineHVCFP);
            break;
        default:
            ALOGE("Currently not support pipeline %d\n", pstParam->ePPL);
            return AX_ERR_SKEL_ILLEGAL_PARAM;
    }

    if (!ppl) {
        ALOGE("Create pipeline failed! Please check ePPL: %d\n", pstParam->ePPL);
        return AX_ERR_SKEL_ILLEGAL_PARAM;
    }

    AX_S32 ret = ppl->Init(pstParam);
    if (AX_SKEL_SUCC != ret) {
        ALOGE("Pipeline %d init failed! ret = 0x%x\n", pstParam->ePPL, ret);
        return AX_ERR_SKEL_ILLEGAL_PARAM;
    }

    ret = ppl->Start();
    if (AX_SKEL_SUCC != ret) {
        ALOGE("Pipeline %d start failed! ret = 0x%x\n", pstParam->ePPL, ret);
        delete ppl;
        return AX_ERR_SKEL_ILLEGAL_PARAM;
    }

    *pHandle = (AX_SKEL_HANDLE)ppl;

    return AX_SKEL_SUCC;
}

bool skel::PipelineMgr::IsPPLValid(AX_SKEL_PPL_E ppl_type) {
    if (m_pstCap->nPPLConfigSize == 0)  return false;

    for (int i = 0; i < m_pstCap->nPPLConfigSize; i++) {
        if (m_pstCap->pstPPLConfig[i].ePPL == ppl_type) {
            return true;
        }
    }
    return false;
}

AX_S32 skel::PipelineMgr::Destroy(AX_SKEL_HANDLE handle) {
    auto *ppl = (skel::ppl::PipelineBase *)handle;

    ppl->Stop();
    ALOGI("Destroying pipeline...\n");
    std::this_thread::sleep_for(std::chrono::seconds(1));

    ppl->DeInit();
    delete ppl;

    return 0;
}


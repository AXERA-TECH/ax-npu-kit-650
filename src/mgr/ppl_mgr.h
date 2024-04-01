/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_PPL_MGR_H
#define SKEL_PPL_MGR_H

#include "api/ax_skel_def.h"
#include "utils/singleton.h"

#include <unordered_map>
#include <string>
#include <vector>

#define PPLMGR skel::PipelineMgr::GetInstance()

typedef std::unordered_map<AX_SKEL_PPL_E, std::string>    PPL_ENUM_STR_MAP;

namespace skel {
    class PipelineMgr : public utils::CSingleton<PipelineMgr> {
        friend class utils::CSingleton<PipelineMgr>;

    public:
        AX_S32 Init(const AX_SKEL_INIT_PARAM_T* pstParam);
        AX_S32 DeInit(AX_VOID);

        inline bool HasInit() const {
            return m_hasInit;
        }

        AX_S32 GetCapability(const AX_SKEL_CAPABILITY_T **ppstCapability);
        AX_S32 Create(const AX_SKEL_HANDLE_PARAM_T *pstParam, AX_SKEL_HANDLE *pHandle);
        AX_S32 Destroy(AX_SKEL_HANDLE handle);
        bool IsPPLValid(AX_SKEL_PPL_E ppl_type);

    private:
        PipelineMgr(AX_VOID):
                m_hasInit(false),
                m_pstCap(nullptr) {

                }
        ~PipelineMgr(AX_VOID) override = default;

        void free_cap();

    private:
        bool m_hasInit;
        AX_SKEL_CAPABILITY_T *m_pstCap;
    };
}

#endif //SKEL_PPL_MGR_H

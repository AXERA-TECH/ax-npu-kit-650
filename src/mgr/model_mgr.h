/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_MODEL_MGR_H
#define SKEL_MODEL_MGR_H

#include "api/ax_skel_def.h"
#include "utils/singleton.h"

#define MODELMGR skel::ModelMgr::GetInstance()

typedef struct {
    std::string path;
    std::string keyword;
    std::string version;
} MODEL_INFO_T;

namespace skel {
    class ModelMgr : public utils::CSingleton<ModelMgr> {
        friend class utils::CSingleton<ModelMgr>;

    public:
        AX_S32 Init(const AX_SKEL_INIT_PARAM_T* pstParam);

        AX_S32 DeInit() {
            m_hasInit = false;
            return AX_SKEL_SUCC;
        }

        inline bool HasInit() const {
            return m_hasInit;
        }

        inline bool Find(const std::string& keyword, MODEL_INFO_T& model_info) const {
            if (m_modelMap.find(keyword) != m_modelMap.end()) {
                model_info = m_modelMap.at(keyword);
                return true;
            }
            else {
                return false;
            }
        }

    private:
        ModelMgr(AX_VOID):
            m_hasInit(false) {

        }
        ~ModelMgr(AX_VOID) override = default;

        std::string get_model_version(const std::string& filename);

    private:
        bool m_hasInit;
        // <keyword, real path> map
        std::unordered_map<std::string, MODEL_INFO_T> m_modelMap;
    };
}

#endif //SKEL_MODEL_MGR_H

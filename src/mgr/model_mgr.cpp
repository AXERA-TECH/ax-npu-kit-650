/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "mgr/model_mgr.h"
#include "ax_skel_err.h"
#include "utils/logger.h"
#include "utils/checker.h"

#include <dirent.h>
#include <cstring>
#include <cstdlib>

AX_S32 skel::ModelMgr::Init(const AX_SKEL_INIT_PARAM_T *pstParam) {
    CHECK_PTR(pstParam);
    CHECK_PTR(pstParam->pStrModelDeploymentPath);

    if (m_hasInit) {
        ALOGE("ModelMgr has already init.\n");
        return AX_ERR_SKEL_INITED;
    }

    std::string model_deploy_path = pstParam->pStrModelDeploymentPath;
    if (model_deploy_path[model_deploy_path.size() - 1] != '/') {
        model_deploy_path += "/";
    }

    // open pStrModelDeploymentPath to find models that match keyword
    DIR *dir_model = NULL;
    struct dirent *ptrl = NULL;
    dir_model = opendir(pstParam->pStrModelDeploymentPath);

    if (!dir_model) {
        ALOGE("SKEL open dir: %s fail\n", pstParam->pStrModelDeploymentPath);
        return AX_ERR_SKEL_ILLEGAL_PARAM;
    }

    // access pStrModelDeploymentPath to map keyword and real path
    while ((ptrl = readdir(dir_model)) != NULL) {
        if (ptrl->d_type == DT_DIR) {
            continue;
        }

        for (const auto& keyword : ModelKeywords) {
            if (m_modelMap.find(keyword) != m_modelMap.end())
                continue;

            if (strstr(ptrl->d_name, keyword.c_str()) != NULL) {
                MODEL_INFO_T model_info;
                model_info.keyword = keyword;
                model_info.path = model_deploy_path + std::string(ptrl->d_name);
                model_info.version = get_model_version(std::string(ptrl->d_name));
                m_modelMap.insert({keyword, model_info});
                ALOGD("keyword: %s\tmodel path: %s\n", keyword.c_str(), model_info.path.c_str());
            }
        }
    }

    m_hasInit = true;

    return AX_SKEL_SUCC;
}

std::string skel::ModelMgr::get_model_version(const std::string &filename) {
    size_t start = filename.find('V');
    size_t end = filename.find(".axmodel");
    if (start == std::string::npos || end == std::string::npos || start > end)
        return "unknown";

    return filename.substr(start, end - start);
}


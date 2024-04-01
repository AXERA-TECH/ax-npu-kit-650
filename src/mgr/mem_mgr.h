/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_MEM_MGR_H
#define SKEL_MEM_MGR_H

#include "api/ax_skel_def.h"
#include "utils/singleton.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <unordered_map>

#define MEMMGR skel::MemMgr::GetInstance()

enum SKEL_MEM_TYPE {
    AX_SKEL_MEM_RESULT = 0,
    AX_SKEL_MEM_MAX
};

namespace skel {
    class MemMgr : public utils::CSingleton<MemMgr> {
        friend class utils::CSingleton<MemMgr>;

    public:
        bool Add(void* pAddr, SKEL_MEM_TYPE type) {
            if (m_memMap.find(pAddr) != m_memMap.end()) {
                return false;
            } else {
                m_memMap.insert({pAddr, type});
                return true;
            }
        }

        bool Find(void* pAddr, SKEL_MEM_TYPE& type) {
            auto it = m_memMap.find(pAddr);
            if (it == m_memMap.end()) {
                return false;
            } else {
                type = it->second;
                return true;
            }
        }

        bool Erase(void* pAddr) {
            auto it = m_memMap.find(pAddr);
            if (it == m_memMap.end()) {
                return false;
            } else {
                m_memMap.erase(it);
                return true;
            }
        }

    private:
        std::unordered_map<void*, SKEL_MEM_TYPE> m_memMap;
    };
}

#endif //SKEL_MEM_MGR_H

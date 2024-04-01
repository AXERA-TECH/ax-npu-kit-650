/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_CONFIG_UTIL_H
#define SKEL_CONFIG_UTIL_H

#include "api/ax_skel_def.h"

#include <cstring>

namespace skel {
    namespace utils {
        static inline bool ParseConfig(const AX_SKEL_CONFIG_ITEM_T& stConfigItem, const char* key, bool& value) {
            if (strcmp(stConfigItem.pstrType, key) == 0 &&
                stConfigItem.nValueSize == sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T)) {
                auto* pstValue = (AX_SKEL_COMMON_THRESHOLD_CONFIG_T*)stConfigItem.pstrValue;
                value = pstValue->fValue == 1;
                return true;
            } else {
                return false;
            }
        }

        template<typename T>
        inline bool ParseConfigCopy(const AX_SKEL_CONFIG_ITEM_T& stConfigItem, const char* key, T& value) {
            if (strcmp(stConfigItem.pstrType, key) == 0 &&
                stConfigItem.nValueSize == sizeof(T)) {
                auto* pstValue = (T*)stConfigItem.pstrValue;
                value = *pstValue;
                return true;
            } else {
                return false;
            }
        }

        static inline bool ParseConfig(const AX_SKEL_CONFIG_ITEM_T& stConfigItem, const char* key, float& value) {
            if (strcmp(stConfigItem.pstrType, key) == 0 &&
                stConfigItem.nValueSize == sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T)) {
                auto* pstValue = (AX_SKEL_COMMON_THRESHOLD_CONFIG_T*)stConfigItem.pstrValue;
                value = pstValue->fValue;
                return true;
            } else {
                return false;
            }
        }

        static inline bool ParseConfig(const AX_SKEL_CONFIG_ITEM_T& stConfigItem, const char* key, AX_U8& value) {
            if (strcmp(stConfigItem.pstrType, key) == 0 &&
                stConfigItem.nValueSize == sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T)) {
                auto* pstValue = (AX_SKEL_COMMON_THRESHOLD_CONFIG_T*)stConfigItem.pstrValue;
                value = (AX_U8)pstValue->fValue;
                return true;
            } else {
                return false;
            }
        }

        static inline bool ParseConfig(const AX_SKEL_CONFIG_ITEM_T& stConfigItem, const char* key, std::vector<std::string>& value) {
            if (strcmp(stConfigItem.pstrType, key) == 0 &&
                stConfigItem.nValueSize == sizeof(AX_SKEL_TARGET_CONFIG_T)) {
                auto* pstConf = (AX_SKEL_TARGET_CONFIG_T*)stConfigItem.pstrValue;
                if (pstConf->pstItems) {
                    if (pstConf->nSize > 0)
                        value.clear();

                    for (size_t j = 0; j < pstConf->nSize; j ++) {
                        if (pstConf->pstItems[j].pstrObjectCategory) {
                            value.push_back(pstConf->pstItems[j].pstrObjectCategory);
                        }
                    }
                }
                return true;
            } else {
                return false;
            }
        }

        static inline bool ParseConfig(const AX_SKEL_CONFIG_ITEM_T& stConfigItem, const char* key, skel::infer::Size& value) {
            if (strcmp(stConfigItem.pstrType, key) == 0 &&
                stConfigItem.nValueSize == sizeof(AX_SKEL_OBJECT_SIZE_FILTER_CONFIG_T)) {
                auto* pstConf = (AX_SKEL_OBJECT_SIZE_FILTER_CONFIG_T*)stConfigItem.pstrValue;
                value.width = pstConf->nWidth;
                value.height = pstConf->nHeight;
                return true;
            } else {
                return false;
            }
        }

        int FindConfig(AX_SKEL_CONFIG_T *pstConfig, const char* key) {
            for (int i = 0; i < pstConfig->nSize; i++) {
                if (strcmp(key, pstConfig->pstItems[i].pstrType) == 0)
                    return i;
            }
            return -1;
        }

        void MakeConfig(AX_SKEL_CONFIG_T *pstConfig, const char* key, bool value) {
            bool bNewItem = true;
            AX_SKEL_CONFIG_ITEM_T *pstNewItem;
            if (!pstConfig->pstItems) {
                pstConfig->pstItems = (AX_SKEL_CONFIG_ITEM_T*)malloc(sizeof(AX_SKEL_CONFIG_ITEM_T));
                pstConfig->nSize = 1;
                pstNewItem = pstConfig->pstItems;
            } else {
                int index = FindConfig(pstConfig, key);
                if (-1 == index) {
                    auto *pstNewItems = (AX_SKEL_CONFIG_ITEM_T*)malloc((pstConfig->nSize + 1) * sizeof(AX_SKEL_CONFIG_ITEM_T));
                    memcpy(pstNewItems, pstConfig->pstItems, pstConfig->nSize * sizeof(AX_SKEL_CONFIG_ITEM_T));
                    pstConfig->nSize++;
                    free(pstConfig->pstItems);
                    pstConfig->pstItems = pstNewItems;
                    pstNewItem = &pstNewItems[pstConfig->nSize - 1];
                }
                else {
                    bNewItem = false;
                    pstNewItem = &(pstConfig->pstItems[index]);
                }
            }

            pstNewItem->pstrType = (char*)key;
            pstNewItem->nValueSize = sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T);

            AX_SKEL_COMMON_THRESHOLD_CONFIG_T *pstValue;
            if (bNewItem) {
                pstValue = (AX_SKEL_COMMON_THRESHOLD_CONFIG_T*)malloc(sizeof(AX_SKEL_COMMON_THRESHOLD_CONFIG_T));
            }
            else {
                pstValue = (AX_SKEL_COMMON_THRESHOLD_CONFIG_T*)pstNewItem->pstrValue;
            }

            pstValue->fValue = value ? 1 : 0;
            pstNewItem->pstrValue = (void*)pstValue;
        }

        void FreeConfig(AX_SKEL_CONFIG_T *pstConfig) {
            if (!pstConfig)
                return;

            for (int i = 0; i < pstConfig->nSize; i++) {
                free((pstConfig->pstItems[i]).pstrValue);
            }
            free(pstConfig);
        }
    }
}

#endif //SKEL_CONFIG_UTIL_H

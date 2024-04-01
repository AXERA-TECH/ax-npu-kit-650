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

#include <cstdio>

int main(int argc, char** argv) {
    AX_S32 ret = AX_SKEL_SUCC;
    AX_SKEL_INIT_PARAM_T stInitParam = {0};
    const AX_SKEL_VERSION_INFO_T *pstVersionInfo = nullptr;

    stInitParam.pStrModelDeploymentPath = "/opt/etc/skelModels";
    ret = AX_SKEL_Init(&stInitParam);
    if (AX_SKEL_SUCC != ret) {
        printf("AX_SKEL_Init failed! ret = 0x%x\n", ret);
        AX_SKEL_DeInit();
        return -1;
    }

    ret = AX_SKEL_GetVersion(&pstVersionInfo);
    if (AX_SKEL_SUCC != ret) {
        printf("AX_SKEL_GetVersion failed! ret = 0x%x\n", ret);
        AX_SKEL_DeInit();
        return -1;
    }

    printf("AX_SKEL_VERSION: %s\n", pstVersionInfo->pstrVersion);

    AX_SKEL_DeInit();

    return 0;
}
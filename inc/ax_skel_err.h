/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_AX_SKEL_ERR_H
#define SKEL_AX_SKEL_ERR_H

#include "ax_global_type.h"

/* error code */
#define AX_SKEL_SUCC                        (0)
#define AX_ERR_SKEL_NULL_PTR                AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_NULL_PTR)
#define AX_ERR_SKEL_ILLEGAL_PARAM           AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_ILLEGAL_PARAM)
#define AX_ERR_SKEL_NOT_INIT                AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_NOT_INIT)
#define AX_ERR_SKEL_QUEUE_EMPTY             AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_QUEUE_EMPTY)
#define AX_ERR_SKEL_QUEUE_FULL              AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_QUEUE_FULL)
#define AX_ERR_SKEL_UNEXIST                 AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_UNEXIST)
#define AX_ERR_SKEL_TIMEOUT                 AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_TIMED_OUT)
#define AX_ERR_SKEL_SYS_NOTREADY            AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_SYS_NOTREADY)
#define AX_ERR_SKEL_INVALID_HANDLE          AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_INVALID_CHNID)
#define AX_ERR_SKEL_NOMEM                   AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_NOMEM)
#define AX_ERR_SKEL_UNKNOWN                 AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_UNKNOWN)
#define AX_ERR_SKEL_NOT_SUPPORT             AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_NOT_SUPPORT)
#define AX_ERR_SKEL_INITED                  AX_DEF_ERR(AX_ID_SKEL, 1, AX_ERR_EXIST)

#endif //SKEL_AX_SKEL_ERR_H

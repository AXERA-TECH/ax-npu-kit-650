/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_CHECKER_H
#define SKEL_CHECKER_H

#include "ax_skel_err.h"
#include "logger.h"

#define CHECK_PTR(p)                     \
    do {                                 \
        if (!p) {                        \
            ALOGE("%s nil pointer\n", #p);      \
            return AX_ERR_SKEL_NULL_PTR; \
        }                                \
    } while (0)

#define CHECK_INITED(p)                     \
    do {                                    \
        if (!p->HasInit()) {                \
            ALOGE("%s has not init\n", #p); \
            return AX_ERR_SKEL_NOT_INIT;    \
        }                                   \
    } while (0)

#endif //SKEL_CHECKER_H

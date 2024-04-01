/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include "ax_global_type.h"

namespace skel {
    namespace tracker {
#define LARGE 1000000

#if !defined TRUE
#define TRUE 1
#endif
#if !defined FALSE
#define FALSE 0
#endif

#define NEW(x, t, n)                               \
        (x = (t *)malloc(sizeof(t) * (n)))

#define FREE(x)   \
    if (x != nullptr) { \
        free(x);  \
        x = nullptr;    \
    }
#define SWAP_INDICES(a, b)     \
    {                          \
        int_t _temp_index = a; \
        a = b;                 \
        b = _temp_index;       \
    }

#if 0
        #include <assert.h>
#define ASSERT(cond) assert(cond)
#define PRINTF(fmt, ...) printf(fmt, ##__VA_ARGS__)
#define PRINT_COST_ARRAY(a, n)               \
    while (1) {                              \
        printf(#a " = [");                   \
        if ((n) > 0) {                       \
            printf("%f", (a)[0]);            \
            for (uint_t j = 1; j < n; j++) { \
                printf(", %f", (a)[j]);      \
            }                                \
        }                                    \
        printf("]\n");                       \
        break;                               \
    }
#define PRINT_INDEX_ARRAY(a, n)              \
    while (1) {                              \
        printf(#a " = [");                   \
        if ((n) > 0) {                       \
            printf("%d", (a)[0]);            \
            for (uint_t j = 1; j < n; j++) { \
                printf(", %d", (a)[j]);      \
            }                                \
        }                                    \
        printf("]\n");                       \
        break;                               \
    }
#else
#define ASSERT(cond)
#define PRINTF(fmt, ...)
#define PRINT_COST_ARRAY(a, n)
#define PRINT_INDEX_ARRAY(a, n)
#endif

        typedef AX_S32 int_t;
        typedef AX_U32 uint_t;
        typedef double cost_t;
        typedef char boolean;
        typedef enum fp_t { FP_1 = 1, FP_2 = 2, FP_DYNAMIC = 3 } fp_t;

        extern int_t lapjv_internal(const uint_t n, cost_t *cost[], int_t *x, int_t *y);
    }
}

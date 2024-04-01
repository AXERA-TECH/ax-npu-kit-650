/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_JENC_H
#define SKEL_JENC_H

#include <map>
#include <mutex>

#include "utils/singleton.h"
#include "api/ax_skel_def.h"
#include "ax_skel_type.h"
#include "ax_venc_api.h"

#define JENCOBJ skel::utils::CJEnc::GetInstance()

namespace skel {
    namespace utils {
        #define SKEL_VENC_CHN_ENV_STR "SKEL_VENC_CHN_SET"
        #define SKEL_VENC_CHN_DEFAULT 9
        #define SKEL_VENC_QPLEVEL_DEFAULT 75
        #define SKEL_VENC_MIN_WIDTH 32
        #define SKEL_VENC_MIN_HEIGHT 32

        ///
        class CJEnc   : public CSingleton<CJEnc> {
            friend class CSingleton<CJEnc>;

        public:
            CJEnc(AX_VOID);
            virtual ~CJEnc(AX_VOID) = default;

        public:
            virtual AX_S32 Create(AX_U32 nWidth, AX_U32 nHeight);
            virtual AX_S32 Destroy(AX_VOID);
            virtual AX_S32 Get(const AX_VIDEO_FRAME_T &stFrame, AX_SKEL_RECT_T &stRect, AX_U32 &nDstWidth, AX_U32 &nDstHeight,
                               AX_VOID **ppBuf, AX_U32 *pBufSize, AX_U32 nQpLevel);
            virtual AX_S32 Rel(AX_VOID *ppBuf);
            virtual AX_S32 Statistics(AX_VOID);

        private:
            std::mutex m_mtx;
            AX_U32 m_nWidth{0};
            AX_U32 m_nHeight{0};
            AX_BOOL m_bPushValid{AX_FALSE};
            VENC_CHN m_nPushJencChn{SKEL_VENC_CHN_DEFAULT};
            AX_U32 m_nQpLevel{SKEL_VENC_QPLEVEL_DEFAULT};
            AX_U64 m_nGetTimes{0};
            AX_U64 m_nRelTimes{0};
        };
    }
}

#endif //SKEL_JENC_H

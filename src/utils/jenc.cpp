/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils/jenc.h"
#include "utils/logger.h"

AX_S32 CreateJenc(VENC_CHN nJencChn, AX_U32 nWidth, AX_U32 nHeight, AX_U32 nQpLevel) {
    AX_VENC_CHN_ATTR_T stVencChnAttr;
    memset(&stVencChnAttr, 0, sizeof(AX_VENC_CHN_ATTR_T));

    stVencChnAttr.stVencAttr.enMemSource = AX_MEMORY_SOURCE_CMM;

    stVencChnAttr.stVencAttr.u32MaxPicWidth = MAX_JENC_PIC_WIDTH;
    stVencChnAttr.stVencAttr.u32MaxPicHeight = MAX_JENC_PIC_HEIGHT;

    stVencChnAttr.stVencAttr.u32PicWidthSrc = nWidth;
    stVencChnAttr.stVencAttr.u32PicHeightSrc = nHeight;

    stVencChnAttr.stVencAttr.u32BufSize = nWidth * nHeight * 3 / 4; /*stream buffer size*/

    stVencChnAttr.stVencAttr.u8InFifoDepth = 1;  /* depth of input fifo */
    stVencChnAttr.stVencAttr.u8OutFifoDepth = 1; /* depth of output fifo */

    stVencChnAttr.stVencAttr.enLinkMode = AX_VENC_UNLINK_MODE;

    stVencChnAttr.stVencAttr.enType = PT_JPEG;

    auto nRet = AX_VENC_CreateChn(nJencChn, &stVencChnAttr);

    if (nRet != 0) {
        ALOGE("SKEL AX_VENC_CreateChn[%d](%d X %d, size=%d) fail, ret=0x%x", nJencChn, nWidth, nHeight, stVencChnAttr.stVencAttr.u32BufSize, nRet);
        return -1;
    }

    AX_VENC_JPEG_PARAM_T stJpegParam;
    memset(&stJpegParam, 0, sizeof(AX_VENC_JPEG_PARAM_T));
    nRet = AX_VENC_GetJpegParam(nJencChn, &stJpegParam);
    if (nRet != 0) {
        ALOGE("SKEL AX_VENC_GetJpegParam[%d] fail", nJencChn, nRet);
        return AX_ERR_SKEL_ILLEGAL_PARAM;
    }

    stJpegParam.u32Qfactor = nQpLevel;

    nRet = AX_VENC_SetJpegParam(nJencChn, &stJpegParam);
    if (nRet != 0){
        ALOGE("AX_VENC_SetJpegParam[%d] fail", nJencChn, nRet);
        return AX_ERR_SKEL_ILLEGAL_PARAM;
    }

    AX_VENC_RECV_PIC_PARAM_T tRecvParam;
    memset(&tRecvParam, 0x00, sizeof(tRecvParam));
    nRet = AX_VENC_StartRecvFrame(nJencChn, &tRecvParam);

    if (nRet != 0) {
        ALOGE("SKEL AX_VENC_StartRecvFrame[%d] fail, nRet=0x%x", nJencChn, nRet);
        return AX_ERR_SKEL_ILLEGAL_PARAM;
    }

    return AX_SKEL_SUCC;
}

namespace skel {
    namespace utils {
        CJEnc::CJEnc(AX_VOID) {
        }

        AX_S32 CJEnc::Create(AX_U32 nWidth, AX_U32 nHeight) {
            if (!m_bPushValid) {
                AX_VENC_MOD_ATTR_T stVencModAttr;
                memset(&stVencModAttr, 0x00, sizeof(stVencModAttr));
                stVencModAttr.enVencType = AX_VENC_MULTI_ENCODER;
                stVencModAttr.stModThdAttr.u32TotalThreadNum = 1;
                stVencModAttr.stModThdAttr.bExplicitSched = AX_FALSE;
                AX_VENC_Init(&stVencModAttr);

                const char *strSkelVencEnvStr = getenv(SKEL_VENC_CHN_ENV_STR);
                if (strSkelVencEnvStr) {
                    m_nPushJencChn = (VENC_CHN)atoi(strSkelVencEnvStr);
                }

                m_nWidth = nWidth;
                m_nHeight = nHeight;

                AX_S32 nRet = CreateJenc(m_nPushJencChn, nWidth, nHeight, m_nQpLevel);

                if (nRet != 0) {
                    return AX_ERR_SKEL_ILLEGAL_PARAM;
                }

                m_bPushValid = AX_TRUE;
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 CJEnc::Destroy(AX_VOID) {
            if (m_bPushValid) {
                AX_VENC_StopRecvFrame(m_nPushJencChn);
                AX_VENC_DestroyChn(m_nPushJencChn);

                AX_VENC_Deinit();

                m_bPushValid = AX_FALSE;
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 CJEnc::Get(const AX_VIDEO_FRAME_T &stFrame, AX_SKEL_RECT_T &stRect, AX_U32 &nDstWidth, AX_U32 &nDstHeight,
                          AX_VOID **ppBuf, AX_U32 *pBufSize, AX_U32 nQpLevel) {
            std::lock_guard<std::mutex> lck(m_mtx);

            if (!ppBuf || !pBufSize) {
                ALOGE("nil pointer");
                return AX_ERR_SKEL_NULL_PTR;
            }

            AX_S32 nRet = AX_SKEL_SUCC;
            AX_BOOL bJencStreamGet = AX_FALSE;
            AX_VIDEO_FRAME_INFO_T tFrame;
            AX_VENC_STREAM_T stVencStream;
            memset(&stVencStream, 0x00, sizeof(stVencStream));

            if (stFrame.u32Width * stFrame.u32Height > m_nWidth * m_nHeight) {
                ALOGE("not match for input size(%dx%d), init(%dx%d)",
                      stFrame.u32Width, stFrame.u32Height, m_nWidth, m_nHeight);
                return AX_ERR_SKEL_ILLEGAL_PARAM;
            }

            if (m_nQpLevel != nQpLevel) {
                AX_VENC_JPEG_PARAM_T stJpegParam;
                memset(&stJpegParam, 0, sizeof(AX_VENC_JPEG_PARAM_T));
                nRet = AX_VENC_GetJpegParam(m_nPushJencChn, &stJpegParam);
                if (nRet != 0) {
                    ALOGE("SKEL AX_VENC_GetJpegParam[%d] fail", m_nPushJencChn, nRet);
                    return AX_ERR_SKEL_ILLEGAL_PARAM;
                }

                stJpegParam.u32Qfactor = nQpLevel;

                nRet = AX_VENC_SetJpegParam(m_nPushJencChn, &stJpegParam);
                if (nRet != 0){
                    ALOGE("AX_VENC_SetJpegParam[%d] fail", m_nPushJencChn, nRet);
                    return AX_ERR_SKEL_ILLEGAL_PARAM;
                }
            }

            memset(&tFrame, 0x00, sizeof(tFrame));
            tFrame.stVFrame = stFrame;
            tFrame.stVFrame.u64PhyAddr[1] = tFrame.stVFrame.u64PhyAddr[0] + tFrame.stVFrame.u32PicStride[0] * tFrame.stVFrame.u32Height;
            tFrame.stVFrame.u32PicStride[1] = tFrame.stVFrame.u32PicStride[0];
            tFrame.stVFrame.u32PicStride[2] = 0;

            // crop
            if ((AX_U32)stRect.fX != 0
                || (AX_U32)stRect.fY != 0
                || (AX_U32)stRect.fW != 0
                || (AX_U32)stRect.fH != 0) {
                if (stRect.fX < 0) {
                    stRect.fX = 0;
                }

                if (stRect.fY < 0) {
                    stRect.fY = 0;
                }

                tFrame.stVFrame.s16CropX = ALIGN_DOWN((AX_S16)stRect.fX, 2);
                tFrame.stVFrame.s16CropY = ALIGN_DOWN((AX_S16)stRect.fY, 2);
                tFrame.stVFrame.s16CropWidth = ALIGN_DOWN((AX_S16)stRect.fW, 2);
                tFrame.stVFrame.s16CropHeight = ALIGN_DOWN((AX_S16)stRect.fH, 2);

                if (tFrame.stVFrame.s16CropX + tFrame.stVFrame.s16CropWidth > (AX_S32)tFrame.stVFrame.u32Width) {
                    tFrame.stVFrame.s16CropWidth = ALIGN_DOWN(tFrame.stVFrame.u32Width - tFrame.stVFrame.s16CropX, 2);
                }

                if (tFrame.stVFrame.s16CropY + tFrame.stVFrame.s16CropHeight > (AX_S32)tFrame.stVFrame.u32Height) {
                    tFrame.stVFrame.s16CropHeight = ALIGN_DOWN(tFrame.stVFrame.u32Height - tFrame.stVFrame.s16CropY, 2);
                }

                nDstWidth = tFrame.stVFrame.s16CropWidth;
                nDstHeight = tFrame.stVFrame.s16CropHeight;

                // FIXME.
                if (nDstWidth < SKEL_VENC_MIN_WIDTH) {
                    nDstWidth = SKEL_VENC_MIN_WIDTH;
                    tFrame.stVFrame.s16CropWidth = SKEL_VENC_MIN_WIDTH;
                    if (tFrame.stVFrame.s16CropX + tFrame.stVFrame.s16CropWidth > (AX_S32)tFrame.stVFrame.u32Width) {
                        tFrame.stVFrame.s16CropX = tFrame.stVFrame.u32Width - tFrame.stVFrame.s16CropWidth - 2;
                    }
                }
                if (nDstHeight < SKEL_VENC_MIN_HEIGHT) {
                    nDstHeight = SKEL_VENC_MIN_HEIGHT;
                    tFrame.stVFrame.s16CropHeight = SKEL_VENC_MIN_HEIGHT;
                    if (tFrame.stVFrame.s16CropY + tFrame.stVFrame.s16CropHeight > (AX_S32)tFrame.stVFrame.u32Height) {
                        tFrame.stVFrame.s16CropY = tFrame.stVFrame.u32Height - tFrame.stVFrame.s16CropHeight - 2;
                    }
                }

                if (tFrame.stVFrame.s16CropX + tFrame.stVFrame.s16CropWidth > (AX_S32)tFrame.stVFrame.u32Width
                    || tFrame.stVFrame.s16CropY + tFrame.stVFrame.s16CropHeight > (AX_S32)tFrame.stVFrame.u32Height) {
                    ALOGE("SKEL ignore the region[%d,%d,%d,%d]",
                          tFrame.stVFrame.s16CropX,
                          tFrame.stVFrame.s16CropY,
                          tFrame.stVFrame.s16CropWidth,
                          tFrame.stVFrame.s16CropHeight);
                    nRet = AX_ERR_SKEL_ILLEGAL_PARAM;
                    goto JENC_EXIT;
                }
            }
            else {
                nDstWidth = tFrame.stVFrame.u32Width;
                nDstHeight = tFrame.stVFrame.u32Height;
            }

            nRet = AX_VENC_SendFrame(m_nPushJencChn, &tFrame, 2000);

            if (nRet != 0) {
                ALOGE("SKEL AX_VENC_SendFrame[%d] fail, nRet=0x%x", m_nPushJencChn, nRet);
                nRet = AX_ERR_SKEL_ILLEGAL_PARAM;
                goto JENC_EXIT;
            }

            nRet = AX_VENC_GetStream(m_nPushJencChn, &stVencStream, 2000);

            bJencStreamGet = AX_TRUE;

            if (nRet != 0
                || !stVencStream.stPack.pu8Addr
                || stVencStream.stPack.u32Len == 0) {
                ALOGE("SKEL AX_VENC_GetStream[%d] fail, ret=0x%x", m_nPushJencChn, nRet);
                nRet = AX_ERR_SKEL_ILLEGAL_PARAM;
                goto JENC_EXIT;
            }

            *ppBuf = new AX_U8[stVencStream.stPack.u32Len];

            if (!(*ppBuf)) {
                *pBufSize = 0;

                ALOGE("SKEL alloc push buffer fail");
                nRet = AX_ERR_SKEL_NOMEM;
                goto JENC_EXIT;
            }

            ++m_nGetTimes;

            *pBufSize = stVencStream.stPack.u32Len;
            memcpy(*ppBuf, stVencStream.stPack.pu8Addr, stVencStream.stPack.u32Len);

            JENC_EXIT:
            if (bJencStreamGet) {
                AX_VENC_ReleaseStream(m_nPushJencChn, &stVencStream);
            }

            return nRet;
        }

        AX_S32 CJEnc::Rel(AX_VOID *pBuf) {
            if (pBuf) {
                AX_U8 *p = (AX_U8 *)pBuf;
                delete[] p;

                ++m_nRelTimes;
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 CJEnc::Statistics(AX_VOID) {
            ALOGN("Jenc Statistics:");

            ALOGN("\tGet times: %lld, Rel times: %lld", m_nGetTimes, m_nRelTimes);

            return AX_SKEL_SUCC;
        }
    }
}


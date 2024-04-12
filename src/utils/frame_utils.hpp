/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include <string>
#include <array>

#include "ax_skel_type.h"
#include "utils/io.hpp"
#include "ax_sys_api.h"
#include "ax_ivps_api.h"
#include "inference/cv_types.h"

namespace skel {
    namespace utils
    {
        static inline uint32_t get_image_stride_w(const AX_VIDEO_FRAME_T* pImg)
        {
            if (pImg->u32PicStride[0] == 0) {
                return pImg->u32Width;
            }

            return pImg->u32PicStride[0];
        }

        static inline int get_image_data_size(const AX_VIDEO_FRAME_T* img)
        {
            int stride_w = get_image_stride_w(img);
            switch (img->enImgFormat) {
                case AX_FORMAT_YUV420_SEMIPLANAR:  // FIXME
                case AX_FORMAT_YUV420_SEMIPLANAR_VU:
                    return int(stride_w * img->u32Height * 3 / 2);

                case AX_FORMAT_RGB888:
                case AX_FORMAT_BGR888:
                case AX_FORMAT_YUV444_SEMIPLANAR:
                case AX_FORMAT_YUV444_SEMIPLANAR_VU:
                    return int(stride_w * img->u32Height * 3);

                case AX_FORMAT_ARGB8888:
                case AX_FORMAT_RGBA8888:
                    return int(stride_w * img->u32Height * 4);

                case AX_FORMAT_YUV400:
                    return int(stride_w * img->u32Height * 1);

                default:
                    fprintf(stderr, "[ERR] unsupported color space %d to calculate image data size\n", (int)img->enImgFormat);
                    return 0;
            }
        }

        static inline int AllocFrame(AX_VIDEO_FRAME_T& frame, const std::string& token, int nWidth, int nHeight, AX_IMG_FORMAT_E eDtype, utils::SKEL_IO_BUFFER_STRATEGY_T eStrategy = utils::SKEL_IO_BUFFER_STRATEGY_DEFAULT)
        {
            int ret = 0;

            memset(&frame, 0x00, sizeof(AX_VIDEO_FRAME_T));
            frame.u32Width = nWidth;
            frame.u32Height = nHeight;
            frame.u32PicStride[0] = frame.u32Width;
            frame.u32PicStride[1] = frame.u32PicStride[0];
            frame.u32PicStride[2] = frame.u32PicStride[0];
            frame.enImgFormat = eDtype;
            frame.u32FrameSize = get_image_data_size(&frame);

            const std::string token_name = "skel_" + token + "_in";

            if (eStrategy == utils::SKEL_IO_BUFFER_STRATEGY_CACHED) {
                ret = AX_SYS_MemAllocCached((AX_U64*)&frame.u64PhyAddr[0], (AX_VOID **)&frame.u64VirAddr[0], frame.u32FrameSize, SKEL_IO_CMM_ALIGN_SIZE, (AX_S8*)token_name.c_str());
            }
            else {
                ret = AX_SYS_MemAlloc((AX_U64*)&frame.u64PhyAddr[0], (AX_VOID **)&frame.u64VirAddr[0], frame.u32FrameSize, SKEL_IO_CMM_ALIGN_SIZE, (AX_S8*)token_name.c_str());
            }

            if (ret != 0) {
                fprintf(stderr, "[ERR] error alloc image sys mem %x \n", ret);
                return ret;
            }

            frame.u64PhyAddr[1] = frame.u64PhyAddr[0] + frame.u32PicStride[0] * frame.u32Height;
            frame.u64VirAddr[1] = frame.u64VirAddr[0] + frame.u32PicStride[0] * frame.u32Height;

            if (eDtype == AX_FORMAT_BGR888 || eDtype == AX_FORMAT_RGB888) {
                frame.u64PhyAddr[2] = frame.u64PhyAddr[1] + frame.u32PicStride[1] * frame.u32Height;
                frame.u64VirAddr[2] = frame.u64VirAddr[1] + frame.u32PicStride[1] * frame.u32Height;
            }

            return ret;
        }

        static inline AX_S32 CopyFrame(const AX_VIDEO_FRAME_T& src, AX_VIDEO_FRAME_T& dst, const std::string& token) {
            int ret = AllocFrame(dst, token, src.u32Width, src.u32Height, src.enImgFormat);
            AX_U64 virAddr = src.u64VirAddr[0];
            if (AX_SKEL_SUCC != ret) {
                ALOGE("Copy frame failed! ret = 0x%x\n", ret);
                return ret;
            }

            if (virAddr == 0) {
                virAddr = (AX_U64)AX_POOL_GetBlockVirAddr(src.u32BlkId[0]);
            }
            memcpy((void*)dst.u64VirAddr[0], (void*)virAddr, src.u32FrameSize);

            return AX_SKEL_SUCC;
        }

        static inline void FlushFrame(const AX_VIDEO_FRAME_T& stFrame)
        {
            if (stFrame.u64PhyAddr[0] != 0) {
                AX_SYS_MflushCache(stFrame.u64PhyAddr[0], (AX_VOID *)&stFrame.u64VirAddr[0], stFrame.u32FrameSize);
            }
        }

        static inline int FreeFrame(AX_VIDEO_FRAME_T& stFrame)
        {
            int ret = 0;
            if (stFrame.u64PhyAddr[0] != 0)
            {
                ret = AX_SYS_MemFree((AX_U64)stFrame.u64PhyAddr[0], (AX_VOID *)stFrame.u64VirAddr[0]);
                if (ret != 0) {
                    // fprintf(stderr, "[ERR] error free %x \n", ret);
                    return ret;
                }
            }
            memset(&stFrame, 0x00, sizeof(AX_VIDEO_FRAME_T));
            return 0;
        }

        static inline AX_VIDEO_FRAME_T CropFrame(const AX_VIDEO_FRAME_T& src, const skel::infer::Rect& crop_rect)
        {
            if (crop_rect.area() <= 0)
                return src;

            AX_VIDEO_FRAME_T t = {0};
            AX_S32 offset_x = crop_rect.x;
            AX_S32 offset_y = crop_rect.y;
            if (offset_x < 0) {
                offset_x = 0;
            }
            if (offset_y < 0) {
                offset_y = 0;
            }

            t = src;

            t.u32Width  = crop_rect.width;
            t.u32Height = crop_rect.height;

            t.u64PhyAddr[0] = src.u64PhyAddr[0] + src.u32PicStride[0] * offset_y + offset_x;
            t.u64PhyAddr[1] = src.u64PhyAddr[1] + src.u32PicStride[0] * offset_y + offset_x;

            t.u64VirAddr[0] = src.u64VirAddr[0] + src.u32PicStride[0] * offset_y + offset_x;
            t.u64VirAddr[1] = src.u64VirAddr[1] + src.u32PicStride[0] * offset_y + offset_x;

            return t;
        }

        static inline int CropResizeFrame(const AX_VIDEO_FRAME_T& src, AX_VIDEO_FRAME_T& dst, int nWidth, int nHeight, const skel::infer::Rect& crop_rect)
        {
            int ret = 0;
            ret = AllocFrame(dst, "crop_resize", nWidth, nHeight, src.enImgFormat);
            if (ret != 0)
            {
                ALOGE("Alloc crop_resize frame failed!\n");
                return ret;
            }

            AX_IVPS_ASPECT_RATIO_T tAspectRatio;

            memset(&tAspectRatio, 0x00, sizeof(tAspectRatio));
            tAspectRatio.eAligns[0] = AX_IVPS_ASPECT_RATIO_HORIZONTAL_CENTER;
            tAspectRatio.eAligns[1] = AX_IVPS_ASPECT_RATIO_VERTICAL_CENTER;
            tAspectRatio.nBgColor = 0x00000000;
            tAspectRatio.eMode = AX_IVPS_ASPECT_RATIO_AUTO;

            AX_VIDEO_FRAME_T cropSrc;
            memcpy(&cropSrc, &src, sizeof(AX_VIDEO_FRAME_T));
            if (crop_rect.area() > 0)
            {
                tAspectRatio.tRect.nX = crop_rect.x / 2 * 2;
                tAspectRatio.tRect.nY = crop_rect.y / 2 * 2;
                tAspectRatio.tRect.nW = (AX_S16)crop_rect.width / 2 * 2;
                tAspectRatio.tRect.nH = (AX_S16)crop_rect.height / 2 * 2;
            }
            else
            {
                tAspectRatio.tRect.nX = 0;
                tAspectRatio.tRect.nY = 0;
                tAspectRatio.tRect.nW = cropSrc.u32Width;
                tAspectRatio.tRect.nH = cropSrc.u32Height;
            }

            ret = AX_IVPS_CropResizeTdp(&cropSrc, &dst, &tAspectRatio);
            if (ret != 0)
            {
                FreeFrame(dst);
                fprintf(stderr, "AX_IVPS_CropResizeTdp error, ret=0x%8x\n", ret);
                return ret;
            }

            return ret;
        }

        static inline void IncFrameRefCnt(AX_SKEL_FRAME_T& frame)
        {
            utils::inc_io_ref_cnt(frame.stFrame);
        }

        static inline void DecFrameRefCnt(AX_SKEL_FRAME_T& frame)
        {
            utils::dec_io_ref_cnt(frame.stFrame);
        }

        static void FreeFrame(AX_SKEL_FRAME_T* frame)
        {
            if (frame) {
                DecFrameRefCnt(*frame);
                free(frame);
            }
        }
    }
}

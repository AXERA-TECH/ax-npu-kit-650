/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_TEST_UTILS_H
#define SKEL_TEST_UTILS_H

#include <stdlib.h>
#include <string.h>
#include <ax_global_type.h>
#include <ax_engine_type.h>
#include <ax_sys_api.h>
#include <set>

#include "ax_skel_err.h"
#include "ax_skel_type.h"
#include "opencv2/opencv.hpp"

#define SKEL_IO_CMM_ALIGN_SIZE  128

const int COLOR_LIST[80][3] =
        {
                //{255 ,255 ,255}, //bg
                {216 , 82 , 24},
                {236 ,176 , 31},
                {125 , 46 ,141},
                {118 ,171 , 47},
                { 76 ,189 ,237},
                {238 , 19 , 46},
                { 76 , 76 , 76},
                {153 ,153 ,153},
                {255 ,  0 ,  0},
                {255 ,127 ,  0},
                {190 ,190 ,  0},
                {  0 ,255 ,  0},
                {  0 ,  0 ,255},
                {170 ,  0 ,255},
                { 84 , 84 ,  0},
                { 84 ,170 ,  0},
                { 84 ,255 ,  0},
                {170 , 84 ,  0},
                {170 ,170 ,  0},
                {170 ,255 ,  0},
                {255 , 84 ,  0},
                {255 ,170 ,  0},
                {255 ,255 ,  0},
                {  0 , 84 ,127},
                {  0 ,170 ,127},
                {  0 ,255 ,127},
                { 84 ,  0 ,127},
                { 84 , 84 ,127},
                { 84 ,170 ,127},
                { 84 ,255 ,127},
                {170 ,  0 ,127},
                {170 , 84 ,127},
                {170 ,170 ,127},
                {170 ,255 ,127},
                {255 ,  0 ,127},
                {255 , 84 ,127},
                {255 ,170 ,127},
                {255 ,255 ,127},
                {  0 , 84 ,255},
                {  0 ,170 ,255},
                {  0 ,255 ,255},
                { 84 ,  0 ,255},
                { 84 , 84 ,255},
                { 84 ,170 ,255},
                { 84 ,255 ,255},
                {170 ,  0 ,255},
                {170 , 84 ,255},
                {170 ,170 ,255},
                {170 ,255 ,255},
                {255 ,  0 ,255},
                {255 , 84 ,255},
                {255 ,170 ,255},
                { 42 ,  0 ,  0},
                { 84 ,  0 ,  0},
                {127 ,  0 ,  0},
                {170 ,  0 ,  0},
                {212 ,  0 ,  0},
                {255 ,  0 ,  0},
                {  0 , 42 ,  0},
                {  0 , 84 ,  0},
                {  0 ,127 ,  0},
                {  0 ,170 ,  0},
                {  0 ,212 ,  0},
                {  0 ,255 ,  0},
                {  0 ,  0 , 42},
                {  0 ,  0 , 84},
                {  0 ,  0 ,127},
                {  0 ,  0 ,170},
                {  0 ,  0 ,212},
                {  0 ,  0 ,255},
                {  0 ,  0 ,  0},
                { 36 , 36 , 36},
                { 72 , 72 , 72},
                {109 ,109 ,109},
                {145 ,145 ,145},
                {182 ,182 ,182},
                {218 ,218 ,218},
                {  0 ,113 ,188},
                { 80 ,182 ,188},
                {127 ,127 ,  0},
        };

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

static inline int AllocFrame(AX_VIDEO_FRAME_T& frame, const std::string& token, int nWidth, int nHeight, AX_IMG_FORMAT_E eDtype)
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

    const std::string token_name = "demo_" + token + "_in";

    ret = AX_SYS_MemAlloc((AX_U64*)&frame.u64PhyAddr[0], (AX_VOID **)&frame.u64VirAddr[0], frame.u32FrameSize, SKEL_IO_CMM_ALIGN_SIZE, (AX_S8*)token_name.c_str());

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

static inline int FreeFrame(AX_VIDEO_FRAME_T& stFrame)
{
    int ret = 0;
    if (stFrame.u64PhyAddr[0] != 0)
    {
        ret = AX_SYS_MemFree((AX_U64)stFrame.u64PhyAddr[0], (AX_VOID *)stFrame.u64VirAddr[0]);
        if (ret != 0) {
             fprintf(stderr, "[ERR] error free %x \n", ret);
            return ret;
        }
    }
    memset(&stFrame, 0x00, sizeof(AX_VIDEO_FRAME_T));
    return 0;
}

static inline cv::Mat BGR2YUV_NV12(const cv::Mat &src) {
    auto src_h = src.rows;
    auto src_w = src.cols;
    cv::Mat dst(src_h * 1.5, src_w, CV_8UC1);
    cv::cvtColor(src, dst, cv::COLOR_BGR2YUV_I420);  // I420: YYYY...UU...VV...

    auto n_y = src_h * src_w;
    auto n_uv = n_y / 2;
    auto n_u = n_y / 4;
    std::vector<uint8_t> uv(n_uv);
    std::copy(dst.data+n_y, dst.data+n_y+n_uv, uv.data());
    for (auto i = 0; i < n_u; i++) {
        dst.data[n_y + 2*i] = uv[i];            // U
        dst.data[n_y + 2*i + 1] = uv[n_u + i];  // V
    }
    return dst;
}

static inline int ReadFrame(AX_VIDEO_FRAME_T& frame, const char* filename, const std::string& token, AX_IMG_FORMAT_E eDtype) {
    int ret = 0;

    cv::Mat img = cv::imread(filename);
    if (img.empty()) {
        return AX_ERR_SKEL_ILLEGAL_PARAM;
    }

    int width = img.cols;
    int height = img.rows;

    cv::Mat dst;
    switch (eDtype) {
        case AX_FORMAT_YUV420_SEMIPLANAR: {
            dst = BGR2YUV_NV12(img);
            ret = AllocFrame(frame, token, width, height, eDtype);
            if (AX_SKEL_SUCC != ret) {
                return ret;
            }
            memcpy((void*)frame.u64VirAddr[0], dst.data, frame.u32FrameSize);
            return AX_SKEL_SUCC;
        }

        case AX_FORMAT_BGR888: {
            std::vector<cv::Mat> mats;
            cv::split(img, mats);
            memcpy((void*)frame.u64VirAddr[0], mats[0].data, frame.u32FrameSize / 3);
            memcpy((void*)frame.u64VirAddr[1], mats[1].data, frame.u32FrameSize / 3);
            memcpy((void*)frame.u64VirAddr[2], mats[2].data, frame.u32FrameSize / 3);
            return AX_SKEL_SUCC;
        }

        case AX_FORMAT_RGB888: {
            std::vector <cv::Mat> mats;
            cv::split(img, mats);
            memcpy((void *) frame.u64VirAddr[0], mats[2].data, frame.u32FrameSize / 3);
            memcpy((void *) frame.u64VirAddr[1], mats[1].data, frame.u32FrameSize / 3);
            memcpy((void *) frame.u64VirAddr[2], mats[0].data, frame.u32FrameSize / 3);
            return AX_SKEL_SUCC;
        }

        default:
            return AX_ERR_SKEL_ILLEGAL_PARAM;
    }
    return AX_SKEL_SUCC;
}

static inline cv::Mat DrawResult(const char* filename, AX_SKEL_RESULT_T *pstResult) {
    cv::Mat img = cv::imread(filename);
    if (img.empty()) {
        return img;
    }

    std::set<const char*> class_names_set;

    AX_SKEL_OBJECT_ITEM_T *pstObjectItems = pstResult->pstObjectItems;
    for (int i = 0; i < pstResult->nObjectSize; i++) {
        cv::Rect bbox(pstObjectItems[i].stRect.fX,
                      pstObjectItems[i].stRect.fY,
                      pstObjectItems[i].stRect.fW,
                      pstObjectItems[i].stRect.fH);

        int color_index = 0;
        const char* pstrCategory = pstObjectItems[i].pstrObjectCategory;
        auto it = class_names_set.find(pstrCategory);
        if (it != class_names_set.end()) {
            color_index = (int)std::distance(class_names_set.begin(), it);
        } else {
            class_names_set.insert(pstrCategory);
            color_index = class_names_set.size() - 1;
        }

        cv::Scalar color(COLOR_LIST[color_index][0],
                         COLOR_LIST[color_index][1],
                         COLOR_LIST[color_index][2]);
        cv::rectangle(img, bbox, color);

        // Draw text
        char text[256];
        sprintf(text, "%d %s %.1f%%", pstObjectItems[i].nTrackId, pstrCategory, pstObjectItems[i].fConfidence * 100);

        int baseLine = 0;
        cv::Size label_size = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.4, 1, &baseLine);

        int x = bbox.x;
        int y = bbox.y - label_size.height - baseLine;
        if (y < 0)
            y = 0;
        if (x + label_size.width > img.cols)
            x = img.cols - label_size.width;

        cv::rectangle(img, cv::Rect(cv::Point(x, y), cv::Size(label_size.width, label_size.height + baseLine)),
                      color, -1);

        cv::putText(img, text, cv::Point(x, y + label_size.height),
                    cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(255, 255, 255));
    }

    return img;
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

#endif //SKEL_TEST_UTILS_H

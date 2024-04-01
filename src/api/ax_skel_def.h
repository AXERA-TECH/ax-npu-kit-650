/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_AX_SKEL_DEF_H
#define SKEL_AX_SKEL_DEF_H

#include <unordered_map>
#include <string>
#include <vector>
#include <cstring>

#include "ax_skel_type.h"
#include "ax_skel_err.h"
#include "inference/cv_types.h"

#define SKEL_HVCFP_MODEL_KEY_STR     "hvcfp_algo_model"

#define SKEL_DEFAULT_QUEUE_LEN      20

const std::vector<std::string> ModelKeywords = {
        SKEL_HVCFP_MODEL_KEY_STR,
};

// a pipeline may need multiple models
const std::unordered_map<AX_SKEL_PPL_E, std::vector<std::string>> PipelineModelBindings = {
        {AX_SKEL_PPL_HVCFP,  std::vector<std::string>{SKEL_HVCFP_MODEL_KEY_STR} },
};

#define AX_MAX(a, b) (((a) > (b)) ? (a) : (b))
#define AX_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define ALIGN_UP(x, align) (((x) + ((align)-1)) & ~((align)-1))
#define ALIGN_DOWN(x, align) ((x) & ~((align)-1))

#define DEFAULT_SRC_H 1920
#define DEFAULT_SRC_W 1080
#define DEFAULT_QPLEVEL 75
#define DEFAULT_QPLEVEL_MIN 1
#define DEFAULT_QPLEVEL_MAX 99
#define DEFAULT_FRAME_DEPTH 1
#define DEFAULT_FRAME_CACHE_DEPTH 1
#define DEFAULT_POSE_BODY_COUNT 3
#define DEFAULT_BODY_PROB_THRESHOLD 0.55
#define DEFAULT_VEHICLE_PROB_THRESHOLD 0.5
#define DEFAULT_CYCLE_PROB_THRESHOLD 0.5
#define DEFAULT_FACE_PROB_THRESHOLD 0.5
#define DEFAULT_PLATE_PROB_THRESHOLD 0.5

typedef struct axSKEL_MAX_TARGET_COUNT_T {
    AX_U8 nBodyTargetCount;
    AX_U8 nVehicleTargetCount;
    AX_U8 nCycleTargetCount;
} AX_SKEL_MAX_TARGET_COUNT_T;

typedef struct axSKEL_FILTER_CONFIG_T {
    skel::infer::Size minSize;
    AX_F32 fConfidence;
} AX_SKEL_FILTER_CONFIG_T;

typedef struct axSKEL_PARAM_T {
    AX_SKEL_PPL_E ePPL;
    AX_U32 nNpuType;
    AX_U32 nFrameDepth;
    AX_U32 nWidth;
    AX_U32 nHeight;
    AX_U32 nFrameCacheDepth;
    AX_U32 nIoDepth;
    AX_F32 fCropEncoderQpLevel;
    AX_BOOL bPushBindEnable;
    AX_BOOL bTrackEnable;
    AX_BOOL bPushEnable;
    AX_SKEL_ROI_CONFIG_T stRoi;
    AX_SKEL_PUSH_STRATEGY_T stPushStrategy;
    AX_SKEL_MAX_TARGET_COUNT_T stMaxTargetCount;
    AX_SKEL_CROP_ENCODER_THRESHOLD_CONFIG_T stCropEncoderThreshold;
    AX_SKEL_RESIZE_CONFIG_T stPanoramaResizeConfig;
    AX_SKEL_PUSH_PANORAMA_CONFIG_T stPushPanoramaConfig;
    std::unordered_map<std::string, AX_SKEL_FILTER_CONFIG_T> stFilterMaps;
    std::unordered_map<std::string, AX_SKEL_ATTR_FILTER_CONFIG_T> stAttrFliterMaps;
    std::vector<std::string> stWantClasses;

    axSKEL_PARAM_T() {
        ePPL = AX_SKEL_PPL_BODY;
        nNpuType = 0;
        nFrameDepth = DEFAULT_FRAME_DEPTH;
        nWidth = DEFAULT_SRC_H;
        nHeight = DEFAULT_SRC_W;
        nFrameCacheDepth = DEFAULT_FRAME_CACHE_DEPTH;
        nIoDepth = 0;
        fCropEncoderQpLevel = DEFAULT_QPLEVEL;
        bPushBindEnable = AX_TRUE;
        bTrackEnable = AX_TRUE;
        bPushEnable = AX_TRUE;
        memset(&stRoi, 0x00, sizeof(stRoi));
        memset(&stMaxTargetCount, 0x00, sizeof(stMaxTargetCount));
        memset(&stCropEncoderThreshold, 0x00, sizeof(stCropEncoderThreshold));
        memset(&stPanoramaResizeConfig, 0x00, sizeof(stPanoramaResizeConfig));
        memset(&stPushPanoramaConfig, 0x00, sizeof(stPushPanoramaConfig));
        stFilterMaps["body"] = {skel::infer::Size(), DEFAULT_BODY_PROB_THRESHOLD};
        stFilterMaps["vehicle"] = {skel::infer::Size(), DEFAULT_VEHICLE_PROB_THRESHOLD};
        stFilterMaps["cycle"] = {skel::infer::Size(), DEFAULT_CYCLE_PROB_THRESHOLD};
        stFilterMaps["face"] = {skel::infer::Size(), DEFAULT_FACE_PROB_THRESHOLD};
        stFilterMaps["plate"] = {skel::infer::Size(), DEFAULT_PLATE_PROB_THRESHOLD};
        stPushStrategy.ePushMode = AX_SKEL_PUSH_MODE_BEST;
        stPushStrategy.nIntervalTimes = 2000;
        stPushStrategy.nPushCounts = 1;
        stPushStrategy.bPushSameFrame = AX_FALSE;
        stAttrFliterMaps["body"].stCommonAttrFilterConfig.fQuality = 0;
        stAttrFliterMaps["vehicle"].stCommonAttrFilterConfig.fQuality = 0;
        stAttrFliterMaps["cycle"].stCommonAttrFilterConfig.fQuality = 0;
        stAttrFliterMaps["face"].stFaceAttrFilterConfig.nWidth = 0;
        stAttrFliterMaps["face"].stFaceAttrFilterConfig.nHeight = 0;
        stAttrFliterMaps["face"].stFaceAttrFilterConfig.stPoseblur.fPitch = 180;
        stAttrFliterMaps["face"].stFaceAttrFilterConfig.stPoseblur.fYaw = 180;
        stAttrFliterMaps["face"].stFaceAttrFilterConfig.stPoseblur.fRoll = 180;
        stAttrFliterMaps["face"].stFaceAttrFilterConfig.stPoseblur.fBlur = 1.0;
        stAttrFliterMaps["plate"].stCommonAttrFilterConfig.fQuality = 0;
    }
} AX_SKEL_PARAM_T;

#endif //SKEL_AX_SKEL_DEF_H

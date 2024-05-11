/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/
#include "pipeline_hvcfp.h"

#include "utils/checker.h"
#include "utils/config_util.h"

#include "mgr/model_mgr.h"
#include "mgr/mem_mgr.h"

#include "ax_skel_api.h"

#include <algorithm>

using namespace std;
using namespace skel::tracker;
using namespace skel::utils;

static std::vector<std::string> HVCFP_CLASS_NAMES = {"body", "vehicle", "cycle", "plate"};

AX_S32 skel::ppl::PipelineHVCFP::Init(const AX_SKEL_HANDLE_PARAM_T *pstParam) {
    CHECK_PTR(pstParam);

    DealWithParams(pstParam);
    SetConfig(&pstParam->stConfig);

    AX_S32 ret = InitDetector();
    if (AX_SKEL_SUCC != ret) {
        ALOGE("Init detector failed!\n");
        return ret;
    }

    ret = InitTracker();
    if (AX_SKEL_SUCC != ret) {
        ALOGE("Init tracker failed!\n");
        return ret;
    }

    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineHVCFP::DeInit() {
    m_input_queue.Close();
    m_detect_result_queue.Close();
    m_track_result_queue.Close();

    m_detector.Release();
    if (m_tracker_dealer)
        delete m_tracker_dealer;
    FreeConfig(m_pstApiConfig);

    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineHVCFP::GetConfig(const AX_SKEL_CONFIG_T **ppstConfig) {
    if (m_pstApiConfig == nullptr)
    {
        m_pstApiConfig = (AX_SKEL_CONFIG_T*)malloc(sizeof(AX_SKEL_CONFIG_T));
        memset(m_pstApiConfig, 0, sizeof(AX_SKEL_CONFIG_T));
    }

    MakeConfig(m_pstApiConfig, "track_disable", m_config.track_disable);
    MakeConfig(m_pstApiConfig, "push_disable", m_config.push_disable);

    *ppstConfig = m_pstApiConfig;

    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineHVCFP::SetConfig(const AX_SKEL_CONFIG_T *pstConfig) {
    if (pstConfig && pstConfig->nSize > 0) {
        for (int i = 0; i < pstConfig->nSize; i++) {
            if (ParseConfig(pstConfig->pstItems[i], "track_disable", m_config.track_disable)) {
                ALOGD("track_disable: %d\n", m_config.track_disable);
                m_result_constrain.bTrackEnable = m_config.track_disable ? AX_FALSE : AX_TRUE;
            }

            if (ParseConfig(pstConfig->pstItems[i], "push_disable", m_config.push_disable)) {
                ALOGD("push_disable: %d\n", m_config.push_disable);
                m_result_constrain.bPushEnable = m_config.push_disable ? AX_FALSE : AX_TRUE;
            }

            ParseConfigCopy(pstConfig->pstItems[i], "push_strategy", m_result_constrain.stPushStrategy);
            ParseConfig(pstConfig->pstItems[i], "target_config", m_result_constrain.stWantClasses);

            ParseConfig(pstConfig->pstItems[i], "body_max_target_count", m_result_constrain.stMaxTargetCount.nBodyTargetCount);
            ParseConfig(pstConfig->pstItems[i], "vehicle_max_target_count", m_result_constrain.stMaxTargetCount.nVehicleTargetCount);
            ParseConfig(pstConfig->pstItems[i], "cycle_max_target_count", m_result_constrain.stMaxTargetCount.nCycleTargetCount);

            ParseConfig(pstConfig->pstItems[i], "body_confidence", m_result_constrain.stFilterMaps["body"].fConfidence);
            ParseConfig(pstConfig->pstItems[i], "face_confidence", m_result_constrain.stFilterMaps["face"].fConfidence);
            ParseConfig(pstConfig->pstItems[i], "vehicle_confidence", m_result_constrain.stFilterMaps["vehicle"].fConfidence);
            ParseConfig(pstConfig->pstItems[i], "cycle_confidence", m_result_constrain.stFilterMaps["cycle"].fConfidence);
            ParseConfig(pstConfig->pstItems[i], "plate_confidence", m_result_constrain.stFilterMaps["plate"].fConfidence);

            ParseConfig(pstConfig->pstItems[i], "body_min_size", m_result_constrain.stFilterMaps["body"].minSize);
            ParseConfig(pstConfig->pstItems[i], "face_min_size", m_result_constrain.stFilterMaps["face"].minSize);
            ParseConfig(pstConfig->pstItems[i], "vehicle_min_size", m_result_constrain.stFilterMaps["vehicle"].minSize);
            ParseConfig(pstConfig->pstItems[i], "cycle_min_size", m_result_constrain.stFilterMaps["cycle"].minSize);
            ParseConfig(pstConfig->pstItems[i], "plate_min_size", m_result_constrain.stFilterMaps["plate"].minSize);

            ParseConfigCopy(pstConfig->pstItems[i], "detect_roi", m_result_constrain.stRoi);

            ParseConfigCopy(pstConfig->pstItems[i], "crop_encoder", m_result_constrain.stCropEncoderThreshold);
            ParseConfig(pstConfig->pstItems[i], "crop_encoder_qpLevel", m_result_constrain.fCropEncoderQpLevel);

            ParseConfigCopy(pstConfig->pstItems[i], "resize_panorama_encoder_config", m_result_constrain.stPanoramaResizeConfig);
            ParseConfigCopy(pstConfig->pstItems[i], "push_panorama", m_result_constrain.stPushPanoramaConfig);

            ParseConfigCopy(pstConfig->pstItems[i], "push_quality_body", m_result_constrain.stAttrFliterMaps["body"]);
            ParseConfigCopy(pstConfig->pstItems[i], "push_quality_vehicle", m_result_constrain.stAttrFliterMaps["face"]);
            ParseConfigCopy(pstConfig->pstItems[i], "push_quality_cycle", m_result_constrain.stAttrFliterMaps["vehicle"]);
            ParseConfigCopy(pstConfig->pstItems[i], "push_quality_face", m_result_constrain.stAttrFliterMaps["cycle"]);
            ParseConfigCopy(pstConfig->pstItems[i], "push_quality_plate", m_result_constrain.stAttrFliterMaps["plate"]);
        }
    }
    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineHVCFP::GetResult(AX_SKEL_RESULT_T **ppstResult, AX_S32 nTimeout) {
    AX_S32 ret = AX_SKEL_SUCC;

    if (m_config.track_disable) {
        ret = GetDetectResult(ppstResult, nTimeout);
    }
    else {
        ret = GetTrackResult(ppstResult, nTimeout);
    }

    return ret;
}

AX_S32 skel::ppl::PipelineHVCFP::Run() {
    AX_S32 ret = AX_SKEL_SUCC;
    AX_SKEL_FRAME_T *frame = nullptr;

    ret = m_input_queue.Pop(frame);
    if (AX_SKEL_SUCC != ret) {
        if (AX_ERR_SKEL_UNEXIST == ret) {
            ALOGW("pipeline will be closed.\n");
            return ret;
        }
        ALOGE("pop failed! ret=0x%x\n", ret);
        return ret;
    }

    DetQueueType det_queue_item;
    det_queue_item.pstFrame = frame;
    ret = m_detector.Detect(frame->stFrame, det_queue_item.detResult);
    if (AX_SKEL_SUCC != ret) {
        ALOGE("Detect failed! ret = 0x%x\n", ret);
        return ret;
    }

    FilterDetResult(det_queue_item.detResult);

    if (m_config.track_disable) {
        ret = m_detect_result_queue.Push(det_queue_item);
        if (AX_SKEL_SUCC != ret) {
            ALOGE("push failed! ret=0x%x\n", ret);
            return ret;
        }
    }
    else {
        if (!m_tracker_dealer && !m_config.push_disable) {
            ALOGD("Init tracker dealer\n");
            m_tracker_dealer = new TrackerDealer(m_result_constrain);
        }

        TrackQueueType track_queue_item;
        track_queue_item.pstFrame = frame;
        track_queue_item.trackResult = m_tracker.Update(frame, det_queue_item.detResult);

        FilterTrackResult(track_queue_item.trackResult);

        if (m_callback) {
            AX_SKEL_RESULT_T *pstResult = nullptr;
            ConvertTrackResult(track_queue_item.pstFrame, track_queue_item.trackResult, &pstResult);
            m_callback((AX_SKEL_HANDLE)this, pstResult, m_userData);
            FreeResult(pstResult);
            utils::FreeFrame(frame);
        }
        else {
            if (m_track_result_queue.IsFull()) {
                TrackQueueType pop_item;
                m_track_result_queue.Pop(pop_item, 0);
            }
            ret = m_track_result_queue.Push(track_queue_item);
            if (AX_SKEL_SUCC != ret) {
                ALOGE("push failed! ret=0x%x\n", ret);
                return ret;
            }
        }
    }

    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineHVCFP::GetDetectResult(AX_SKEL_RESULT_T **ppstResult, AX_S32 nTimeout) {
    AX_S32 ret = AX_SKEL_SUCC;

    DetQueueType queue_item;
    ret = m_detect_result_queue.Pop(queue_item, nTimeout);
    if (AX_SKEL_SUCC != ret) {
        if (AX_ERR_SKEL_UNEXIST == ret) {
            ALOGW("pipeline will be closed.\n");
            return ret;
        }
        ALOGE("pop failed! ret=0x%x\n", ret);
        return ret;
    }

    *ppstResult = (AX_SKEL_RESULT_T*)malloc(sizeof(AX_SKEL_RESULT_T));
    AX_SKEL_RESULT_T* dst = *ppstResult;
    memset(dst, 0, sizeof(AX_SKEL_RESULT_T));

    auto *pstFrame = queue_item.pstFrame;
    auto& detect_result = queue_item.detResult;
    dst->nOriginalHeight = m_originSize[0];
    dst->nOriginalWidth = m_originSize[1];
    dst->nFrameId = pstFrame->nFrameId;
    dst->nStreamId = pstFrame->nStreamId;
    dst->pUserData = pstFrame->pUserData;

    if (!detect_result.empty()) {
        dst->nObjectSize = (int)detect_result.size();
        dst->pstObjectItems = (AX_SKEL_OBJECT_ITEM_T*)malloc(dst->nObjectSize * sizeof(AX_SKEL_OBJECT_ITEM_T));
        memset(dst->pstObjectItems, 0, dst->nObjectSize * sizeof(AX_SKEL_OBJECT_ITEM_T));
        AX_SKEL_OBJECT_ITEM_T *pstItems = dst->pstObjectItems;
        for (int i = 0; i < dst->nObjectSize; i++) {
            pstItems[i].nFrameId = pstFrame->nFrameId;

            pstItems[i].fConfidence = detect_result[i].prob;
            pstItems[i].stRect.fX = detect_result[i].rect.x;
            pstItems[i].stRect.fY = detect_result[i].rect.y;
            pstItems[i].stRect.fW = detect_result[i].rect.width;
            pstItems[i].stRect.fH = detect_result[i].rect.height;
            pstItems[i].pstrObjectCategory = HVCFP_CLASS_NAMES[detect_result[i].label].c_str();
        }
    }

    utils::FreeFrame(pstFrame);

    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineHVCFP::GetTrackResult(AX_SKEL_RESULT_T **ppstResult, AX_S32 nTimeout) {
    AX_S32 ret = AX_SKEL_SUCC;

    TrackQueueType queue_item;
    ret = m_track_result_queue.Pop(queue_item, nTimeout);
    if (AX_SKEL_SUCC != ret) {
        if (AX_ERR_SKEL_UNEXIST == ret) {
            ALOGW("pipeline will be closed.\n");
            return ret;
        }
        ALOGE("pop failed! ret=0x%x\n", ret);
        return ret;
    }

    ConvertTrackResult(queue_item.pstFrame, queue_item.trackResult, ppstResult);

    utils::FreeFrame(queue_item.pstFrame);

    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineHVCFP::ResultCallbackThread() {
    AX_S32 ret = AX_SKEL_SUCC;
    AX_SKEL_RESULT_T *result = nullptr;
    ret = GetResult(&result, -1);
    if (AX_SKEL_SUCC != ret) {
        if (AX_ERR_SKEL_UNEXIST == ret) {
            ALOGW("pipeline will be closed.\n");
            return ret;
        }
        ALOGE("GetResult failed! ret=0x%x\n", ret);
        return ret;
    }

    m_callback((AX_SKEL_HANDLE)this, result, m_userData);

    FreeResult(result);

    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineHVCFP::DealWithParams(const AX_SKEL_HANDLE_PARAM_T *pstParam) {
    m_stHandleParam = *pstParam;

    if (m_stHandleParam.nHeight > 0)
        m_originSize[0] = m_stHandleParam.nHeight;

    if (m_stHandleParam.nWidth > 0)
        m_originSize[1] = m_stHandleParam.nWidth;

    if (m_originSize[0] == 0 || m_originSize[1] == 0)
        ALOGW("origin size not set in param, will use frame size in SendFrame");

    if (m_stHandleParam.nFrameDepth > 0) {
//        m_stHandleParam.nFrameDepth = m_stHandleParam.nFrameDepth * 4;
        ALOGD("Set capacity: %d\n", m_stHandleParam.nFrameDepth);
        m_input_queue.SetCapacity(m_stHandleParam.nFrameDepth);
        m_detect_result_queue.SetCapacity(m_stHandleParam.nFrameDepth);
        m_track_result_queue.SetCapacity(m_stHandleParam.nFrameDepth);
    }

    m_result_constrain.nFrameCacheDepth = m_stHandleParam.nFrameCacheDepth;
    m_result_constrain.stWantClasses = HVCFP_CLASS_NAMES;

    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineHVCFP::InitDetector() {
    MODEL_INFO_T model_info;
    std::string model_keyword = PipelineModelBindings.at(m_stHandleParam.ePPL)[0];
    if (!MODELMGR->Find(model_keyword, model_info)) {
        ALOGE("Find model %s failed!\n", model_keyword.c_str());
        return AX_ERR_SKEL_ILLEGAL_PARAM;
    }

    AX_S32 ret = m_detector.Init(model_info.path, m_stHandleParam.nNpuType);
    if (ret != AX_SKEL_SUCC) {
        ALOGE("Init detector %s failed!\n", model_info.path.c_str());
        return AX_ERR_SKEL_ILLEGAL_PARAM;
    }

    return AX_SKEL_SUCC;
}

AX_S32 skel::ppl::PipelineHVCFP::InitTracker() {
    m_tracker_config.n_classes = HVCFP_CLASS_NAMES.size();
    m_tracker.Init(m_tracker_config);

    return AX_SKEL_SUCC;
}

AX_VOID skel::ppl::PipelineHVCFP::FilterDetResult(vector<skel::detection::Object> &detResult) {
    // want classes
    if (!m_result_constrain.stWantClasses.empty()) {
        for (auto it = detResult.begin(); it != detResult.end();) {
            string strLabel = HVCFP_CLASS_NAMES[it->label];
            if (std::find(m_result_constrain.stWantClasses.begin(), m_result_constrain.stWantClasses.end(), strLabel)
                    == m_result_constrain.stWantClasses.end()) {
                it = detResult.erase(it);
            } else {
                it++;
            }
        }
    }

    // ROI
    if (m_result_constrain.stRoi.bEnable) {
        float roi_x1 = m_result_constrain.stRoi.stRect.fX;
        float roi_y1 = m_result_constrain.stRoi.stRect.fY;
        float roi_x2 = roi_x1 + m_result_constrain.stRoi.stRect.fW;
        float roi_y2 = roi_y1 + m_result_constrain.stRoi.stRect.fH;
        for (auto it = detResult.begin(); it != detResult.end();) {
            float x1 = it->rect.x;
            float y1 = it->rect.y;
            float x2 = it->rect.x + it->rect.width;
            float y2 = it->rect.y + it->rect.height;
            if (x1 >= roi_x1 && x2 <= roi_x2 && y1 >= roi_y1 && y2 <= roi_y2) {
                it++;
            } else {
                it = detResult.erase(it);
            }
        }
    }

    // size and confidence
    for (auto it = detResult.begin(); it != detResult.end(); ) {
        string strLabel = HVCFP_CLASS_NAMES[it->label];
        skel::infer::Size min_size = m_result_constrain.stFilterMaps[strLabel].minSize;
        float conf_thresh = m_result_constrain.stFilterMaps[strLabel].fConfidence;
        if (it->rect.width < min_size.width || it->rect.height < min_size.height ||
            it->prob < conf_thresh) {
            it = detResult.erase(it);
        } else {
            it++;
        }
    }

    // max target count
    int nBodyCount = 0;
    int nVehicleCount = 0;
    int nCycleCount = 0;
    for (auto it = detResult.begin(); it != detResult.end(); ) {
        string strLabel = HVCFP_CLASS_NAMES[it->label];
        if (strLabel == "body") {
            nBodyCount++;
            if (m_result_constrain.stMaxTargetCount.nBodyTargetCount > 0 &&
                    nBodyCount > m_result_constrain.stMaxTargetCount.nBodyTargetCount) {
                it = detResult.erase(it);
                continue;
            }
        }

        if (strLabel == "vehicle") {
            nVehicleCount++;
            if (m_result_constrain.stMaxTargetCount.nVehicleTargetCount > 0 &&
                    nVehicleCount > m_result_constrain.stMaxTargetCount.nVehicleTargetCount) {
                it = detResult.erase(it);
                continue;
            }
        }

        if (strLabel == "cycle") {
            nCycleCount++;
            if (m_result_constrain.stMaxTargetCount.nCycleTargetCount > 0 &&
                    nCycleCount > m_result_constrain.stMaxTargetCount.nCycleTargetCount) {
                it = detResult.erase(it);
                continue;
            }
        }

        it++;
    }
}

AX_VOID skel::ppl::PipelineHVCFP::FilterTrackResult(tracker::TrackResultType& trackResult) {
    // max target count
    int nBodyCount = 0;
    int nVehicleCount = 0;
    int nCycleCount = 0;
    for (auto outer_it = trackResult.begin(); outer_it != trackResult.end(); outer_it++) {
        auto& output_tracks = outer_it->second;
        for (auto it = output_tracks.begin(); it != output_tracks.end(); ) {
            AX_U32 class_id = (*it)->class_id;
            string strLabel = HVCFP_CLASS_NAMES[class_id];
            if (strLabel == "body") {
                nBodyCount++;
                if (m_result_constrain.stMaxTargetCount.nBodyTargetCount > 0 &&
                    nBodyCount > m_result_constrain.stMaxTargetCount.nBodyTargetCount) {
                    it = output_tracks.erase(it);
                    continue;
                }
            }

            if (strLabel == "vehicle") {
                nVehicleCount++;
                if (m_result_constrain.stMaxTargetCount.nVehicleTargetCount > 0 &&
                    nVehicleCount > m_result_constrain.stMaxTargetCount.nVehicleTargetCount) {
                    it = output_tracks.erase(it);
                    continue;
                }
            }

            if (strLabel == "cycle") {
                nCycleCount++;
                if (m_result_constrain.stMaxTargetCount.nCycleTargetCount > 0 &&
                    nCycleCount > m_result_constrain.stMaxTargetCount.nCycleTargetCount) {
                    it = output_tracks.erase(it);
                    continue;
                }
            }

            it++;
        }
    }
}

AX_VOID skel::ppl::PipelineHVCFP::ConvertTrackResult(AX_SKEL_FRAME_T* pstFrame, const tracker::TrackResultType& trackResult, AX_SKEL_RESULT_T **ppstResult) {
    *ppstResult = (AX_SKEL_RESULT_T*)malloc(sizeof(AX_SKEL_RESULT_T));
    AX_SKEL_RESULT_T* dst = *ppstResult;
    memset(dst, 0, sizeof(AX_SKEL_RESULT_T));

    dst->nOriginalHeight = m_originSize[0];
    dst->nOriginalWidth = m_originSize[1];
    dst->nFrameId = pstFrame->nFrameId;
    dst->nStreamId = pstFrame->nStreamId;
    dst->pUserData = pstFrame->pUserData;

    vector<AX_SKEL_OBJECT_ITEM_T> vecResult;
    for (auto it = trackResult.begin(); it != trackResult.end(); it++) {
        const auto& output_tracks = it->second;
        for (AX_U32 i = 0; i < output_tracks.size(); ++ i) {
            const auto& obj = output_tracks[i];
            AX_U32 nClassId = obj->class_id;
            const AX_CHAR *pstrObjectCategory = HVCFP_CLASS_NAMES[nClassId].c_str();
            AX_SKEL_OBJECT_ITEM_T stObjectItem;
            memset(&stObjectItem, 0x00, sizeof(stObjectItem));

            if (obj->state == TrackState::New) {
                stObjectItem.eTrackState = AX_SKEL_TRACK_STATUS_NEW;
            }
            else if (obj->state == TrackState::Tracked) {
                stObjectItem.eTrackState = AX_SKEL_TRACK_STATUS_UPDATE;
            }
            else if (obj->state == TrackState::Removed) {
                stObjectItem.eTrackState = AX_SKEL_TRACK_STATUS_DIE;
            }
            else {
                continue;
            }

            stObjectItem.pstrObjectCategory = (const AX_CHAR *)pstrObjectCategory;
            stObjectItem.stRect.fX = (float)obj->_tlwh[0];
            stObjectItem.stRect.fY = (float)obj->_tlwh[1];
            stObjectItem.stRect.fW = (float)obj->_tlwh[2];
            stObjectItem.stRect.fH = (float)obj->_tlwh[3];
            stObjectItem.fConfidence = (float)obj->score;
            stObjectItem.nFrameId = obj->real_frame_id;

            // track
            stObjectItem.nTrackId = obj->track_id;

            if (!m_config.push_disable) {
                m_tracker_dealer->Update(pstFrame, stObjectItem);
            }

            vecResult.push_back(stObjectItem);
        }
    }

    if (!vecResult.empty()) {
        dst->nObjectSize = (int)vecResult.size();
        dst->pstObjectItems = (AX_SKEL_OBJECT_ITEM_T*)malloc(dst->nObjectSize * sizeof(AX_SKEL_OBJECT_ITEM_T));
        memcpy(dst->pstObjectItems, vecResult.data(), dst->nObjectSize * sizeof(AX_SKEL_OBJECT_ITEM_T));
    }

    if (!m_config.push_disable) {
        m_tracker_dealer->Finalize(pstFrame, dst, vecResult);
    }
}

AX_VOID skel::ppl::PipelineHVCFP::FreeResult(AX_SKEL_RESULT_T *pstResult) {
    if (!pstResult) return;

    if (pstResult->nObjectSize > 0) {
        for (int i = 0; i < pstResult->nObjectSize; i++) {
            if (pstResult->pstObjectItems[i].pstPointSet) {
                delete[] pstResult->pstObjectItems[i].pstPointSet;
            }
        }
        free(pstResult->pstObjectItems);
    }
    if (pstResult->nCacheListSize > 0) {
        delete[] pstResult->pstCacheList;
    }
    free(pstResult);
}
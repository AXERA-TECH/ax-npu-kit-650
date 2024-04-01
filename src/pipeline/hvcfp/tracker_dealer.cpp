/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <math.h>

#include "utils/io.hpp"
#include "tracker_dealer.h"
#include "utils/jenc.h"
#include "utils/logger.h"

using namespace std;

#define PUSH_DIE_FORCE_TIMEOUT 5000
#define STRATEGY_BEST_MIN_PUSH_COUNT 2

#define PUSH_JENC_REL(p) \
            if (p) { \
                JENCOBJ->Rel(p); \
                p = nullptr; \
            }

static std::unordered_map<std::string, AX_SKEL_CROP_ENCODER_THRESHOLD_CONFIG_T> s_stEncoderScaleDefaultMaps = {
        {"body", {0.1, 0.1, 0.1, 0.1}}, // => 1.2
        {"vehicle", {0.1, 0.1, 0.1, 0.1}}, // => 1.2
        {"cycle", {0.1, 0.1, 0.1, 0.1}}, // => 1.2
        {"face", {0.25, 0.25, 0.25, 0.25}}, // => 1.5
        {"plate", {1, 1, 1, 1}} // => 3
};

// Normalization
static AX_BOOL TrackEliminationStrategy(const AX_SKEL_FRAME_T *pstFrame, const AX_SKEL_OBJECT_ITEM_T &stObjectItemNew, const AX_SKEL_OBJECT_ITEM_T &stObjectItemOld) {
    // confidence look better
    if (stObjectItemNew.fConfidence > stObjectItemOld.fConfidence) {
        return AX_TRUE;
    }

    return AX_FALSE;
}

namespace skel {
    namespace utils {
        TrackerDealer::TrackerDealer(AX_SKEL_PARAM_T stParam) : m_stParam (stParam) {
            m_TrackMaps.clear();
            m_FrameMaps.clear();
            m_CacheListVec.clear();
        }

        TrackerDealer::~TrackerDealer(AX_VOID) {
            Statistics();

            ClearPush();
        }

        AX_S32 TrackerDealer::ClearPush(AX_VOID) {
            for (auto iterStream = m_TrackMaps.begin(); iterStream != m_TrackMaps.end();) {
                for (auto iter = iterStream->second.begin(); iter != iterStream->second.end();) {
                    // release jenc buffer
                    PUSH_JENC_REL(iter->second.stObjectItem.stCropFrame.pFrameData);

                    FrameDelete(iterStream->first, iter->second.stObjectItem.nFrameId, iter->first);

                    iterStream->second.erase(iter++);
                }

                m_TrackMaps.erase(iterStream++);
            }

            for (auto iterStream = m_FrameMaps.begin(); iterStream != m_FrameMaps.end();) {
                for (auto iter = iterStream->second.begin(); iter != iterStream->second.end();) {
                    // release jenc buffer
                    PUSH_JENC_REL(iter->second.stPanoraFrame.pFrameData);

                    if (!iter->second.stCacheFrame.bFrameDrop) {
                        dec_io_ref_cnt(iter->second.stCacheFrame.stFrame);
                        iter->second.stCacheFrame.bFrameDrop = AX_TRUE;
                    }

                    iter->second.liTrackList.clear();

                    iterStream->second.erase(iter++);
                }

                m_FrameMaps.erase(iterStream++);
            }

            m_TrackMaps.clear();
            m_FrameMaps.clear();
            m_CacheListVec.clear();

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::UpdateFast(const AX_SKEL_FRAME_T *pstFrame, AX_SKEL_OBJECT_ITEM_T &stObjectItem) {
            auto &trackMap = m_TrackMaps[pstFrame->nStreamId];
            auto &frameMap = m_FrameMaps[pstFrame->nStreamId];
            AX_U64 nTrackId = stObjectItem.nTrackId;
            auto nowTime = std::chrono::steady_clock::now();

            switch (stObjectItem.eTrackState) {
                case AX_SKEL_TRACK_STATUS_NEW:
                {
                    // new TrackId map
                    auto &stTrackMap = trackMap[nTrackId];

                    stTrackMap.stObjectItem = stObjectItem;
                    stTrackMap.stObjectItem.nPointSetSize = 0;
                    stTrackMap.stObjectItem.pstPointSet = nullptr;
                    stTrackMap.eTrackLastState = stObjectItem.eTrackState;
                    stTrackMap.updateTime = nowTime;

                    // new Frame map
                    auto &stFrameMap = frameMap[pstFrame->nFrameId];
                    stFrameMap.liTrackList.push_back(nTrackId);
                    stFrameMap.stCacheFrame.nFrameId = pstFrame->nFrameId;
                    stFrameMap.stCacheFrame.stFrame = pstFrame->stFrame;
                    stFrameMap.stCacheFrame.bFrameDrop = AX_FALSE;
                }
                    break;

                case AX_SKEL_TRACK_STATUS_UPDATE:
                {
                    AX_SKEL_CROP_FRAME_T stCropFrame;

                    if (!TrackFind(pstFrame->nStreamId, nTrackId, stCropFrame)) {
                        // not found, do nothing
                    }
                    else {
                        // push count
                        if (trackMap[nTrackId].nPushCounts >= m_stParam.stPushStrategy.nPushCounts) {
                            return TrackDelete(pstFrame->nStreamId, nTrackId, AX_TRUE);
                        }

                        // update track
                        if (TrackEliminationStrategy(pstFrame, stObjectItem, trackMap[nTrackId].stObjectItem)) {
                            AX_U32 nUpdateCounts = trackMap[nTrackId].nUpdateCounts;
                            AX_U32 nPushCounts = trackMap[nTrackId].nPushCounts;
                            std::chrono::steady_clock::time_point pushTime = trackMap[nTrackId].pushTime;

                            // new TrackId map
                            TrackDelete(pstFrame->nStreamId, nTrackId);

                            auto &stTrackMap = trackMap[nTrackId];
                            stTrackMap.stObjectItem = stObjectItem;
                            stTrackMap.stObjectItem.nPointSetSize = 0;
                            stTrackMap.stObjectItem.pstPointSet = nullptr;
                            stTrackMap.eTrackLastState = stObjectItem.eTrackState;
                            stTrackMap.nUpdateCounts = nUpdateCounts + 1;
                            stTrackMap.nPushCounts = nPushCounts;
                            stTrackMap.pushTime = pushTime;
                            stTrackMap.updateTime = nowTime;

                            // new Frame map
                            auto &stFrameMap = frameMap[pstFrame->nFrameId];
                            stFrameMap.liTrackList.push_back(nTrackId);
                            stFrameMap.stCacheFrame.nFrameId = pstFrame->nFrameId;
                            stFrameMap.stCacheFrame.stFrame = pstFrame->stFrame;
                            stFrameMap.stCacheFrame.bFrameDrop = AX_FALSE;
                        }
                        else {
                            trackMap[nTrackId].nUpdateCounts ++;
                            trackMap[nTrackId].eTrackLastState = stObjectItem.eTrackState;
                            trackMap[nTrackId].stObjectItem.eTrackState = stObjectItem.eTrackState;
                            trackMap[nTrackId].updateTime = nowTime;
                        }
                    }
                }
                    break;

                case AX_SKEL_TRACK_STATUS_DIE:
                {
                    return TrackDelete(pstFrame->nStreamId, nTrackId, AX_TRUE);
                }
                    break;

                default:
                    break;
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::UpdateInterval(const AX_SKEL_FRAME_T *pstFrame, AX_SKEL_OBJECT_ITEM_T &stObjectItem) {
            auto &trackMap = m_TrackMaps[pstFrame->nStreamId];
            auto &frameMap = m_FrameMaps[pstFrame->nStreamId];
            AX_U64 nTrackId = stObjectItem.nTrackId;
            auto nowTime = std::chrono::steady_clock::now();

            switch (stObjectItem.eTrackState) {
                case AX_SKEL_TRACK_STATUS_NEW:
                {
                    // new TrackId map
                    auto &stTrackMap = trackMap[nTrackId];
                    stTrackMap.stObjectItem = stObjectItem;
                    stTrackMap.stObjectItem.nPointSetSize = 0;
                    stTrackMap.stObjectItem.pstPointSet = nullptr;
                    stTrackMap.eTrackLastState = stObjectItem.eTrackState;
                    stTrackMap.updateTime = nowTime;
                    stTrackMap.pushTime = nowTime;

                    // new Frame map
                    auto &stFrameMap = frameMap[pstFrame->nFrameId];
                    stFrameMap.liTrackList.push_back(nTrackId);
                    stFrameMap.stCacheFrame.nFrameId = pstFrame->nFrameId;
                    stFrameMap.stCacheFrame.stFrame = pstFrame->stFrame;
                    stFrameMap.stCacheFrame.bFrameDrop = AX_FALSE;
                }
                    break;

                case AX_SKEL_TRACK_STATUS_UPDATE:
                {
                    AX_SKEL_CROP_FRAME_T stCropFrame;

                    if (!TrackFind(pstFrame->nStreamId, nTrackId, stCropFrame)) {
                        // not found, do nothing
                    }
                    else {
                        // push count
                        if (trackMap[nTrackId].nPushCounts >= m_stParam.stPushStrategy.nPushCounts) {
                            return TrackDelete(pstFrame->nStreamId, nTrackId, AX_TRUE);
                        }

                        // update track
                        if (TrackEliminationStrategy(pstFrame, stObjectItem, trackMap[nTrackId].stObjectItem)) {
                            AX_U32 nUpdateCounts = trackMap[nTrackId].nUpdateCounts;
                            AX_U32 nPushCounts = trackMap[nTrackId].nPushCounts;
                            std::chrono::steady_clock::time_point pushTime = trackMap[nTrackId].pushTime;

                            // new TrackId map
                            TrackDelete(pstFrame->nStreamId, nTrackId);

                            auto &stTrackMap = trackMap[nTrackId];
                            stTrackMap.stObjectItem = stObjectItem;
                            stTrackMap.stObjectItem.nPointSetSize = 0;
                            stTrackMap.stObjectItem.pstPointSet = nullptr;
                            stTrackMap.eTrackLastState = stObjectItem.eTrackState;
                            stTrackMap.nUpdateCounts = nUpdateCounts + 1;
                            stTrackMap.nPushCounts = nPushCounts;
                            stTrackMap.pushTime = pushTime;
                            stTrackMap.updateTime = nowTime;

                            // new Frame map
                            auto &stFrameMap = frameMap[pstFrame->nFrameId];
                            stFrameMap.liTrackList.push_back(nTrackId);
                            stFrameMap.stCacheFrame.nFrameId = pstFrame->nFrameId;
                            stFrameMap.stCacheFrame.stFrame = pstFrame->stFrame;
                            stFrameMap.stCacheFrame.bFrameDrop = AX_FALSE;
                        }
                        else {
                            trackMap[nTrackId].nUpdateCounts ++;
                            trackMap[nTrackId].eTrackLastState = stObjectItem.eTrackState;
                            trackMap[nTrackId].stObjectItem.eTrackState = stObjectItem.eTrackState;
                            trackMap[nTrackId].updateTime = nowTime;
                        }
                    }
                }
                    break;

                case AX_SKEL_TRACK_STATUS_DIE:
                {
                    return TrackDelete(pstFrame->nStreamId, nTrackId, AX_TRUE);
                }
                    break;

                default:
                    break;
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::UpdateBest(const AX_SKEL_FRAME_T *pstFrame, AX_SKEL_OBJECT_ITEM_T &stObjectItem) {
            auto &trackMap = m_TrackMaps[pstFrame->nStreamId];
            auto &frameMap = m_FrameMaps[pstFrame->nStreamId];
            AX_U64 nTrackId = stObjectItem.nTrackId;
            auto nowTime = std::chrono::steady_clock::now();

            switch (stObjectItem.eTrackState) {
                case AX_SKEL_TRACK_STATUS_NEW:
                {
                    // new TrackId map
                    auto &stTrackMap = trackMap[nTrackId];
                    stTrackMap.stObjectItem = stObjectItem;
                    stTrackMap.stObjectItem.nPointSetSize = 0;
                    stTrackMap.stObjectItem.pstPointSet = nullptr;
                    stTrackMap.eTrackLastState = stObjectItem.eTrackState;
                    stTrackMap.updateTime = nowTime;

                    // new Frame map
                    auto &stFrameMap = frameMap[pstFrame->nFrameId];
                    stFrameMap.liTrackList.push_back(nTrackId);
                    stFrameMap.stCacheFrame.nFrameId = pstFrame->nFrameId;
                    stFrameMap.stCacheFrame.stFrame = pstFrame->stFrame;
                    stFrameMap.stCacheFrame.bFrameDrop = AX_FALSE;
                }
                    break;

                case AX_SKEL_TRACK_STATUS_UPDATE:
                case AX_SKEL_TRACK_STATUS_DIE:
                {
                    AX_SKEL_CROP_FRAME_T stCropFrame;

                    if (!TrackFind(pstFrame->nStreamId, nTrackId, stCropFrame)) {
                        // not found, do nothing
                    }
                    else {
                        // update track
                        if (TrackEliminationStrategy(pstFrame, stObjectItem, trackMap[nTrackId].stObjectItem)) {
                            AX_U32 nUpdateCounts = trackMap[nTrackId].nUpdateCounts;

                            // new TrackId map
                            TrackDelete(pstFrame->nStreamId, nTrackId);

                            auto &stTrackMap = trackMap[nTrackId];
                            stTrackMap.stObjectItem = stObjectItem;
                            stTrackMap.stObjectItem.nPointSetSize = 0;
                            stTrackMap.stObjectItem.pstPointSet = nullptr;
                            stTrackMap.eTrackLastState = stObjectItem.eTrackState;
                            stTrackMap.nUpdateCounts = nUpdateCounts + 1;
                            stTrackMap.updateTime = nowTime;

                            // new Frame map
                            auto &stFrameMap = frameMap[pstFrame->nFrameId];
                            stFrameMap.liTrackList.push_back(nTrackId);
                            stFrameMap.stCacheFrame.nFrameId = pstFrame->nFrameId;
                            stFrameMap.stCacheFrame.stFrame = pstFrame->stFrame;
                            stFrameMap.stCacheFrame.bFrameDrop = AX_FALSE;
                        }
                        else {
                            trackMap[nTrackId].nUpdateCounts ++;
                            trackMap[nTrackId].eTrackLastState = stObjectItem.eTrackState;
                            trackMap[nTrackId].stObjectItem.eTrackState = stObjectItem.eTrackState;
                            trackMap[nTrackId].updateTime = nowTime;
                        }
                    }
                }
                    break;

                default:
                    break;
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::Update(const AX_SKEL_FRAME_T *pstFrame, AX_SKEL_OBJECT_ITEM_T &stObjectItem) {
            std::lock_guard<std::mutex> lck(m_mtxMaps);

            if (!pstFrame) {
                ALOGE("nil pointer");
                return AX_ERR_SKEL_NULL_PTR;
            }

            switch (m_stParam.stPushStrategy.ePushMode) {
                case AX_SKEL_PUSH_MODE_FAST:
                {
                    return UpdateFast(pstFrame, stObjectItem);
                }
                    break;

                case AX_SKEL_PUSH_MODE_INTERVAL:
                {
                    return UpdateInterval(pstFrame, stObjectItem);
                }
                    break;

                case AX_SKEL_PUSH_MODE_BEST:
                {
                    return UpdateBest(pstFrame, stObjectItem);
                }
                    break;

                default:
                    break;
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::Release(AX_U32 nStreamId, const AX_SKEL_OBJECT_ITEM_T &stObjectItem) {
            std::lock_guard<std::mutex> lck(m_mtxMaps);

            if (stObjectItem.bCropFrame) {
                TrackDelete(nStreamId, stObjectItem.nTrackId);
            }

            if (stObjectItem.bCropFrame) {
                FrameDelete(nStreamId, stObjectItem.stCropFrame.nFrameId, stObjectItem.nTrackId);
            }

            if (stObjectItem.bPanoraFrame) {
                FrameDelete(nStreamId, stObjectItem.stPanoraFrame.nFrameId, stObjectItem.nTrackId);
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::FinalizeFast(AX_U32 nStreamId, vector<AX_SKEL_OBJECT_ITEM_T> &vecObjectItem, vector<AX_SKEL_PUSH_CACHE_LIST_T> &dropCacheListVec) {
            AX_S32 nRet = AX_SKEL_SUCC;
            auto &trackMap = m_TrackMaps[nStreamId];
            auto &frameMap = m_FrameMaps[nStreamId];
            auto nowTime = std::chrono::steady_clock::now();

            // 2. push track
            for (auto iter = trackMap.begin(); iter != trackMap.end();) {
                auto &stTrackMap = iter->second;
                AX_SKEL_OBJECT_ITEM_T stObjectItem = stTrackMap.stObjectItem;

                switch (stObjectItem.eTrackState) {
                    case AX_SKEL_TRACK_STATUS_NEW:
                    {
                        if (!PushFliter(stObjectItem)
                            && stObjectItem.fConfidence > 0
                            && FrameFind(nStreamId, stObjectItem.nFrameId)
                            && stTrackMap.nPushCounts == 0) {
                            nRet = TrackPush(nStreamId, frameMap[stObjectItem.nFrameId].stCacheFrame, stObjectItem);

                            if (nRet == AX_SKEL_SUCC) {
                                stObjectItem.stRect = {0, 0, 0, 0};
                                vecObjectItem.push_back(stObjectItem);

                                // update push status
                                stTrackMap.nPushCounts ++;
                                stTrackMap.pushTime = nowTime;

                                // reset confidence
                                stTrackMap.stObjectItem.fConfidence = 0;
                            }
                            else {
                                // reset object confidence
                                stTrackMap.stObjectItem.fConfidence = 0;
                            }
                        }
                    }
                        break;

                    case AX_SKEL_TRACK_STATUS_UPDATE:
                    {
                        AX_S32 nElapsed = (AX_S32)(std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - stTrackMap.pushTime).count());

                        if (nElapsed >= (AX_S32)m_stParam.stPushStrategy.nIntervalTimes) {
                            if (!PushFliter(stObjectItem)
                                && stObjectItem.fConfidence > 0
                                && FrameFind(nStreamId, stObjectItem.nFrameId)) {
                                nRet = TrackPush(nStreamId, frameMap[stObjectItem.nFrameId].stCacheFrame, stObjectItem);

                                if (nRet == AX_SKEL_SUCC) {
                                    stObjectItem.stRect = {0, 0, 0, 0};
                                    vecObjectItem.push_back(stObjectItem);

                                    // update push status
                                    stTrackMap.nPushCounts ++;
                                    stTrackMap.pushTime = nowTime;

                                    // reset object confidence
                                    stTrackMap.stObjectItem.fConfidence = 0;
                                }
                                else {
                                    // reset object confidence
                                    stTrackMap.stObjectItem.fConfidence = 0;
                                }
                            }
                        }
                        else {
                            AX_U32 nDropCacheListSize = dropCacheListVec.size();

                            for (size_t i = 0; i < nDropCacheListSize; i++) {
                                // find
                                if (stTrackMap.stObjectItem.nFrameId == dropCacheListVec[i].nFrameId
                                    && FrameFind(nStreamId, stTrackMap.stObjectItem.nFrameId)) {
                                    nRet = TrackPush(nStreamId, frameMap[stTrackMap.stObjectItem.nFrameId].stCacheFrame, stTrackMap.stObjectItem);

                                    if (nRet != AX_SKEL_SUCC) {
                                        // reset object confidence
                                        stTrackMap.stObjectItem.fConfidence = 0;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                        break;

                    default:
                        break;
                }

                ++ iter;
            }

            // force drop track
            for (auto iter = trackMap.begin(); iter != trackMap.end();) {
                auto &stTrackMap = iter->second;

                AX_S32 nElapsed = (AX_S32)(std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - stTrackMap.updateTime).count());

                if (stTrackMap.eTrackLastState != AX_SKEL_TRACK_STATUS_DIE
                    && nElapsed > PUSH_DIE_FORCE_TIMEOUT) {
                    // release jenc buffer
                    PUSH_JENC_REL(stTrackMap.stObjectItem.stCropFrame.pFrameData);

                    FrameDelete(nStreamId, stTrackMap.stObjectItem.nFrameId, iter->first);

                    stTrackMap.stObjectItem.eTrackState = AX_SKEL_TRACK_STATUS_DIE;
                    vecObjectItem.push_back(stTrackMap.stObjectItem);

                    ALOGI("TRACK ID[%lld] force to drop, timeout(%d)", iter->first, nElapsed);

                    trackMap.erase(iter++);

                    continue;
                }

                ++ iter;
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::FinalizeInterval(AX_U32 nStreamId, vector<AX_SKEL_OBJECT_ITEM_T> &vecObjectItem, vector<AX_SKEL_PUSH_CACHE_LIST_T> &dropCacheListVec) {
            AX_S32 nRet = AX_SKEL_SUCC;
            auto &trackMap = m_TrackMaps[nStreamId];
            auto &frameMap = m_FrameMaps[nStreamId];
            auto nowTime = std::chrono::steady_clock::now();

            // 2. push track
            for (auto iter = trackMap.begin(); iter != trackMap.end();) {
                auto &stTrackMap = iter->second;
                AX_SKEL_OBJECT_ITEM_T stObjectItem = stTrackMap.stObjectItem;

                switch (stObjectItem.eTrackState) {
                    case AX_SKEL_TRACK_STATUS_NEW:
                    case AX_SKEL_TRACK_STATUS_UPDATE:
                    {
                        AX_S32 nElapsed = (AX_S32)(std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - stTrackMap.pushTime).count());

                        if (nElapsed >= (AX_S32)m_stParam.stPushStrategy.nIntervalTimes) {
                            if (!PushFliter(stObjectItem)
                                && stObjectItem.fConfidence > 0
                                && FrameFind(nStreamId, stObjectItem.nFrameId)) {
                                nRet = TrackPush(nStreamId, frameMap[stObjectItem.nFrameId].stCacheFrame, stObjectItem);

                                if (nRet == AX_SKEL_SUCC) {
                                    stObjectItem.stRect = {0, 0, 0, 0};
                                    vecObjectItem.push_back(stObjectItem);

                                    // update push status
                                    stTrackMap.nPushCounts ++;
                                    stTrackMap.pushTime = nowTime;

                                    // reset object confidence
                                    stTrackMap.stObjectItem.fConfidence = 0;
                                }
                                else {
                                    // reset object confidence
                                    stTrackMap.stObjectItem.fConfidence = 0;
                                }
                            }
                        }
                        else {
                            AX_U32 nDropCacheListSize = dropCacheListVec.size();

                            for (size_t i = 0; i < nDropCacheListSize; i++) {
                                // find
                                if (stTrackMap.stObjectItem.fConfidence > 0
                                    && stTrackMap.stObjectItem.nFrameId == dropCacheListVec[i].nFrameId
                                    && FrameFind(nStreamId, stTrackMap.stObjectItem.nFrameId)) {
                                    nRet = TrackPush(nStreamId, frameMap[stTrackMap.stObjectItem.nFrameId].stCacheFrame, stTrackMap.stObjectItem);

                                    if (nRet != AX_SKEL_SUCC) {
                                        // reset object confidence
                                        stTrackMap.stObjectItem.fConfidence = 0;
                                    }
                                    break;
                                }
                            }
                        }
                    }
                        break;

                    default:
                        break;
                }

                ++ iter;
            }

            // force drop track
            for (auto iter = trackMap.begin(); iter != trackMap.end();) {
                auto &stTrackMap = iter->second;

                AX_S32 nElapsed = (AX_S32)(std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - stTrackMap.updateTime).count());

                if (stTrackMap.eTrackLastState != AX_SKEL_TRACK_STATUS_DIE
                    && nElapsed > PUSH_DIE_FORCE_TIMEOUT) {
                    // release jenc buffer
                    PUSH_JENC_REL(stTrackMap.stObjectItem.stCropFrame.pFrameData);

                    FrameDelete(nStreamId, stTrackMap.stObjectItem.nFrameId, iter->first);

                    stTrackMap.stObjectItem.eTrackState = AX_SKEL_TRACK_STATUS_DIE;
                    vecObjectItem.push_back(stTrackMap.stObjectItem);

                    ALOGI("TRACK ID[%lld] force to drop, timeout(%d)", iter->first, nElapsed);

                    trackMap.erase(iter++);

                    continue;
                }

                ++ iter;
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::FinalizeBest(AX_U32 nStreamId, vector<AX_SKEL_OBJECT_ITEM_T> &vecObjectItem, vector<AX_SKEL_PUSH_CACHE_LIST_T> &dropCacheListVec) {
            AX_S32 nRet = AX_SKEL_SUCC;
            auto &trackMap = m_TrackMaps[nStreamId];
            auto &frameMap = m_FrameMaps[nStreamId];
            auto nowTime = std::chrono::steady_clock::now();

            // 2. push track die
            for (auto iter = trackMap.begin(); iter != trackMap.end();) {
                auto &stTrackMap = iter->second;

                AX_S32 nElapsed = (AX_S32)(std::chrono::duration_cast<std::chrono::milliseconds>(nowTime - stTrackMap.updateTime).count());

                if (nElapsed > PUSH_DIE_FORCE_TIMEOUT) {
                    stTrackMap.eTrackLastState = AX_SKEL_TRACK_STATUS_DIE;
                    ALOGI("TRACK ID[%lld] force to die, timeout(%d)", iter->first, nElapsed);
                }

                if (stTrackMap.eTrackLastState == AX_SKEL_TRACK_STATUS_DIE) {
                    if (stTrackMap.nUpdateCounts >= STRATEGY_BEST_MIN_PUSH_COUNT
                        && !PushFliter(stTrackMap.stObjectItem)
                        && FrameFind(nStreamId, stTrackMap.stObjectItem.nFrameId)) {
                        nRet = TrackPush(nStreamId, frameMap[stTrackMap.stObjectItem.nFrameId].stCacheFrame, stTrackMap.stObjectItem);

                        if (nRet == AX_SKEL_SUCC) {
                            // update push status
                            stTrackMap.nPushCounts ++;
                            stTrackMap.pushTime = nowTime;

                            memset(&stTrackMap.stObjectItem.stRect, 0x00, sizeof(stTrackMap.stObjectItem.stRect));
                            vecObjectItem.push_back(stTrackMap.stObjectItem);
                        }
                        else {
                            // release jenc buffer
                            PUSH_JENC_REL(stTrackMap.stObjectItem.stCropFrame.pFrameData);

                            FrameDelete(nStreamId, stTrackMap.stObjectItem.nFrameId, iter->first);

                            vecObjectItem.push_back(stTrackMap.stObjectItem);

                            trackMap.erase(iter++);
                            continue;
                        }
                    }
                    else {
                        // release jenc buffer
                        PUSH_JENC_REL(stTrackMap.stObjectItem.stCropFrame.pFrameData);

                        FrameDelete(nStreamId, stTrackMap.stObjectItem.nFrameId, iter->first);

                        vecObjectItem.push_back(stTrackMap.stObjectItem);

                        trackMap.erase(iter++);
                        continue;
                    }
                }
                else if (stTrackMap.eTrackLastState == AX_SKEL_TRACK_STATUS_NEW
                         || stTrackMap.eTrackLastState == AX_SKEL_TRACK_STATUS_UPDATE) {
                    AX_U32 nDropCacheListSize = dropCacheListVec.size();

                    for (size_t i = 0; i < nDropCacheListSize; i++) {
                        // find
                        if (stTrackMap.stObjectItem.nFrameId == dropCacheListVec[i].nFrameId
                            && FrameFind(nStreamId, stTrackMap.stObjectItem.nFrameId)) {
                            nRet = TrackPush(nStreamId, frameMap[stTrackMap.stObjectItem.nFrameId].stCacheFrame, stTrackMap.stObjectItem);

                            if (nRet != AX_SKEL_SUCC) {
                                // reset object confidence
                                stTrackMap.stObjectItem.fConfidence = 0;
                            }
                            break;
                        }
                    }
                }

                ++ iter;
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::Finalize(const AX_SKEL_FRAME_T *pstFrame, AX_SKEL_RESULT_T *pstResult, vector<AX_SKEL_OBJECT_ITEM_T> &vecObjectItem) {
            std::lock_guard<std::mutex> lck(m_mtxMaps);
            auto &frameMap = m_FrameMaps[pstFrame->nStreamId];
            auto &cacheListVec = m_CacheListVec[pstFrame->nStreamId];

            vector<AX_SKEL_PUSH_CACHE_LIST_T> tmpCacheListVec = cacheListVec;

            // 1. insert new cache list
            cacheListVec.clear();

            for (auto iter = frameMap.begin(); iter != frameMap.end();) {
                AX_SKEL_FRAME_T stFrame;
                stFrame.nFrameId = iter->second.stCacheFrame.nFrameId;
                stFrame.nStreamId = pstFrame->nStreamId;
                stFrame.stFrame = iter->second.stCacheFrame.stFrame;
                CachePush(&stFrame);

                ++ iter;
            }

            vector<AX_SKEL_PUSH_CACHE_LIST_T> dropCacheListVec;

            AX_U32 nTmpCacheListSize = tmpCacheListVec.size();
            AX_U32 nCacheListSize = cacheListVec.size();

            for (size_t i = 0; i < nTmpCacheListSize; i++) {
                AX_BOOL bDrop = AX_TRUE;

                for (size_t j = 0; j < nCacheListSize; j++) {
                    if (tmpCacheListVec[i].nFrameId == cacheListVec[j].nFrameId) {
                        bDrop = AX_FALSE;
                        break;
                    }
                }

                if (bDrop) {
                    dropCacheListVec.push_back(tmpCacheListVec[i]);
                }
            }

            // 2. Finalize track
            switch (m_stParam.stPushStrategy.ePushMode) {
                case AX_SKEL_PUSH_MODE_FAST:
                {
                    FinalizeFast(pstFrame->nStreamId, vecObjectItem, dropCacheListVec);
                }
                    break;

                case AX_SKEL_PUSH_MODE_INTERVAL:
                {
                    FinalizeInterval(pstFrame->nStreamId, vecObjectItem, dropCacheListVec);
                }
                    break;

                case AX_SKEL_PUSH_MODE_BEST:
                {
                    FinalizeBest(pstFrame->nStreamId, vecObjectItem, dropCacheListVec);
                }
                    break;

                default:
                    break;
            }

            // 3. clear drop list ref cnt
            for (auto iter = frameMap.begin(); iter != frameMap.end();) {
                if (!iter->second.stCacheFrame.bFrameDrop) {
                    AX_BOOL bFound = AX_FALSE;
                    for (size_t i = 0; i < nCacheListSize; i++) {
                        // found
                        if (iter->second.stCacheFrame.nFrameId == cacheListVec[i].nFrameId) {
                            bFound = AX_TRUE;
                            break;
                        }
                    }

                    if (!bFound) {
                        dec_io_ref_cnt(iter->second.stCacheFrame.stFrame);
                        iter->second.stCacheFrame.bFrameDrop = AX_TRUE;
                    }
                }

                ++ iter;
            }

            AX_BOOL bFound = AX_FALSE;
            for (size_t i = 0; i < nCacheListSize; i++) {
                // found
                if (pstFrame->nFrameId == cacheListVec[i].nFrameId) {
                    bFound = AX_TRUE;
                    break;
                }
            }

            if (!bFound) {
                dec_io_ref_cnt(pstFrame->stFrame);
            }

            // 4. set cache list
            vector<AX_SKEL_FRAME_CACHE_LIST_T> cacheList;

            for (auto iter = m_CacheListVec.begin(); iter != m_CacheListVec.end();) {
                for (size_t i = 0;  i < iter->second.size(); i++) {
                    AX_SKEL_FRAME_CACHE_LIST_T cacheItem = {0};
                    cacheItem.nFrameId = iter->second[i].nFrameId;
                    cacheItem.nStreamId = iter->first;

                    cacheList.push_back(cacheItem);
                }

                ++ iter;
            }

            nCacheListSize = cacheList.size();
            pstResult->pstCacheList = new AX_SKEL_FRAME_CACHE_LIST_T[nCacheListSize];

            if (!pstResult->pstCacheList) {
                ALOGW("SKEL alloc cache list fail");
                return AX_ERR_SKEL_NOMEM;
            }

            memcpy(pstResult->pstCacheList, cacheList.data(), sizeof(AX_SKEL_FRAME_CACHE_LIST_T) * nCacheListSize);

            pstResult->nCacheListSize = nCacheListSize;

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::CachePush(const AX_SKEL_FRAME_T *pstFrame) {
            std::lock_guard<std::mutex> lck(m_mtxSet);

            AX_U32 nFrameCacheDepth = m_stParam.nFrameCacheDepth;
            auto &cacheListVec = m_CacheListVec[pstFrame->nStreamId];

            if (m_stParam.stPushStrategy.ePushMode == AX_SKEL_PUSH_MODE_FAST
                || m_stParam.stPushStrategy.ePushMode == AX_SKEL_PUSH_MODE_INTERVAL) {
                nFrameCacheDepth = 1;
            }

            AX_BOOL bInsertBack = AX_TRUE;
            AX_SKEL_PUSH_CACHE_LIST_T stFrameCache;
            stFrameCache.nFrameId = pstFrame->nFrameId;
            stFrameCache.stFrame = pstFrame->stFrame;

            AX_U32 nCacheListSize = cacheListVec.size();

            if (nCacheListSize > 0) {
                for (size_t i = 0; i < nCacheListSize; i++) {
                    if (pstFrame->nFrameId > cacheListVec[i].nFrameId) {
                        bInsertBack = AX_FALSE;
                        cacheListVec.emplace(cacheListVec.begin() + i, stFrameCache);
                        break;
                    }
                    else if (pstFrame->nFrameId < cacheListVec[i].nFrameId) {
                        bInsertBack = AX_TRUE;
                    }
                }
            }

            if (bInsertBack) {
                cacheListVec.push_back(stFrameCache);
            }

            while (cacheListVec.size() > 0
                   && cacheListVec.size() > nFrameCacheDepth) {
                cacheListVec.pop_back();
            }

            return AX_SKEL_SUCC;
        }

        AX_BOOL TrackerDealer::PushFliter(AX_SKEL_OBJECT_ITEM_T &stObjectItem) {
            AX_SKEL_PARAM_T stParam;
            std::string strObjectCategory = stObjectItem.pstrObjectCategory;

            {
                std::unique_lock<std::mutex> lck(m_mtxSet);
                stParam = m_stParam;
            }

            vector<float> tlwh;
            tlwh.resize(4);
            tlwh[0] = stObjectItem.stRect.fX;
            tlwh[1] = stObjectItem.stRect.fY;
            tlwh[2] = stObjectItem.stRect.fW;
            tlwh[3] = stObjectItem.stRect.fH;

            // check roi
            if (stParam.stRoi.bEnable) {
                float x1 = (float)tlwh[0];
                float y1 = (float)tlwh[1];
                float x2 = x1 + (float)tlwh[2];
                float y2 = y1 + (float)tlwh[3];

                float x1_roi = (float)stParam.stRoi.stRect.fX;
                float y1_roi = (float)stParam.stRoi.stRect.fY;
                float x2_roi = x1_roi + (float)stParam.stRoi.stRect.fW;
                float y2_roi = y1_roi + (float)stParam.stRoi.stRect.fH;

                if (!((x1 >= x1_roi && x1 <= x2_roi) && (x2 >= x1_roi && x2 <= x2_roi) && (y1 >= y1_roi && y1 <= y2_roi) &&
                      (y2 >= y1_roi && y2 <= y2_roi))) {
                    return AX_TRUE;
                }
            }

            if (strObjectCategory == "body") {
                // check filter size

                // check filter size
                if (!(tlwh[2] >= stParam.stFilterMaps["body"].minSize.width && tlwh[3] >= stParam.stFilterMaps["body"].minSize.height)) {
                    ALOGI("SKEL body filter(%fx%f:%dx%d)", tlwh[2], tlwh[3], stParam.stFilterMaps["body"].minSize.width, stParam.stFilterMaps["body"].minSize.height);
                    return AX_TRUE;
                }

                // check Attr filter size
                if (stObjectItem.fConfidence < stParam.stAttrFliterMaps["body"].stCommonAttrFilterConfig.fQuality) {
                    ALOGI("SKEL body quality filter(%f:%f)", stObjectItem.fConfidence, stParam.stAttrFliterMaps["body"].stCommonAttrFilterConfig.fQuality);
                    return AX_TRUE;
                }
            }
            else if (strObjectCategory == "vehicle")
            {
                // check filter size
                if (!(tlwh[2] >= stParam.stFilterMaps["vehicle"].minSize.width && tlwh[3] >= stParam.stFilterMaps["vehicle"].minSize.height)) {
                    ALOGI("SKEL vehicle filter(%fx%f:%dx%d)", tlwh[2], tlwh[3], stParam.stFilterMaps["vehicle"].minSize.width, stParam.stFilterMaps["vehicle"].minSize.height);
                    return AX_TRUE;
                }

                // check Attr filter size
                if (stObjectItem.fConfidence < stParam.stAttrFliterMaps["vehicle"].stCommonAttrFilterConfig.fQuality) {
                    ALOGI("SKEL vehicle quality filter(%f:%f)", stObjectItem.fConfidence, stParam.stAttrFliterMaps["vehicle"].stCommonAttrFilterConfig.fQuality);
                    return AX_TRUE;
                }
            }
            else if (strObjectCategory == "cycle")
            {
                // check filter size
                if (!(tlwh[2] >= stParam.stFilterMaps["cycle"].minSize.width && tlwh[3] >= stParam.stFilterMaps["cycle"].minSize.height)) {
                    ALOGI("SKEL cycle filter(%fx%f:%dx%d)", tlwh[2], tlwh[3], stParam.stFilterMaps["cycle"].minSize.width, stParam.stFilterMaps["cycle"].minSize.height);
                    return AX_TRUE;
                }

                // check Attr filter size
                if (stObjectItem.fConfidence < stParam.stAttrFliterMaps["cycle"].stCommonAttrFilterConfig.fQuality) {
                    ALOGI("SKEL cycle quality filter(%f:%f)", stObjectItem.fConfidence, stParam.stAttrFliterMaps["cycle"].stCommonAttrFilterConfig.fQuality);
                    return AX_TRUE;
                }
            }
            else if (strObjectCategory == "face")
            {
                // check filter size
                if (!(tlwh[2] >= stParam.stFilterMaps["face"].minSize.width && tlwh[3] >= stParam.stFilterMaps["face"].minSize.height)) {
                    ALOGI("SKEL face filter(%fx%f:%dx%d)", tlwh[2], tlwh[3], stParam.stFilterMaps["face"].minSize.width, stParam.stFilterMaps["face"].minSize.height);
                    return AX_TRUE;
                }

                // check Attr filter size
                if (!(tlwh[2] >= stParam.stAttrFliterMaps["face"].stFaceAttrFilterConfig.nWidth && tlwh[3] >= stParam.stAttrFliterMaps["face"].stFaceAttrFilterConfig.nHeight)) {
                    ALOGI("SKEL face filter(%fx%f:%dx%d)", tlwh[2], tlwh[3], stParam.stAttrFliterMaps["face"].stFaceAttrFilterConfig.nWidth, stParam.stAttrFliterMaps["face"].stFaceAttrFilterConfig.nHeight);
                    return AX_TRUE;
                }
            }
            else if (strObjectCategory == "plate")
            {
                // check filter size
                if (!(tlwh[2] >= stParam.stFilterMaps["plate"].minSize.width && tlwh[3] >= stParam.stFilterMaps["plate"].minSize.height)) {
                    ALOGI("SKEL plate filter(%fx%f:%dx%d)", tlwh[2], tlwh[3], stParam.stFilterMaps["plate"].minSize.width, stParam.stFilterMaps["plate"].minSize.height);
                    return AX_TRUE;
                }

                // check Attr filter size
                if (stObjectItem.fConfidence < stParam.stAttrFliterMaps["plate"].stCommonAttrFilterConfig.fQuality) {
                    ALOGI("SKEL plate quality filter(%f:%f)", stObjectItem.fConfidence, stParam.stAttrFliterMaps["plate"].stCommonAttrFilterConfig.fQuality);
                    return AX_TRUE;
                }
            }

            return AX_FALSE;
        }

        AX_S32 TrackerDealer::TrackPush(AX_U32 nStreamId, const AX_SKEL_PUSH_CACHE_LIST_T &stCacheFrame, AX_SKEL_OBJECT_ITEM_T &stObjectItem) {
            AX_U64 nTrackId = stObjectItem.nTrackId;
            AX_U64 nFrameId = stObjectItem.nFrameId;
            auto &trackMap = m_TrackMaps[nStreamId];
            auto &frameMap = m_FrameMaps[nStreamId];

            AX_VOID *pBuf = nullptr;
            AX_U32 nBufSize = 0;
            AX_U32 nDstWidth = 0;
            AX_U32 nDstHeight = 0;
            AX_S32 nRet = 0;

            // not found Or buffer is nil
            if (!TrackFind(nStreamId, nTrackId, stObjectItem.stCropFrame)
                || !stObjectItem.stCropFrame.pFrameData) {
                AX_SKEL_CROP_ENCODER_THRESHOLD_CONFIG_T stEncoderScaleSet = s_stEncoderScaleDefaultMaps[stObjectItem.pstrObjectCategory];

                AX_SKEL_RECT_T stRect = stObjectItem.stRect;
                stRect.fX = stRect.fX - stRect.fW * stEncoderScaleSet.fScaleLeft;
                stRect.fY = stRect.fY - stRect.fH * stEncoderScaleSet.fScaleTop;
                stRect.fW = stRect.fW * (1 + stEncoderScaleSet.fScaleLeft + stEncoderScaleSet.fScaleRight);
                stRect.fH = stRect.fH * (1 + stEncoderScaleSet.fScaleTop + stEncoderScaleSet.fScaleBottom);

                if (!stCacheFrame.bFrameDrop) {
                    nRet = JENCOBJ->Get(stCacheFrame.stFrame, stRect, nDstWidth, nDstHeight, &pBuf, &nBufSize, m_stParam.fCropEncoderQpLevel);

                    if (nRet == 0) {
                        auto &stObjectStore = trackMap[nTrackId].stObjectItem;
                        stObjectStore.bCropFrame = AX_TRUE;
                        stObjectStore.stCropFrame.nFrameId = nFrameId;
                        stObjectStore.stCropFrame.pFrameData = (AX_U8 *)pBuf;
                        stObjectStore.stCropFrame.nFrameDataSize = nBufSize;
                        stObjectStore.stCropFrame.nFrameWidth = nDstWidth;
                        stObjectStore.stCropFrame.nFrameHeight = nDstHeight;

                        stObjectItem.eTrackState = AX_SKEL_TRACK_STATUS_SELECT;
                        stObjectItem.bCropFrame = AX_TRUE;
                        stObjectItem.stCropFrame = stObjectStore.stCropFrame;
                    }
                    else {
                        return AX_ERR_SKEL_ILLEGAL_PARAM;
                    }
                }
                else {
                    auto &stTrackMap = trackMap[nTrackId];

                    ALOGW("SKEL FrameId: %lld is drop for trackId: %lld crop. L: %d, S: %d, N: %d, U: %d",
                          stCacheFrame.nFrameId, nTrackId, stTrackMap.eTrackLastState, stObjectItem.eTrackState, stTrackMap.nPushCounts, stTrackMap.nUpdateCounts);

                    return AX_ERR_SKEL_ILLEGAL_PARAM;
                }
            }
            else {
                stObjectItem.eTrackState = AX_SKEL_TRACK_STATUS_SELECT;
                stObjectItem.bCropFrame = AX_TRUE;
            }

            if (m_stParam.stPushPanoramaConfig.bEnable) {
                AX_BOOL isPanoPush = AX_TRUE;

                if (!FrameFind(nStreamId, nFrameId, stObjectItem.stPanoraFrame)
                    || !stObjectItem.stPanoraFrame.pFrameData) {
                    if (!stCacheFrame.bFrameDrop) {
                        AX_SKEL_RECT_T stRect{0};
                        AX_S32 nRet = JENCOBJ->Get(stCacheFrame.stFrame, stRect, nDstWidth, nDstHeight, &pBuf, &nBufSize, m_stParam.fCropEncoderQpLevel);

                        if (nRet == 0) {
                            auto &stPanoraStore = frameMap[nFrameId].stPanoraFrame;
                            stPanoraStore.nFrameId = nFrameId;
                            stPanoraStore.pFrameData = (AX_U8 *)pBuf;
                            stPanoraStore.nFrameDataSize = nBufSize;
                            stPanoraStore.nFrameWidth = nDstWidth;
                            stPanoraStore.nFrameHeight = nDstHeight;

                            stObjectItem.stPanoraFrame = stPanoraStore;
                        }
                        else {
                            isPanoPush = AX_FALSE;
                        }
                    }
                    else {
                        ALOGW("SKEL FrameId: %lld is drop for trackId: %lld panora.", stCacheFrame.nFrameId, nTrackId);
                    }
                }

                if (isPanoPush) {
                    stObjectItem.bPanoraFrame = AX_TRUE;

                    auto &stObjectStore = trackMap[nTrackId].stObjectItem;
                    stObjectStore.bPanoraFrame = AX_TRUE;
                    stObjectStore.stPanoraFrame = stObjectItem.stPanoraFrame;
                }
            }

            return AX_SKEL_SUCC;
        }

        AX_BOOL TrackerDealer::TrackFind(AX_U32 nStreamId, AX_U64 nTrackId, AX_SKEL_CROP_FRAME_T &stCropFrame) {
            auto &trackMap = m_TrackMaps[nStreamId];

            for (auto iter = trackMap.begin(); iter != trackMap.end();) {
                if (iter->first == nTrackId) {
                    stCropFrame = iter->second.stObjectItem.stCropFrame;
                    return AX_TRUE;
                }

                ++ iter;
            }

            return AX_FALSE;
        }

        AX_BOOL TrackerDealer::FrameFind(AX_U32 nStreamId, AX_U64 nFrameId, AX_SKEL_CROP_FRAME_T &stPanoraFrame) {
            auto &frameMap = m_FrameMaps[nStreamId];

            for (auto iter = frameMap.begin(); iter != frameMap.end();) {
                if (iter->first == nFrameId) {
                    stPanoraFrame = iter->second.stPanoraFrame;

                    return AX_TRUE;
                }

                ++ iter;
            }

            return AX_FALSE;
        }

        AX_BOOL TrackerDealer::FrameFind(AX_U32 nStreamId, AX_U64 nFrameId) {
            auto &frameMap = m_FrameMaps[nStreamId];

            for (auto iter = frameMap.begin(); iter != frameMap.end();) {
                if (iter->first == nFrameId) {
                    return AX_TRUE;
                }

                ++ iter;
            }

            return AX_FALSE;
        }

        AX_S32 TrackerDealer::TrackDelete(AX_U32 nStreamId, AX_U64 nTrackId, AX_BOOL bForce/* = AX_FALSE*/) {
            auto &trackMap = m_TrackMaps[nStreamId];

            for (auto iter = trackMap.begin(); iter != trackMap.end();) {
                if (iter->first == nTrackId) {
                    // release jenc buffer
                    PUSH_JENC_REL(iter->second.stObjectItem.stCropFrame.pFrameData);

                    // clear panoraFrame buffer
                    FrameDelete(nStreamId, iter->second.stObjectItem.nFrameId, nTrackId);

                    iter->second.stObjectItem.bPanoraFrame = AX_FALSE;
                    memset(&iter->second.stObjectItem.stPanoraFrame, 0x00, sizeof(iter->second.stObjectItem.stPanoraFrame));

                    // release track map
                    if (((m_stParam.stPushStrategy.ePushMode == AX_SKEL_PUSH_MODE_FAST
                          || m_stParam.stPushStrategy.ePushMode == AX_SKEL_PUSH_MODE_INTERVAL)
                         && iter->second.nPushCounts >= m_stParam.stPushStrategy.nPushCounts)
                        || m_stParam.stPushStrategy.ePushMode == AX_SKEL_PUSH_MODE_BEST
                        || bForce) {
                        trackMap.erase(iter++);
                    }

                    return AX_SKEL_SUCC;
                }

                ++ iter;
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::FrameDelete(AX_U32 nStreamId, AX_U64 nFrameId, AX_U64 nTrackId) {
            auto &frameMap = m_FrameMaps[nStreamId];

            for (auto iter = frameMap.begin(); iter != frameMap.end();) {
                if (iter->first == nFrameId) {
                    iter->second.liTrackList.remove(nTrackId);

                    // track list empty
                    if (iter->second.liTrackList.size() == 0) {
                        // release jenc buffer
                        PUSH_JENC_REL(iter->second.stPanoraFrame.pFrameData);

                        if (!iter->second.stCacheFrame.bFrameDrop) {
                            dec_io_ref_cnt(iter->second.stCacheFrame.stFrame);
                            iter->second.stCacheFrame.bFrameDrop = AX_TRUE;
                        }

                        frameMap.erase(iter++);
                    }
                    break;
                }

                ++ iter;
            }

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::GetConfig(AX_SKEL_PARAM_T &stParam) {
            std::lock_guard<std::mutex> lck(m_mtxSet);

            stParam = m_stParam;

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::SetConfig(const AX_SKEL_PARAM_T &stParam) {
            std::lock_guard<std::mutex> lck(m_mtxSet);

            // clear maps
            if (!stParam.bPushEnable
                || stParam.stPushStrategy.ePushMode != m_stParam.stPushStrategy.ePushMode
                || stParam.stPushStrategy.nIntervalTimes != m_stParam.stPushStrategy.nIntervalTimes
                || stParam.stPushStrategy.nPushCounts != m_stParam.stPushStrategy.nPushCounts
                || stParam.stPushStrategy.bPushSameFrame != m_stParam.stPushStrategy.bPushSameFrame) {
                std::lock_guard<std::mutex> lck(m_mtxMaps);
                ClearPush();
            }

            m_stParam = stParam;

            return AX_SKEL_SUCC;
        }

        AX_S32 TrackerDealer::Statistics(AX_VOID) {
            ALOGN("Push Statistics:");

            if (m_TrackMaps.size() > 0) {
                for (auto iterStream = m_TrackMaps.begin(); iterStream != m_TrackMaps.end();) {
                    if (iterStream->second.size() > 0) {
                        ALOGN("\tStream[%d] Track Maps:", iterStream->first);
                        for (auto iter = iterStream->second.begin(); iter != iterStream->second.end();) {
                            ALOGN("\t\tTrack Id[%lld]: (N: %d, U:%d, L: %d, S: %d, F: %lld, C:%p, P:%p, c=%f)",
                                  iter->first, iter->second.nPushCounts, iter->second.nUpdateCounts,
                                  iter->second.eTrackLastState, iter->second.stObjectItem.eTrackState,
                                  iter->second.stObjectItem.nFrameId,
                                  iter->second.stObjectItem.stCropFrame.pFrameData,
                                  iter->second.stObjectItem.stPanoraFrame.pFrameData,
                                  iter->second.stObjectItem.fConfidence);
                            ++ iter;
                        }
                    }
                    else {
                        ALOGN("\tStream[%d] Track Maps: nil", iterStream->first);
                    }

                    ++ iterStream;
                }
            }
            else {
                ALOGN("\tTrack Maps: nil");
            }

            if (m_FrameMaps.size() > 0) {
                for (auto iterStream = m_FrameMaps.begin(); iterStream != m_FrameMaps.end();) {
                    if (iterStream->second.size() > 0) {
                        ALOGN("\tStream[%d] Frame Maps:", iterStream->first);
                        for (auto iter = iterStream->second.begin(); iter != iterStream->second.end();) {
                            ALOGN("\t\tFrame Id[%lld]: (T:%ld, F: %lld, P=%p)",
                                  iter->first,
                                  iter->second.liTrackList.size(),
                                  iter->second.stPanoraFrame.nFrameId,
                                  iter->second.stPanoraFrame.pFrameData);
                            if (iter->second.liTrackList.size() > 0) {
                                for (auto iterList = iter->second.liTrackList.begin(); iterList != iter->second.liTrackList.end();) {
                                    ALOGN("\t\t\tTrack Id[%lld]", *iterList);
                                    ++ iterList;
                                }
                            }
                            ++ iter;
                        }
                    }
                    else {
                        ALOGN("\t\tStream[%d] Frame Maps: nil", iterStream->first);
                    }

                    ++ iterStream;
                }
            }
            else {
                ALOGN("\t\tFrame Maps: nil");
            }

            if (m_CacheListVec.size() > 0) {
                for (auto iterStream = m_CacheListVec.begin(); iterStream != m_CacheListVec.end();) {
                    if (iterStream->second.size() > 0) {
                        ALOGN("\tStream[%d] Cache List:", iterStream->first);
                        for (auto iter = iterStream->second.begin(); iter != iterStream->second.end();) {
                            ALOGN("\t\tCache Id[%lld]",
                                  iter->nFrameId);
                            ++ iter;
                        }
                    }
                    else {
                        ALOGN("\tStream[%d] Cache List: nil", iterStream->first);
                    }

                    ++ iterStream;
                }
            }
            else {
                ALOGN("\tCache List: nil");
            }

            JENCOBJ->Statistics();

            return AX_SKEL_SUCC;
        }
    }
}

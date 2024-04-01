/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_TRACKER_DEALER_H
#define SKEL_TRACKER_DEALER_H

#include "ax_skel_type.h"
#include "api/ax_skel_def.h"

#include <chrono>
#include <unordered_map>
#include <mutex>
#include <vector>
#include <list>

namespace skel {
    namespace utils {
        typedef struct axSKEL_PUSH_CACHE_LIST_T {
            AX_BOOL bFrameDrop;
            AX_U64 nFrameId;
            AX_VIDEO_FRAME_T stFrame;

            axSKEL_PUSH_CACHE_LIST_T() {
                bFrameDrop = AX_FALSE;
                nFrameId = 0;
                memset(&stFrame, 0x00, sizeof(stFrame));
            }
        } AX_SKEL_PUSH_CACHE_LIST_T;

        typedef struct axSKEL_PUSH_TRACK_MAPS {
            AX_U32 nPushCounts;
            AX_U32 nUpdateCounts;
            AX_SKEL_TRACK_STATUS_E eTrackLastState;
            AX_SKEL_OBJECT_ITEM_T stObjectItem;
            std::chrono::steady_clock::time_point updateTime;
            std::chrono::steady_clock::time_point pushTime;

            axSKEL_PUSH_TRACK_MAPS() {
                nPushCounts = 0;
                nUpdateCounts = 1;
                eTrackLastState = AX_SKEL_TRACK_STATUS_NEW;
                memset(&stObjectItem, 0x00, sizeof(stObjectItem));
                updateTime = std::chrono::steady_clock::now();
                pushTime = std::chrono::steady_clock::now();
            }
        } AX_SKEL_PUSH_TRACK_MAPS;

        typedef struct axSKEL_PUSH_FRAME_MAPS {
            std::list<AX_U64> liTrackList;
            AX_SKEL_PUSH_CACHE_LIST_T stCacheFrame;
            AX_SKEL_CROP_FRAME_T stPanoraFrame;

            axSKEL_PUSH_FRAME_MAPS() {
                memset(&stPanoraFrame, 0x00, sizeof(stPanoraFrame));
            }
        } AX_SKEL_PUSH_FRAME_MAPS;


        class TrackerDealer    {
        public:
            explicit TrackerDealer(AX_SKEL_PARAM_T stParam);
            virtual ~TrackerDealer(AX_VOID);

        public:
            virtual AX_S32 Update(const AX_SKEL_FRAME_T *pstFrame, AX_SKEL_OBJECT_ITEM_T &stObjectItem);
            virtual AX_S32 Release(AX_U32 nStreamId, const AX_SKEL_OBJECT_ITEM_T &stObjectItem);
            virtual AX_S32 Finalize(const AX_SKEL_FRAME_T *pstFrame, AX_SKEL_RESULT_T *pstResult, std::vector<AX_SKEL_OBJECT_ITEM_T> &vecObjectItem);
            virtual AX_S32 GetConfig(AX_SKEL_PARAM_T &stParam);
            virtual AX_S32 SetConfig(const AX_SKEL_PARAM_T &stParam);
            virtual AX_S32 Statistics(AX_VOID);

        private:
            AX_S32 ClearPush(AX_VOID);
            AX_S32 CachePush(const AX_SKEL_FRAME_T *pstFrame);
            AX_S32 TrackPush(AX_U32 nStreamId, const AX_SKEL_PUSH_CACHE_LIST_T &stCacheFrame, AX_SKEL_OBJECT_ITEM_T &stObjectItem);
            AX_S32 UpdateFast(const AX_SKEL_FRAME_T *pstFrame, AX_SKEL_OBJECT_ITEM_T &stObjectItem);
            AX_S32 UpdateInterval(const AX_SKEL_FRAME_T *pstFrame, AX_SKEL_OBJECT_ITEM_T &stObjectItem);
            AX_S32 UpdateBest(const AX_SKEL_FRAME_T *pstFrame, AX_SKEL_OBJECT_ITEM_T &stObjectItem);
            AX_S32 FinalizeFast(AX_U32 nStreamId, std::vector<AX_SKEL_OBJECT_ITEM_T> &vecObjectItem, std::vector<AX_SKEL_PUSH_CACHE_LIST_T> &dropCacheListVec);
            AX_S32 FinalizeInterval(AX_U32 nStreamId, std::vector<AX_SKEL_OBJECT_ITEM_T> &vecObjectItem, std::vector<AX_SKEL_PUSH_CACHE_LIST_T> &dropCacheListVec);
            AX_S32 FinalizeBest(AX_U32 nStreamId, std::vector<AX_SKEL_OBJECT_ITEM_T> &vecObjectItem, std::vector<AX_SKEL_PUSH_CACHE_LIST_T> &dropCacheListVec);
            AX_S32 TrackDelete(AX_U32 nStreamId, AX_U64 nTrackId, AX_BOOL bForce = AX_FALSE);
            AX_S32 FrameDelete(AX_U32 nStreamId, AX_U64 nFrameId, AX_U64 nTrackId);
            AX_BOOL TrackFind(AX_U32 nStreamId, AX_U64 nTrackId, AX_SKEL_CROP_FRAME_T &stCropFrame);
            AX_BOOL FrameFind(AX_U32 nStreamId, AX_U64 nFrameId, AX_SKEL_CROP_FRAME_T &stPanoraFrame);
            AX_BOOL FrameFind(AX_U32 nStreamId, AX_U64 nFrameId);
            AX_BOOL PushFliter(AX_SKEL_OBJECT_ITEM_T &stObjectItem);

        protected:
            std::mutex m_mtxMaps;
            std::unordered_map<AX_U32, std::unordered_map<AX_U64, AX_SKEL_PUSH_TRACK_MAPS>> m_TrackMaps;
            std::unordered_map<AX_U32, std::unordered_map<AX_U64, AX_SKEL_PUSH_FRAME_MAPS>> m_FrameMaps;
            std::unordered_map<AX_U32, std::vector<AX_SKEL_PUSH_CACHE_LIST_T>> m_CacheListVec;

            std::mutex m_mtxSet;
            AX_SKEL_PARAM_T m_stParam;
        };
    }
}

#endif //SKEL_TRACKER_DEALER_H

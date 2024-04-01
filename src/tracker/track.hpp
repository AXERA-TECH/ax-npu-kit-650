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

#include <vector>
#include <unordered_map>
#include "ax_global_type.h"
#include "tracker/kalmanFilter.hpp"
#include "inference/detection.hpp"

namespace skel {
    namespace tracker {
        #define track_map std::unordered_map // map // unordered_map

        enum TrackState { New = 0, Tracked, Lost, Removed };

        class CTrack {
        public:
            CTrack(const std::vector<float>& tlwh_,
                   const float& score,
                   const AX_U32& cls_id,
                   const AX_U64& real_frame_id,
                   const skel::detection::Object& object);
            ~CTrack();

            std::vector<float> static tlbrTotlwh(std::vector<float>& tlbr);
            void static multiPredict(std::vector<CTrack*>& tracks, KalmanFilter& kalman_filter);
            void staticTLWH();
            void staticTLBR();
            std::vector<float> tlwhToxyah(std::vector<float> tlwh_tmp);
            std::vector<float> toXYAH();
            void markLost();
            void markRemoved();
            static void initTrackIDDict(const AX_U32 n_classes);
            static AX_U64 nextID(const AX_U32& cls_id);
            // static void printIDDict();
            AX_U64 endFrame();
            void activate(KalmanFilter& kalman_filter, AX_U64 frame_id, AX_U64 real_frame_id);
            void reActivate(CTrack& new_track, AX_U64 frame_id, AX_U64 real_frame_id, bool new_id = false);
            void update(CTrack& new_track, AX_U64 frame_id, AX_U64 real_frame_id);

        public:
            bool is_activated;  // top tracking state
            AX_U64 track_id{};
            AX_U32 class_id{};
            AX_U32 state{};

            std::vector<float> _tlwh;
            std::vector<float> tlwh;  // x1y1wh
            std::vector<float> tlbr;  // x1y1x2y2
            AX_U64 frame_id{};
            AX_U64 tracklet_len{};
            AX_U64 start_frame{};

            KAL_MEAN mean{};
            KAL_COVA covariance{};
            float score;

            AX_U64 real_frame_id{}; // real frame id
            skel::detection::Object object{}; // object data (valid for NEW and UPDATE status)

            // mapping each class id to the track id count of this class
            //static track_map<AX_U32, AX_U64> static_track_id_dict;
            static AX_U64 static_track_id_dict;

        private:
            KalmanFilter kalman_filter;
        };
    }
}

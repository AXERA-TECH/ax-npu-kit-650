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

#include "ax_skel_type.h"
#include "inference/detection.hpp"
#include "tracker/track.hpp"

namespace skel {
    namespace tracker {
        #define DEFAULT_FRAME_RATE      30
        #define DEFAULT_TRACK_BUFFER    30
        #define DEFAULT_HIGH_DET_THRESH     0.5f
        #define DEFAULT_NEW_TRACK_THRESH    0.3f
        #define DEFAULT_HIGH_MATCH_THRESH   0.8f
        #define DEFAULT_LOW_MATCH_THRESH    0.5f
        #define DEFAULT_UNCONFIRMED_MATCH_THRESH    0.7f

        struct BYTETrackerConfig {
            AX_U32 n_classes;
            AX_U32 frame_rate;
            AX_U32 track_buffer;
            float high_det_thresh;
            float new_track_thresh;
            float high_match_thresh;
            float low_match_thresh;
            float unconfirmed_match_thresh;


            BYTETrackerConfig(AX_U32 _n_classes = 1):
                n_classes(_n_classes),
                frame_rate(DEFAULT_FRAME_RATE),
                track_buffer(DEFAULT_TRACK_BUFFER),
                high_det_thresh(DEFAULT_HIGH_DET_THRESH),
                new_track_thresh(DEFAULT_NEW_TRACK_THRESH),
                high_match_thresh(DEFAULT_HIGH_MATCH_THRESH),
                low_match_thresh(DEFAULT_LOW_MATCH_THRESH),
                unconfirmed_match_thresh(DEFAULT_UNCONFIRMED_MATCH_THRESH) {

            }
        };

        typedef track_map<AX_U32, std::vector<CTrack*>>  TrackResultType;

        class CBYTETracker {
        public:
            CBYTETracker():
                m_hasInited(false) {

            }
            ~CBYTETracker() = default;
            bool Init(const BYTETrackerConfig& config);
            TrackResultType Update(AX_SKEL_FRAME_T* frame, const std::vector<skel::detection::Object>& objects);

        private:
            std::vector<CTrack*> joinTracks(std::vector<CTrack*>& tlista, std::vector<CTrack>& tlistb);
            std::vector<CTrack> joinTracks(std::vector<CTrack>& tlista, std::vector<CTrack>& tlistb);
            std::vector<CTrack> subTracks(std::vector<CTrack>& tlista, std::vector<CTrack>& tlistb);
            void removeDuplicateTracks(std::vector<CTrack>& resa, std::vector<CTrack>& resb, std::vector<CTrack>& tracks_a, std::vector<CTrack>& tracks_b);
            void linearAssignment(std::vector<std::vector<float>>& cost_matrix, AX_U32 cost_matrix_size, AX_U32 cost_matrix_size_size, float thresh,
                                  std::vector<std::vector<AX_S32>>& matches, std::vector<AX_S32>& unmatched_a, std::vector<AX_S32>& unmatched_b);
            std::vector<std::vector<float>> iouDistance(std::vector<CTrack*>& atracks, std::vector<CTrack>& btracks, AX_U32& dist_size, AX_U32& dist_size_size);
            std::vector<std::vector<float>> iouDistance(std::vector<CTrack>& atracks, std::vector<CTrack>& btracks);
            std::vector<std::vector<float>> ious(std::vector<std::vector<float>>& atlbrs, std::vector<std::vector<float>>& btlbrs);
            double lapjv(const std::vector<std::vector<float>>& cost, std::vector<AX_S32>& rowsol, std::vector<AX_S32>& colsol, bool extend_cost = false,
                         float cost_limit = LONG_MAX, bool return_cost = true);

        private:
            bool m_hasInited;
            float m_high_det_thresh{};
            float m_new_track_thresh{};
            float m_high_match_thresh{};
            float m_low_match_thresh{};
            float m_unconfirmed_match_thresh{};
            AX_U64 m_frame_id{};
            AX_U32 m_max_time_lost{};

            // tracking object class number
            AX_U32 m_N_CLASSES{};

            // 3 containers of the tracker
            track_map<AX_U32, track_map<AX_U32, std::vector<CTrack>>> m_tracked_tracks_dict;
            track_map<AX_U32, track_map<AX_U32, std::vector<CTrack>>> m_lost_tracks_dict;
            track_map<AX_U32, track_map<AX_U32, std::vector<CTrack>>> m_removed_tracks_dict;

            KalmanFilter m_kalman_filter;
        };
    }
}

/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "tracker/byteTracker.hpp"

#include "utils/logger.h"

using namespace std;
using namespace skel::tracker;
using namespace skel::detection;
using namespace skel::infer;

bool CBYTETracker::Init(const BYTETrackerConfig& config) {
    m_N_CLASSES = config.n_classes;
    m_frame_id = 0;
    m_max_time_lost = config.track_buffer;
    m_high_det_thresh = config.high_det_thresh;
    m_new_track_thresh = config.new_track_thresh;
    m_high_match_thresh = config.high_match_thresh;
    m_low_match_thresh = config.low_match_thresh;
    m_unconfirmed_match_thresh = config.unconfirmed_match_thresh;

    m_hasInited = true;

    return true;
}

track_map<AX_U32, vector<CTrack*>> CBYTETracker::Update(AX_SKEL_FRAME_T* frame, const vector<Object>& objects) {
    if (!m_hasInited) {
        ALOGE("CBYTETracker has not inited!\n");
        return track_map<AX_U32, vector<CTrack*>>{};
    }

    AX_U32 nStreamId = frame->nStreamId;
    AX_U64 nFrameId = frame->nFrameId;

    // frame id updating
    this->m_frame_id++;

    auto &this_tracked_tracks_dict = this->m_tracked_tracks_dict[nStreamId];
    auto &this_lost_tracks_dict = this->m_lost_tracks_dict[nStreamId];
    auto &this_removed_tracks_dict = this->m_removed_tracks_dict[nStreamId];

    // ---------- Track's track id initialization
    if (this->m_frame_id == 1) {
        CTrack::initTrackIDDict(this->m_N_CLASSES);
    }

    // 8 current frame's containers
    track_map<AX_U32, vector<CTrack*>> unconfirmed_tracks_dict;
    track_map<AX_U32, vector<CTrack*>> tracked_tracks_dict;
    track_map<AX_U32, vector<CTrack*>> track_pool_dict;
    track_map<AX_U32, vector<CTrack>> activated_tracks_dict;
    track_map<AX_U32, vector<CTrack>> refind_tracks_dict;
    track_map<AX_U32, vector<CTrack>> lost_tracks_dict;
    track_map<AX_U32, vector<CTrack>> removed_tracks_dict;
    track_map<AX_U32, vector<CTrack *>> output_tracks_dict;

    ////////////////// Step 1: Get detections //////////////////
    track_map<AX_U32, vector<vector<float>>> bboxes_dict;
    track_map<AX_U32, vector<float>> scores_dict;
    track_map<AX_U32, vector<Object>> objects_dict;
    if (objects.size() > 0) {
        for (AX_U32 i = 0; i < objects.size(); ++i) {
            const Object& obj = objects[i];
            const float& score = obj.prob;  // confidence
            const AX_U32& cls_id = obj.label;  // class ID

            vector<float> x1y1x2y2;
            x1y1x2y2.resize(4);

            Rect_<float> rect = obj.rect;
            x1y1x2y2[0] = rect.x;                // x1
            x1y1x2y2[1] = rect.y;                // y1
            x1y1x2y2[2] = rect.x + rect.width;   // x2
            x1y1x2y2[3] = rect.y + rect.height;  // y2

            bboxes_dict[cls_id].push_back(x1y1x2y2);
            scores_dict[cls_id].push_back(score);
            objects_dict[cls_id].push_back(obj);
        }
    }  // non-empty objects assumption

    // ---------- Processing each object classes
    // ----- Build bbox_dict and score_dict
    for (AX_U32 cls_id = 0; cls_id < this->m_N_CLASSES; ++cls_id) {
        auto &cls_tracked_tracks_global = this_tracked_tracks_dict[cls_id];
        auto &cls_lost_tracks_global = this_lost_tracks_dict[cls_id];
        auto &cls_removed_tracks_global = this_removed_tracks_dict[cls_id];

        auto &cls_unconfirmed_tracks_inner = unconfirmed_tracks_dict[cls_id];
        auto &cls_tracked_tracks_inner = tracked_tracks_dict[cls_id];
        auto &cls_track_pool_inner = track_pool_dict[cls_id];
        auto &cls_activated_tracks_inner = activated_tracks_dict[cls_id];
        auto &cls_refind_tracks_inner = refind_tracks_dict[cls_id];
        auto &cls_lost_tracks_inner = lost_tracks_dict[cls_id];
        auto &cls_removed_tracks_inner = removed_tracks_dict[cls_id];
        auto &cls_output_tracks_inner = output_tracks_dict[cls_id];

        // clear removed tracks
        cls_removed_tracks_global.clear();

        // class bboxes
        vector<vector<float>>& cls_bboxes = bboxes_dict[cls_id];

        // class scores
        const vector<float>& cls_scores = scores_dict[cls_id];

        // class objects
        const vector<Object>& cls_objcets = objects_dict[cls_id];

        // temporary containers
        vector<CTrack> cls_dets;
        vector<CTrack> cls_dets_low;
        vector<CTrack> cls_dets_remain;
        vector<CTrack> cls_tracked_tracks_swap;
        vector<CTrack> cls_res_a, cls_res_b;
        vector<CTrack*> cls_unmatched_tracks;

        // detections classifications
        for (AX_U32 i = 0; i < cls_bboxes.size(); ++i) {
            vector<float>& tlbr_ = cls_bboxes[i];
            const float& score = cls_scores[i];
            const Object& object = cls_objcets[i];

            CTrack track(CTrack::tlbrTotlwh(tlbr_), score, cls_id, nFrameId, object);
            if (score >= this->m_high_det_thresh) { // high confidence dets
                cls_dets.push_back(track);
            }
            else {  // low confidence dets
                cls_dets_low.push_back(track);
            }
        }

        // Add newly detected tracklets to tracked_tracks
        for (AX_U32 i = 0; i < cls_tracked_tracks_global.size(); ++i) {
            if (!cls_tracked_tracks_global[i].is_activated) {
                cls_unconfirmed_tracks_inner.push_back(&cls_tracked_tracks_global[i]);
            } else {
                cls_tracked_tracks_inner.push_back(&cls_tracked_tracks_global[i]);
            }
        }

        ////////////////// Step 2: First association, with IoU //////////////////
        cls_track_pool_inner = joinTracks(cls_tracked_tracks_inner, cls_lost_tracks_global);
        CTrack::multiPredict(cls_track_pool_inner, this->m_kalman_filter);

        vector<vector<float>> dists;
        AX_U32 dist_size = 0, dist_size_size = 0;
        dists = iouDistance(cls_track_pool_inner, cls_dets, dist_size, dist_size_size);

        vector<vector<AX_S32>> matches;
        vector<AX_S32> u_track, u_detection;
        linearAssignment(dists, dist_size, dist_size_size, this->m_high_match_thresh, matches, u_track, u_detection);

        for (AX_U32 i = 0; i < matches.size(); ++i) {
            CTrack* track = cls_track_pool_inner[matches[i][0]];
            CTrack* det = &cls_dets[matches[i][1]];
            // FIXME.
            //if (track->state == TrackState::New
            //    || track->state == TrackState::Tracked) {
            if (track->state == TrackState::Tracked) {
                track->update(*det, this->m_frame_id, nFrameId);
                cls_activated_tracks_inner.push_back(*track);
            } else {
                track->reActivate(*det, this->m_frame_id, nFrameId, false);
                cls_refind_tracks_inner.push_back(*track);
            }
        }

        //// ----- Step 3: Second association, using low score dets ----- ////
        for (AX_U32 i = 0; i < u_detection.size(); ++i) {  // store unmatched detections from the the 1st round(high)
            cls_dets_remain.push_back(cls_dets[u_detection[i]]);
        }
        cls_dets.clear();
        cls_dets.assign(cls_dets_low.begin(), cls_dets_low.end());

        // unnatched tacks in track pool to cls_r_tracked_tracks
        for (AX_U32 i = 0; i < u_track.size(); ++i) {
            // FIXME.
            //if (cls_track_pool_inner[u_track[i]]->state == TrackState::New
            //    || cls_track_pool_inner[u_track[i]]->state == TrackState::Tracked) {
            if (cls_track_pool_inner[u_track[i]]->state == TrackState::Tracked) {
                cls_unmatched_tracks.push_back(cls_track_pool_inner[u_track[i]]);
            }
        }

        dists.clear();
        dists = iouDistance(cls_unmatched_tracks, cls_dets, dist_size, dist_size_size);

        matches.clear();
        u_track.clear();
        u_detection.clear();
        linearAssignment(dists, dist_size, dist_size_size, this->m_low_match_thresh, matches, u_track, u_detection);

        for (AX_U32 i = 0; i < matches.size(); ++i) {
            CTrack* track = cls_unmatched_tracks[matches[i][0]];
            CTrack* det = &cls_dets[matches[i][1]];
            // FIXME.
            //if (track->state == TrackState::New
            //    || track->state == TrackState::Tracked) {
            if (track->state == TrackState::Tracked) {
                track->update(*det, this->m_frame_id, nFrameId);
                cls_activated_tracks_inner.push_back(*track);
            } else {
                track->reActivate(*det, this->m_frame_id, nFrameId, false);
                cls_refind_tracks_inner.push_back(*track);
            }
        }

        // process the unmatched tracks for first 2 rounds
        for (AX_U32 i = 0; i < u_track.size(); ++i) {
            CTrack* track = cls_unmatched_tracks[u_track[i]];
            if (track->state != TrackState::Lost) {
                track->markLost();
                cls_lost_tracks_inner.push_back(*track);
            }
        }

        // ---------- Deal with unconfirmed tracks,
        // usually tracks with only one beginning frame
        cls_dets.clear();  // to store unmatched dets in the first round(high)
        cls_dets.assign(cls_dets_remain.begin(), cls_dets_remain.end());

        dists.clear();
        dists = iouDistance(cls_unconfirmed_tracks_inner, cls_dets, dist_size, dist_size_size);

        matches.clear();
        vector<AX_S32> u_unconfirmed;
        u_detection.clear();
        linearAssignment(dists, dist_size, dist_size_size, this->m_unconfirmed_match_thresh, matches, u_unconfirmed, u_detection);

        for (AX_U32 i = 0; i < matches.size(); ++i) {
            CTrack* track = cls_unconfirmed_tracks_inner[matches[i][0]];
            CTrack* det = &cls_dets[matches[i][1]];
            track->update(*det, this->m_frame_id, nFrameId);
            cls_activated_tracks_inner.push_back(*track);
        }

        for (AX_U32 i = 0; i < u_unconfirmed.size(); ++i) {
            CTrack* track = cls_unconfirmed_tracks_inner[u_unconfirmed[i]];
            track->markRemoved();
            cls_removed_tracks_inner.push_back(*track);
        }

        ////////////////// Step 4: Init new tracks //////////////////
        for (AX_U32 i = 0; i < u_detection.size(); ++i) {
            CTrack* track = &cls_dets[u_detection[i]];
            if (track->score < this->m_new_track_thresh) {
                continue;
            }
            track->activate(this->m_kalman_filter, this->m_frame_id, nFrameId);
            cls_activated_tracks_inner.push_back(*track);
        }

        ////////////////// Step 5: Update state //////////////////
        // ---------- update lost tracks' state
        for (AX_U32 i = 0; i < cls_lost_tracks_global.size(); ++i) {
            CTrack& track = cls_lost_tracks_global[i];
            if (this->m_frame_id - track.endFrame() > this->m_max_time_lost) {
                track.markRemoved();
                cls_removed_tracks_inner.push_back(track);
            }
        }

        // ---------- Post processing
        // ----- post processing of m_tracked_tracks
        for (AX_U32 i = 0; i < cls_tracked_tracks_global.size(); ++i) {
            // FIXME.
            //if (cls_tracked_tracks_global[i].state == TrackState::New
            //    || cls_tracked_tracks_global[i].state == TrackState::Tracked) {
            if (cls_tracked_tracks_global[i].state == TrackState::Tracked) {
                cls_tracked_tracks_swap.push_back(cls_tracked_tracks_global[i]);
            }
        }
        cls_tracked_tracks_global.clear();
        cls_tracked_tracks_global.assign(cls_tracked_tracks_swap.begin(), cls_tracked_tracks_swap.end());

        cls_tracked_tracks_global = joinTracks(cls_tracked_tracks_global, cls_activated_tracks_inner);
        cls_tracked_tracks_global = joinTracks(cls_tracked_tracks_global, cls_refind_tracks_inner);

        // ----- post processing of m_lost_tracks
        cls_lost_tracks_global = subTracks(cls_lost_tracks_global, cls_tracked_tracks_global);
        for (AX_U32 i = 0; i < cls_lost_tracks_inner.size(); ++i) {
            cls_lost_tracks_global.push_back(cls_lost_tracks_inner[i]);
        }

        // FIXME.
        // cls_lost_tracks_global = subTracks(cls_lost_tracks_global, cls_removed_tracks_global);
        for (AX_U32 i = 0; i < cls_removed_tracks_inner.size(); ++i) {
            cls_removed_tracks_global.push_back(cls_removed_tracks_inner[i]);
        }
        cls_lost_tracks_global = subTracks(cls_lost_tracks_global, cls_removed_tracks_global);

        // remove duplicate
        removeDuplicateTracks(cls_res_a, cls_res_b, cls_tracked_tracks_global, cls_lost_tracks_global);

        cls_tracked_tracks_global.clear();
        cls_tracked_tracks_global.assign(cls_res_a.begin(), cls_res_a.end());

        cls_lost_tracks_global.clear();
        cls_lost_tracks_global.assign(cls_res_b.begin(), cls_res_b.end());

        // return output
        for (AX_U32 i = 0; i < cls_tracked_tracks_global.size(); ++i) {
            // FIXME.
            //if (true/*cls_tracked_tracks_global[i].is_activated*/) {
            if (cls_tracked_tracks_global[i].is_activated) {
                cls_output_tracks_inner.push_back(&cls_tracked_tracks_global[i]);
            }
        }

        // FIXME.
        for (AX_U32 i = 0; i < cls_removed_tracks_global.size(); ++i) {
            if (true/*cls_removed_tracks_global[i].is_activated*/) {
                cls_output_tracks_inner.push_back(&cls_removed_tracks_global[i]);
            }
        }
    }  // End of class itereations

    return output_tracks_dict;
}

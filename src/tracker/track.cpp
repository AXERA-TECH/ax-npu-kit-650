/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Shanghai) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Shanghai) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Shanghai) Co., Ltd.
 *
 **************************************************************************************************/

#include "tracker/track.hpp"

using namespace std;
using namespace skel::detection;

//unordered_map<AX_U32, AX_U64> CTrack::static_track_id_dict;
AX_U64 skel::tracker::CTrack::static_track_id_dict = 0;

skel::tracker::CTrack::CTrack(const vector<float>& tlwh_,
                const float& score,
                const AX_U32& cls_id,
                const AX_U64& real_frame_id,
                const Object& object) {
    _tlwh.resize(4);
    _tlwh.assign(tlwh_.begin(), tlwh_.end());

    is_activated = false;  // init activate flag to be false
    track_id = 0;
    state = TrackState::New;

    tlwh.resize(4);
    tlbr.resize(4);

    staticTLWH();
    staticTLBR();

    frame_id = 0;
    tracklet_len = 0;

    this->class_id = cls_id;  // object class id
    this->score = score;
    this->real_frame_id = real_frame_id;
    this->object = object;

    start_frame = 0;
}

skel::tracker::CTrack::~CTrack() {
}

void skel::tracker::CTrack::initTrackIDDict(const AX_U32 n_classes) {
#if 0
    for (AX_U32 i = 0; i < n_classes; ++i) {
        CTrack::static_track_id_dict[i] = 0;
    }
#else
    static_track_id_dict = 0;
#endif
}

void skel::tracker::CTrack::activate(KalmanFilter& kalman_filter, AX_U64 frame_id, AX_U64 real_frame_id) {
    this->kalman_filter = kalman_filter;  // send the shared kalman filter
    this->track_id = CTrack::nextID(this->class_id);

    vector<float> _tlwh_tmp(4);
    _tlwh_tmp[0] = this->_tlwh[0];
    _tlwh_tmp[1] = this->_tlwh[1];
    _tlwh_tmp[2] = this->_tlwh[2];
    _tlwh_tmp[3] = this->_tlwh[3];
    vector<float> xyah = tlwhToxyah(_tlwh_tmp);
    DETECT_BOX xyah_box;
    xyah_box[0] = xyah[0];
    xyah_box[1] = xyah[1];
    xyah_box[2] = xyah[2];
    xyah_box[3] = xyah[3];
    auto mc = this->kalman_filter.initiate(xyah_box);
    this->mean = mc.first;
    this->covariance = mc.second;

    staticTLWH();
    staticTLBR();

    this->tracklet_len = 0;

    // FIXME.
    // this->state = TrackState::Tracked;
    this->state = TrackState::New;
    if (frame_id == 1) {
        this->is_activated = true;
    }

    // FIXME.
    this->is_activated = true;
    // this->is_activated = true;
    this->frame_id = frame_id;
    this->real_frame_id = real_frame_id;

    // set start frame
    this->start_frame = frame_id;
}

void skel::tracker::CTrack::reActivate(CTrack& new_track, AX_U64 frame_id, AX_U64 real_frame_id, bool new_id) {
    vector<float> xyah = tlwhToxyah(new_track.tlwh);

    DETECT_BOX xyah_box;
    xyah_box[0] = xyah[0];
    xyah_box[1] = xyah[1];
    xyah_box[2] = xyah[2];
    xyah_box[3] = xyah[3];
    auto mc = this->kalman_filter.update(this->mean, this->covariance, xyah_box, new_track.score);  // NSA kalman filter

    this->mean = mc.first;
    this->covariance = mc.second;

    // FIXME.
    _tlwh.resize(4);
    _tlwh.assign(new_track.tlwh.begin(), new_track.tlwh.end());

    staticTLWH();
    staticTLBR();

    this->tracklet_len = 0;
    this->frame_id = frame_id;
    this->score = new_track.score;
    this->real_frame_id = real_frame_id;

    this->state = TrackState::Tracked;
    this->is_activated = true;  // set to be activated

    if (new_id) {
        // FIXME.
        this->state = TrackState::New;
        this->track_id = CTrack::nextID(this->class_id);
    }
}

void skel::tracker::CTrack::update(CTrack& new_track, AX_U64 frame_id, AX_U64 real_frame_id) {
    this->frame_id = frame_id;
    this->tracklet_len++;

    vector<float> xyah = tlwhToxyah(new_track.tlwh);
    DETECT_BOX xyah_box;
    xyah_box[0] = xyah[0];
    xyah_box[1] = xyah[1];
    xyah_box[2] = xyah[2];
    xyah_box[3] = xyah[3];

    auto mc = this->kalman_filter.update(this->mean, this->covariance, xyah_box, new_track.score);  // NSA kalman filter
    this->mean = mc.first;
    this->covariance = mc.second;

    // FIXME.
    _tlwh.resize(4);
    _tlwh.assign(new_track.tlwh.begin(), new_track.tlwh.end());

    staticTLWH();
    staticTLBR();

    this->state = TrackState::Tracked;
    this->is_activated = true;  // set to be activated

    this->score = new_track.score;
    this->real_frame_id = real_frame_id;
}

void skel::tracker::CTrack::staticTLWH() {
    if (this->state == TrackState::New) {
        tlwh[0] = _tlwh[0];
        tlwh[1] = _tlwh[1];
        tlwh[2] = _tlwh[2];
        tlwh[3] = _tlwh[3];
        return;
    }

    tlwh[0] = mean[0];  // x(center_x)
    tlwh[1] = mean[1];  // y(center_y)
    tlwh[2] = mean[2];  // a(a=w/h, aspect ratio)
    tlwh[3] = mean[3];  // h

    tlwh[2] *= tlwh[3];  // -> center_x, center_y, w, h

    // -> x1y1wh
    tlwh[0] -= tlwh[2] * 0.5f;
    tlwh[1] -= tlwh[3] * 0.5f;
}

void skel::tracker::CTrack::staticTLBR() {  // x1y1wh -> x1y1x2y2
    tlbr.clear();
    tlbr.assign(tlwh.begin(), tlwh.end());
    tlbr[2] += tlbr[0];
    tlbr[3] += tlbr[1];
}

vector<float> skel::tracker::CTrack::tlwhToxyah(vector<float> tlwh_tmp) {
    vector<float> tlwh_output = tlwh_tmp;
    tlwh_output[0] += tlwh_output[2] * 0.5f;
    tlwh_output[1] += tlwh_output[3] * 0.5f;
    tlwh_output[2] /= tlwh_output[3];
    return tlwh_output;
}

vector<float> skel::tracker::CTrack::toXYAH() {
    return tlwhToxyah(tlwh);
}

vector<float> skel::tracker::CTrack::tlbrTotlwh(vector<float>& tlbr) {  // x1y1x2y2 -> x1y1wh
    tlbr[2] -= tlbr[0];                                 // w = x2 - x1
    tlbr[3] -= tlbr[1];
    return tlbr;
}

void skel::tracker::CTrack::markLost() {
    state = TrackState::Lost;
}

void skel::tracker::CTrack::markRemoved() {
    state = TrackState::Removed;
}

AX_U64 skel::tracker::CTrack::nextID(const AX_U32& cls_id) {
#if 0
    CTrack::static_track_id_dict[cls_id] += 1;
    return CTrack::static_track_id_dict[cls_id];
#else
    return (++ static_track_id_dict);
#endif
}

AX_U64 skel::tracker::CTrack::endFrame() {
    return this->frame_id;
}

void skel::tracker::CTrack::multiPredict(vector<CTrack*>& tracks, KalmanFilter& kalman_filter) {
    for (AX_U32 i = 0; i < tracks.size(); ++i) {
        if (tracks[i]->state != TrackState::Tracked) {
            tracks[i]->mean[7] = 0;
        }
        kalman_filter.predict(tracks[i]->mean, tracks[i]->covariance);
    }
}

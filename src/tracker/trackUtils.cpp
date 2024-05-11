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
#include "tracker/lapjv.hpp"

#include <map>

using namespace std;
using namespace skel::tracker;

vector<CTrack*> CBYTETracker::joinTracks(vector<CTrack*>& tlista, vector<CTrack>& tlistb) {
    map<AX_U64, AX_U64> exists;
    vector<CTrack*> res;
    for (AX_U32 i = 0; i < tlista.size(); i++) {
        exists.insert(pair<AX_U64, AX_U64>(tlista[i]->track_id, 1));
        res.push_back(tlista[i]);
    }
    for (AX_U32 i = 0; i < tlistb.size(); i++) {
        AX_U64 tid = tlistb[i].track_id;
        if (!exists[tid] || exists.count(tid) == 0) {
            exists[tid] = 1;
            res.push_back(&tlistb[i]);
        }
    }

    return res;
}

vector<CTrack> CBYTETracker::joinTracks(vector<CTrack>& tlista, vector<CTrack>& tlistb) {
    map<AX_U64, AX_U64> exists;
    vector<CTrack> res;
    for (AX_U32 i = 0; i < tlista.size(); ++i) {
        exists.insert(pair<AX_U64, AX_U64>(tlista[i].track_id, 1));
        res.push_back(tlista[i]);
    }
    for (AX_U32 i = 0; i < tlistb.size(); i++) {
        AX_U64 tid = tlistb[i].track_id;
        if (!exists[tid] || exists.count(tid) == 0) {
            exists[tid] = 1;
            res.push_back(tlistb[i]);
        }
    }
    return res;
}

vector<CTrack> CBYTETracker::subTracks(vector<CTrack>& tlista, vector<CTrack>& tlistb) {
    map<AX_U64, CTrack> tracks;
    for (AX_U32 i = 0; i < tlista.size(); i++) {
        tracks.insert(pair<AX_U64, CTrack>(tlista[i].track_id, tlista[i]));
    }
    for (AX_U32 i = 0; i < tlistb.size(); i++) {
        AX_U64 tid = tlistb[i].track_id;
        if (tracks.count(tid) != 0) {
            tracks.erase(tid);
        }
    }

    vector<CTrack> res;
    std::map<AX_U64, CTrack>::iterator it;
    for (it = tracks.begin(); it != tracks.end(); ++it) {
        res.push_back(it->second);
    }

    return res;
}

void CBYTETracker::removeDuplicateTracks(vector<CTrack>& res_a, vector<CTrack>& res_b, vector<CTrack>& tracks_a, vector<CTrack>& tracks_b) {
    vector<vector<float>> pdist = iouDistance(tracks_a, tracks_b);
    vector<pair<AX_U32, AX_U32>> pairs;
    for (AX_U32 i = 0; i < pdist.size(); i++) {
        for (AX_U32 j = 0; j < pdist[i].size(); j++) {
            if (pdist[i][j] < 0.15) {
                pairs.push_back(pair<AX_U32, AX_U32>(i, j));
            }
        }
    }

    vector<AX_U32> dupa, dupb;
    for (AX_U32 i = 0; i < pairs.size(); i++) {
        AX_U64 timep = tracks_a[pairs[i].first].frame_id - tracks_a[pairs[i].first].start_frame;
        AX_U64 timeq = tracks_b[pairs[i].second].frame_id - tracks_b[pairs[i].second].start_frame;
        if (timep > timeq) {
            dupb.push_back(pairs[i].second);
        }
        else {
            dupa.push_back(pairs[i].first);
        }
    }

    for (AX_U32 i = 0; i < tracks_a.size(); i++) {
        vector<AX_U32>::iterator iter = find(dupa.begin(), dupa.end(), i);
        if (iter == dupa.end()) {
            res_a.push_back(tracks_a[i]);
        }
    }

    for (AX_U32 i = 0; i < tracks_b.size(); i++) {
        vector<AX_U32>::iterator iter = find(dupb.begin(), dupb.end(), i);
        if (iter == dupb.end()) {
            res_b.push_back(tracks_b[i]);
        }
    }
}

void CBYTETracker::linearAssignment(vector<vector<float>>& cost_matrix, AX_U32 cost_matrix_size, AX_U32 cost_matrix_size_size, float thresh,
                                   vector<vector<AX_S32>>& matches, vector<AX_S32>& unmatched_a, vector<AX_S32>& unmatched_b) {
    if (cost_matrix.size() == 0) {
        for (AX_U32 i = 0; i < cost_matrix_size; i++) {
            unmatched_a.push_back(i);
        }
        for (AX_U32 i = 0; i < cost_matrix_size_size; i++) {
            unmatched_b.push_back(i);
        }
        return;
    }

    vector<AX_S32> rowsol;
    vector<AX_S32> colsol;
    //float c = (float)lapjv(cost_matrix, rowsol, colsol, true, thresh);
    lapjv(cost_matrix, rowsol, colsol, true, thresh);
    for (AX_U32 i = 0; i < rowsol.size(); i++) {
        if (rowsol[i] >= 0) {
            vector<AX_S32> match;
            match.push_back(i);
            match.push_back(rowsol[i]);
            matches.push_back(match);
        } else {
            unmatched_a.push_back(i);
        }
    }

    for (AX_U32 i = 0; i < colsol.size(); i++) {
        if (colsol[i] < 0) {
            unmatched_b.push_back(i);
        }
    }
}

vector<vector<float>> CBYTETracker::ious(vector<vector<float>>& atlbrs, vector<vector<float>>& btlbrs) {
    vector<vector<float>> ious;
    if (atlbrs.size() * btlbrs.size() == 0) return ious;

    ious.resize(atlbrs.size());
    for (AX_U32 i = 0; i < ious.size(); i++) {
        ious[i].resize(btlbrs.size());
    }

    // bbox_ious
    for (AX_U32 k = 0; k < btlbrs.size(); k++) {
        vector<float> ious_tmp;
        float box_area = (btlbrs[k][2] - btlbrs[k][0] + 1) * (btlbrs[k][3] - btlbrs[k][1] + 1);
        for (AX_U32 n = 0; n < atlbrs.size(); n++) {
            float iw = min(atlbrs[n][2], btlbrs[k][2]) - max(atlbrs[n][0], btlbrs[k][0]) + 1;
            if (iw > 0) {
                float ih = min(atlbrs[n][3], btlbrs[k][3]) - max(atlbrs[n][1], btlbrs[k][1]) + 1;
                if (ih > 0) {
                    float ua = (atlbrs[n][2] - atlbrs[n][0] + 1) * (atlbrs[n][3] - atlbrs[n][1] + 1) + box_area - iw * ih;
                    ious[n][k] = iw * ih / ua;
                } else {
                    ious[n][k] = 0.0;
                }
            } else {
                ious[n][k] = 0.0;
            }
        }
    }

    return ious;
}

vector<vector<float>> CBYTETracker::iouDistance(vector<CTrack*>& atracks, vector<CTrack>& btracks, AX_U32& dist_size, AX_U32& dist_size_size) {
    vector<vector<float>> cost_matrix;
    if (atracks.size() * btracks.size() == 0) {
        dist_size = (AX_U32)atracks.size();
        dist_size_size = (AX_U32)btracks.size();
        return cost_matrix;
    }
    vector<vector<float>> atlbrs, btlbrs;
    for (AX_U32 i = 0; i < atracks.size(); i++) {
        atlbrs.push_back(atracks[i]->tlbr);
    }
    for (AX_U32 i = 0; i < btracks.size(); i++) {
        btlbrs.push_back(btracks[i].tlbr);
    }

    dist_size = (AX_U32)atracks.size();
    dist_size_size = (AX_U32)btracks.size();

    vector<vector<float>> _ious = ious(atlbrs, btlbrs);

    for (AX_U32 i = 0; i < _ious.size(); i++) {
        vector<float> _iou;
        for (AX_U32 j = 0; j < _ious[i].size(); j++) {
            _iou.push_back(1 - _ious[i][j]);
        }
        cost_matrix.push_back(_iou);
    }

    return cost_matrix;
}

vector<vector<float>> CBYTETracker::iouDistance(vector<CTrack>& atracks, vector<CTrack>& btracks) {
    vector<vector<float>> atlbrs, btlbrs;
    for (AX_U32 i = 0; i < atracks.size(); i++) {
        atlbrs.push_back(atracks[i].tlbr);
    }
    for (AX_U32 i = 0; i < btracks.size(); i++) {
        btlbrs.push_back(btracks[i].tlbr);
    }

    vector<vector<float>> _ious = ious(atlbrs, btlbrs);
    vector<vector<float>> cost_matrix;
    for (AX_U32 i = 0; i < _ious.size(); i++) {
        vector<float> _iou;
        for (AX_U32 j = 0; j < _ious[i].size(); j++) {
            _iou.push_back(1 - _ious[i][j]);
        }
        cost_matrix.push_back(_iou);
    }

    return cost_matrix;
}

double CBYTETracker::lapjv(const vector<vector<float>>& cost, vector<AX_S32>& rowsol, vector<AX_S32>& colsol, bool extend_cost, float cost_limit,
                          bool return_cost) {
    vector<vector<float>> cost_c;
    cost_c.assign(cost.begin(), cost.end());

    vector<vector<float>> cost_c_extended;

    AX_U32 n_rows = (AX_U32)cost.size();
    AX_U32 n_cols = (AX_U32)cost[0].size();
    rowsol.resize(n_rows);
    colsol.resize(n_cols);

    AX_U32 n = 0;
    if (n_rows == n_cols) {
        n = n_rows;
    } else {
        if (!extend_cost) {
            //TODO
            //cout << "set extend_cost=True" << endl;
            //system("pause");
            //exit(0);
            return 0;
        }
    }

    if (extend_cost || cost_limit < LONG_MAX) {
        n = n_rows + n_cols;
        cost_c_extended.resize(n);
        for (AX_U32 i = 0; i < cost_c_extended.size(); i++) {
            cost_c_extended[i].resize(n);
        }

        if (cost_limit < LONG_MAX) {
            for (AX_U32 i = 0; i < cost_c_extended.size(); i++) {
                for (AX_U32 j = 0; j < cost_c_extended[i].size(); j++) {
                    cost_c_extended[i][j] = cost_limit / 2.0f;
                }
            }
        } else {
            float cost_max = -1;
            for (AX_U32 i = 0; i < cost_c.size(); i++) {
                for (AX_U32 j = 0; j < cost_c[i].size(); j++) {
                    if (cost_c[i][j] > cost_max) {
                        cost_max = cost_c[i][j];
                    }
                }
            }
            for (AX_U32 i = 0; i < cost_c_extended.size(); i++) {
                for (AX_U32 j = 0; j < cost_c_extended[i].size(); j++) {
                    cost_c_extended[i][j] = cost_max + 1;
                }
            }
        }

        for (AX_U32 i = n_rows; i < cost_c_extended.size(); i++) {
            for (AX_U32 j = n_cols; j < cost_c_extended[i].size(); j++) {
                cost_c_extended[i][j] = 0;
            }
        }
        for (AX_U32 i = 0; i < n_rows; i++) {
            for (AX_U32 j = 0; j < n_cols; j++) {
                cost_c_extended[i][j] = cost_c[i][j];
            }
        }

        cost_c.clear();
        cost_c.assign(cost_c_extended.begin(), cost_c_extended.end());
    }

    double opt = 0.0;
    double** cost_ptr;
    cost_ptr = new double*[sizeof(double*) * n];
    for (AX_U32 i = 0; i < n; i++) {
        cost_ptr[i] = new double[sizeof(double) * n];
    }

    for (AX_U32 i = 0; i < n; i++) {
        for (AX_U32 j = 0; j < n; j++) {
            cost_ptr[i][j] = cost_c[i][j];
        }
    }

    AX_S32* x_c = new AX_S32[sizeof(AX_S32) * n];
    AX_S32* y_c = new AX_S32[sizeof(AX_S32) * n];

    AX_S32 ret = lapjv_internal(n, cost_ptr, x_c, y_c);
    if (ret != 0) {
        //TODO
        // cout << "Calculate Wrong!" << endl;
        //system("pause");
        //exit(0);
        printf("Calculate Wrong!\n");
        goto EXIT;
    }

    if (n != n_rows) {
        for (AX_U32 i = 0; i < n; i++) {
            if (x_c[i] >= (AX_S32)n_cols) {
                x_c[i] = -1;
            }
            if (y_c[i] >= (AX_S32)n_rows) {
                y_c[i] = -1;
            }
        }
        for (AX_U32 i = 0; i < n_rows; i++) {
            rowsol[i] = x_c[i];
        }
        for (AX_U32 i = 0; i < n_cols; i++) {
            colsol[i] = y_c[i];
        }

        if (return_cost) {
            for (AX_U32 i = 0; i < rowsol.size(); i++) {
                if (rowsol[i] != -1) {
                    // cout << i << "\t" << rowsol[i] << "\t" << cost_ptr[i][rowsol[i]] << endl;
                    opt += cost_ptr[i][rowsol[i]];
                }
            }
        }
    } else if (return_cost) {
        for (AX_U32 i = 0; i < rowsol.size(); i++) {
            opt += cost_ptr[i][rowsol[i]];
        }
    }

EXIT:
    for (AX_U32 i = 0; i < n; i++) {
        delete[] cost_ptr[i];
    }

    delete[] cost_ptr;
    delete[] x_c;
    delete[] y_c;
    cost_ptr = nullptr;
    x_c = nullptr;
    y_c = nullptr;

    return opt;
}

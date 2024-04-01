/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * License); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * AS IS BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

#include "ax_global_type.h"
#include "ax_engine_type.h"
#include "inference/cv_types.h"

namespace skel {
    namespace detection {
        typedef struct {
            int grid0;
            int grid1;
            int stride;
        } GridAndStride;

        typedef struct {
            float x;
            float y;
            float score;
        } ai_point_t;

        typedef struct _ai_plate_attr_t {
            AX_U32 nCount;
            AX_BOOL bValid;
            AX_F32 fScore;
            skel::infer::Rect_<float> rect;
            std::string strPlateLicense;
            std::vector<int> no_repeat_blank_label;

            _ai_plate_attr_t() {
                nCount = 0;
                bValid = AX_FALSE;
                fScore = 0;
            }
        } ai_plate_attr_t;

        typedef struct _Object {
            skel::infer::Rect_<float> rect;
            skel::infer::Rect_<float> iou_rect; // for iou distance calculate
            skel::infer::Rect_<float> master_rect; // for master rect
            int label;
            float prob;
            std::vector<ai_point_t> points;
            std::vector<float> boxes;
            float angle;
            AX_U64 object_id;
            AX_U64 bind_track_id;
            AX_BOOL exist_plate;
            ai_plate_attr_t plate_attr;

            _Object() {
                label = 0;
                prob = 0;
                angle = 0;
                object_id = 0;
                bind_track_id = 0;
                exist_plate = AX_FALSE;
            }
        } Object;

        typedef struct _detResult {
            int cls;
            float prob;
            skel::infer::Rect bbox;

            _detResult() { }

            _detResult(int cls_, float prob_, const skel::infer::Rect& bbox_)
            {
                cls = cls_;
                prob = prob_;
                bbox = bbox_;
            }
        } DetResult;

        static inline float sigmoid(float x) {
            return static_cast<float>(1.f / (1.f + exp(-x)));
        }

        static inline float intersection_area(const Object& a, const Object& b) {
            skel::infer::Rect_<float> inter = a.rect & b.rect;
            return inter.area();
        }

        static inline void qsort_descent_inplace(std::vector<Object>& faceobjects, int left, int right) {
            int i = left;
            int j = right;
            float p = faceobjects[(left + right) / 2].prob;

            while (i <= j) {
                while (faceobjects[i].prob > p) i++;

                while (faceobjects[j].prob < p) j--;

                if (i <= j) {
                    // swap
                    std::swap(faceobjects[i], faceobjects[j]);

                    i++;
                    j--;
                }
            }
//#pragma omp parallel sections
            {
//#pragma omp section
                {
                    if (left < j) qsort_descent_inplace(faceobjects, left, j);
                }
//#pragma omp section
                {
                    if (i < right) qsort_descent_inplace(faceobjects, i, right);
                }
            }
        }

        static inline void qsort_descent_inplace(std::vector<Object>& faceobjects) {
            if (faceobjects.empty()) return;

            qsort_descent_inplace(faceobjects, 0, faceobjects.size() - 1);
        }

        static inline void nms_sorted_bboxes(const std::vector<Object>& faceobjects, std::vector<int>& picked, float nms_threshold) {
            picked.clear();

            const int n = faceobjects.size();

            std::vector<float> areas(n);
            for (int i = 0; i < n; i++) {
                areas[i] = faceobjects[i].rect.area();
            }

            for (int i = 0; i < n; i++) {
                const Object& a = faceobjects[i];

                int keep = 1;
                for (int j = 0; j < (int)picked.size(); j++) {
                    const Object& b = faceobjects[picked[j]];

                    // intersection over union
                    float inter_area = intersection_area(a, b);
                    float union_area = areas[i] + areas[picked[j]] - inter_area;
                    // float IoU = inter_area / union_area
                    if (inter_area / union_area > nms_threshold) keep = 0;
                }

                if (keep) picked.push_back(i);
            }
        }

        static inline void hvc_nms_sorted_bboxes(const std::vector<Object>& faceobjects,
                                                 std::vector<int>& picked,
                                                 const float nms_threshold,
                                                 const float nms_bbox_overlap_ratio,
                                                 int nums_class) {
            picked.clear();

            const int n = faceobjects.size();

            std::vector<float> areas(n);
            for (int i = 0; i < n; i++) {
                areas[i] = faceobjects[i].rect.area();
            }

            for (int c = 0; c < nums_class; ++c) {
                for (int i = 0; i < n; i++) {
                    const Object& a = faceobjects[i];

                    if (a.label != c) {
                        continue;
                    }

                    int keep = 1;

                    for (int j = 0; j < (int)picked.size(); j++) {
                        const Object& b = faceobjects[picked[j]];
                        if (b.label != c) {
                            continue;
                        }
                        // intersection over union

                        float inter_area = intersection_area(a, b);
                        float union_area = areas[i] + areas[picked[j]] - inter_area;

                        // float IoU = inter_area / union_area
                        if (inter_area / union_area > nms_threshold) {
                            keep = 0;
                        }

                        // suppress bigger contina smaller
                        if ((inter_area > nms_bbox_overlap_ratio * areas[i])
                            || (inter_area > nms_bbox_overlap_ratio * areas[picked[j]])) {
                            keep = 0;
                        }
                    }

                    if (keep) {
                        picked.push_back(i);
                    }
                }
            }
        }

        static inline void generate_grids_and_stride(const int target_w, const int target_h, std::vector<int>& strides,
                                                     std::vector<GridAndStride>& grid_strides) {
            for (auto stride : strides) {
                int num_grid_w = target_w / stride;
                int num_grid_h = target_h / stride;
                for (int g1 = 0; g1 < num_grid_h; g1++) {
                    for (int g0 = 0; g0 < num_grid_w; g0++) {
                        GridAndStride gs;
                        gs.grid0 = g0;
                        gs.grid1 = g1;
                        gs.stride = stride;
                        grid_strides.push_back(gs);
                    }
                }
            }
        }

        static inline void nhwc2nchw(float* src, int n, int h, int w, int c, float* dst)
        {
            int c_stride = h * w;
            int h_stride = w * c;
            int n_stride = c * h * w;
            for (int i = 0; i < n; i++)
            {
                float* _src = src + i * n_stride;
                for (int j = 0; j < h; j++)
                {
                    for (int k = 0; k < w; k++)
                    {
                        for (int l = 0; l < c; l++)
                        {
                            dst[i * n_stride + l * c_stride + j * w + k] = _src[j * h_stride + k * c + l];
                        }
                    }
                }
            }
        }

        static inline void generate_yolox_proposals(std::vector<GridAndStride> grid_strides,
                                                    const AX_ENGINE_IOMETA_T& output_info,
                                                    float* feat_ptr,
                                                    float cls_thresh, const skel::infer::Size& min_size,
                                                    std::vector<Object> &objects) {
            const int num_anchors = grid_strides.size();

            int feat_c = output_info.pShape[1];
            int feat_h = output_info.pShape[2];
            int feat_w = output_info.pShape[3];
            int c_stride = feat_h * feat_w;
            int num_classes = feat_c - 5;
//            printf("c h w num_cls num_anchors: %d %d %d %d %d\n", feat_c, feat_h, feat_w, num_classes, num_anchors);

            // x_center y_center w h obj cls0 cls1 ...
            float* feat_ptr_x_center = feat_ptr;
            float* feat_ptr_y_center = feat_ptr + c_stride;
            float* feat_ptr_w = feat_ptr + 2 * c_stride;
            float* feat_ptr_h = feat_ptr + 3 * c_stride;
            float* feat_ptr_objectness = feat_ptr + 4 * c_stride;

            for (int anchor_idx = 0; anchor_idx < num_anchors; anchor_idx++) {
                float box_objectness = feat_ptr_objectness[anchor_idx];
                // printf("%d,%d\n",num_anchors,anchor_idx);
                const int grid0 = grid_strides[anchor_idx].grid0; // 0
                const int grid1 = grid_strides[anchor_idx].grid1; // 0
                const int stride = grid_strides[anchor_idx].stride; // 8
                // yolox/models/yolo_head.py decode logic
                //  outputs[..., :2] = (outputs[..., :2] + grids) * strides
                //  outputs[..., 2:4] = torch.exp(outputs[..., 2:4]) * strides
                float x_center = (feat_ptr_x_center[anchor_idx] + grid0) * stride;
                float y_center = (feat_ptr_y_center[anchor_idx] + grid1) * stride;
                float w = exp(feat_ptr_w[anchor_idx]) * stride;
                float h = exp(feat_ptr_h[anchor_idx]) * stride;
                float x0 = x_center - w * 0.5f;
                float y0 = y_center - h * 0.5f;

                if (w < min_size.width || h < min_size.height)
                    continue;

                for (int class_idx = 0; class_idx < num_classes; class_idx++) {
                    float box_cls_score = feat_ptr[(5 + class_idx) * c_stride + anchor_idx];
                    float box_prob = box_objectness * box_cls_score;
                    if (box_prob > cls_thresh) {
                        Object obj;
                        obj.rect = skel::infer::Rect_<float>(x0, y0, w, h);
                        obj.label = class_idx;
                        obj.prob = box_prob;

                        objects.push_back(obj);
                    }
                }
            } // point anchor loop
        }

        static inline void softmax(AX_U8* src, float* dst, int length, float zero_point, float scale)
        {
            AX_U8 max_value = *std::max_element(src, src + length);
            max_value = (max_value -zero_point) * scale;
            float denominator{0};
            for (int i = 0; i < length; ++i)
            {
                dst[i] = std::exp /*fast_exp*/ ((src[i] -zero_point)*scale - max_value);
                denominator += dst[i];
            }
            for (int i = 0; i < length; ++i)
            {
                dst[i] /= denominator;
            }
        }

        static inline void generate_pico_proposals(AX_U8* pred_80_32_nhwc, int stride,
                                                   const int& model_h, const int& model_w, float prob_threshold, std::vector<Object>& objects, int num_class = 80, float scale = 1.0, float zero_point = 0)
        {
            prob_threshold = sqrt(prob_threshold * prob_threshold / scale + zero_point + 1e-9);
            const int num_grid_x = model_w / stride;
            const int num_grid_y = model_h / stride;
            // Discrete distribution parameter, see the following resources for more details:
            // [nanodet-m.yml](https://github.com/RangiLyu/nanodet/blob/main/config/nanodet-m.yml)
            // [GFL](https://arxiv.org/pdf/2006.04388.pdf)
            const int reg_max_1 = 8; // 32 / 4;
            const int channel = num_class + reg_max_1 * 4;

            for (int i = 0; i < num_grid_y; i++)
            {
                for (int j = 0; j < num_grid_x; j++)
                {
                    const int idx = i * num_grid_x + j;
                    AX_U8* scores = pred_80_32_nhwc + idx * channel;
                    // find label with max score
                    int label = -1;
                    float score = -FLT_MAX;

                    for (int k = 0; k < num_class; k++)
                    {
                        if (scores[k] > score)
                        {
                            label = k;
                            score = scores[k];
                        }
                    }
                    score = sqrt(score + 1e-9);
                    if (score >= prob_threshold)
                    {
                        float pred_ltrb[4];
                        for (int k = 0; k < 4; k++)
                        {
                            float dis = 0.f;
                            // predicted distance distribution after softmax
                            float dis_after_sm[8] = {0.};
                            softmax(scores + num_class + k * reg_max_1, dis_after_sm, 8, zero_point, scale);
                            // integral on predicted discrete distribution
                            for (int l = 0; l < reg_max_1; l++)
                            {
                                dis += l * dis_after_sm[l];
                                //printf("%2.6f ", dis_after_sm[l]);
                            }
                            //printf("\n");
                            pred_ltrb[k] = dis * stride;
                        }
                        // predict box center point
                        float pb_cx = (j + 0.5f) * stride;
                        float pb_cy = (i + 0.5f) * stride;
                        float x0 = pb_cx - pred_ltrb[0]; // left
                        float y0 = pb_cy - pred_ltrb[1]; // top
                        float x1 = pb_cx + pred_ltrb[2]; // right
                        float y1 = pb_cy + pred_ltrb[3]; // bottom

                        Object obj;
                        obj.label = label;
                        obj.rect = skel::infer::Rect_<float>(x0, y0, x1 - x0, y1 - y0);
                        obj.prob = sqrt((score * score - zero_point) * scale);
                        objects.push_back(obj);
                    }
                }
            }
        }

        static inline void reverse_letterbox(std::vector<Object>& proposals, std::vector<Object>& objects, float nms_threshold, int letterbox_rows, int letterbox_cols, int src_rows,
                                             int src_cols) {
            qsort_descent_inplace(proposals);
            std::vector<int> picked;
            nms_sorted_bboxes(proposals, picked, nms_threshold);

            float scale_letterbox;
            int resize_rows;
            int resize_cols;
            if ((letterbox_rows * 1.0 / src_rows) < (letterbox_cols * 1.0 / src_cols)) {
                scale_letterbox = letterbox_rows * 1.0 / src_rows;
            } else {
                scale_letterbox = letterbox_cols * 1.0 / src_cols;
            }
            resize_cols = int(scale_letterbox * src_cols);
            resize_rows = int(scale_letterbox * src_rows);

            int tmp_h = (letterbox_rows - resize_rows) / 2;
            int tmp_w = (letterbox_cols - resize_cols) / 2;

            float ratio_x = (float)src_cols / resize_cols;
            float ratio_y = (float)src_rows / resize_rows;

            int count = (int)picked.size();
            objects.resize(count);
            for (int i = 0; i < count; i++) {
                objects[i] = proposals[picked[i]];
                float x0 = (objects[i].rect.x);
                float y0 = (objects[i].rect.y);
                float x1 = (objects[i].rect.x + objects[i].rect.width);
                float y1 = (objects[i].rect.y + objects[i].rect.height);

                x0 = (x0 - tmp_w) * ratio_x;
                y0 = (y0 - tmp_h) * ratio_y;
                x1 = (x1 - tmp_w) * ratio_x;
                y1 = (y1 - tmp_h) * ratio_y;

                x0 = std::max(std::min(x0, (float)(src_cols - 1)), 0.f);
                y0 = std::max(std::min(y0, (float)(src_rows - 1)), 0.f);
                x1 = std::max(std::min(x1, (float)(src_cols - 1)), 0.f);
                y1 = std::max(std::min(y1, (float)(src_rows - 1)), 0.f);

                objects[i].rect.x = x0;
                objects[i].rect.y = y0;
                objects[i].rect.width = x1 - x0;
                objects[i].rect.height = y1 - y0;

                objects[i].iou_rect = objects[i].rect;
            }
        }

        static inline void get_out_bbox(std::vector<Object>& proposals, std::vector<Object>& objects, const float nms_threshold, int letterbox_rows,
                                        int letterbox_cols, int src_rows, int src_cols) {
            qsort_descent_inplace(proposals);
            std::vector<int> picked;
            nms_sorted_bboxes(proposals, picked, nms_threshold);

            float ratio_x = (float)src_cols / letterbox_cols;
            float ratio_y = (float)src_rows / letterbox_rows;

            int count = picked.size();

            objects.resize(count);
            for (int i = 0; i < count; i++) {
                objects[i] = proposals[picked[i]];
                float x0 = (objects[i].rect.x * ratio_x);
                float y0 = (objects[i].rect.y * ratio_y);
                float x1 = (objects[i].rect.x + objects[i].rect.width) * ratio_x;
                float y1 = (objects[i].rect.y + objects[i].rect.height) * ratio_y;

                x0 = std::max(std::min(x0, (float)(src_cols - 1)), 0.f);
                y0 = std::max(std::min(y0, (float)(src_rows - 1)), 0.f);
                x1 = std::max(std::min(x1, (float)(src_cols - 1)), 0.f);
                y1 = std::max(std::min(y1, (float)(src_rows - 1)), 0.f);

                objects[i].rect.x = x0;
                objects[i].rect.y = y0;
                objects[i].rect.width = x1 - x0;
                objects[i].rect.height = y1 - y0;

                objects[i].iou_rect = objects[i].rect;
            }
        }
    }
}  // namespace detection


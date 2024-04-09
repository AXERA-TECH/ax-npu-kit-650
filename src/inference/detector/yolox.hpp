/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#pragma once

#include "inference/engine_wrapper.hpp"
#include "inference/detection.hpp"

#include "utils/io.hpp"
#include "utils/frame_utils.hpp"

#include <algorithm>
#include <vector>

namespace skel {
    namespace infer {
        struct YoloXConfig
        {
            std::vector<std::vector<int>> strides;
            float cls_thresh;
            float nms_thresh;
            Size min_size;
            std::vector<int> want_classes;
            std::vector<float> zps;
            std::vector<float> scales;
        };

        class YoloX : public EngineWrapper
        {
        public:
            YoloX():
                m_isAnchorCreated(false)
            { }

            ~YoloX() = default;

            void SetConfig(const YoloXConfig& config)
            {
                m_config = config;
            }

            YoloXConfig GetConfig() const
            {
                return m_config;
            }

            int CreateAnchors()
            {
                m_anchors.resize(m_output_num);
                for (uint32_t i = 0; i < m_output_num; ++i) {
                    generate_grids_and_stride(m_input_size[1], m_input_size[0], m_config.strides[i], m_anchors[i]);
                }
                return 0;
            }

            int Detect(const AX_VIDEO_FRAME_T& img,
                    std::vector<skel::detection::Object>& outputs)
            {
                if (!m_hasInit)
                    return AX_ERR_SKEL_NOT_INIT;

                int ret = 0;

                if (!m_isAnchorCreated)
                {
                    m_isAnchorCreated = true;
                    CreateAnchors();
                }

//                ALOGD("net size: %d %d, image size: %d %d\n", m_input_size[1], m_input_size[0],
//                      img.u32Width, img.u32Height);
                if (m_input_size[0] != img.u32Height || m_input_size[1] != img.u32Width) {

                    AX_VIDEO_FRAME_T dst;
                    ret = Preprocess(img, dst);
                    if (ret != 0)
                    {
                        utils::FreeFrame(dst);
                        return ret;
                    }

//                printf("dst.size: width: %d  height: %d\n", dst.u32Width, dst.u32Height);

                    ret = Run(dst);
                    if (ret != 0)
                    {
                        utils::FreeFrame(dst);
                        return ret;
                    }
                    utils::FreeFrame(dst);
                }
                else {
                    ret = Run(img);
                    if (ret != 0)
                    {
                        return ret;
                    }
                }

                // generate proposals
                std::vector<skel::detection::Object> proposals;
                for (int i = 0; i < m_output_num; i++)
                {
                    auto& output_info = m_io_info->pOutputs[i];
                    auto& buf = m_io.pOutputs[i];
                    utils::cache_io_flush(&buf);

                    float* pfBuf = (float*)buf.pVirAddr;
                    skel::detection::generate_yolox_proposals(m_anchors[i], output_info, pfBuf,
                                                              m_config.cls_thresh, m_config.min_size, proposals);
                }

                // nms & rescale coords & select class
                outputs.clear();
                skel::detection::reverse_letterbox(proposals, outputs, m_config.nms_thresh, m_input_size[0], m_input_size[1], img.u32Height, img.u32Width);

                if (!m_config.want_classes.empty())
                {
                    for (auto it = outputs.begin(); it != outputs.end(); )
                    {
                        if (std::find(m_config.want_classes.begin(), m_config.want_classes.end(), it->label) == m_config.want_classes.end())
                            it = outputs.erase(it);
                        else
                            it++;
                    }
                }

                return 0;
            }

        protected:
            bool m_isAnchorCreated;
            std::vector<std::vector<skel::detection::GridAndStride>> m_anchors;
            YoloXConfig m_config;
        };
    }
}
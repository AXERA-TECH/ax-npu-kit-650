/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_PIPELINE_HVCFP_H
#define SKEL_PIPELINE_HVCFP_H

#include "pipeline/ax_skel_pipeline.h"
#include "pipeline/hvcfp/HVCFPDetector.h"

#include "inference/detection.hpp"
#include "tracker/byteTracker.hpp"
#include "tracker_dealer.h"

namespace skel {
    namespace ppl {
        struct HVCPConfig {
            bool track_disable;
            bool push_disable;

            HVCPConfig():
                    track_disable(false),
                    push_disable(true) {

            }
        };

        class PipelineHVCFP : public PipelineBase {
        public:
            PipelineHVCFP():
                PipelineBase(),
                m_pstApiConfig(nullptr),
                m_detect_result_queue(SKEL_DEFAULT_QUEUE_LEN),
                m_track_result_queue(SKEL_DEFAULT_QUEUE_LEN),
                m_tracker_dealer(nullptr) {

            }

            ~PipelineHVCFP() = default;

            AX_SKEL_PPL_E Type() override {
                return AX_SKEL_PPL_HVCFP;
            }

            AX_S32 Init(const AX_SKEL_HANDLE_PARAM_T *pstParam) override;
            AX_S32 DeInit() override;
            AX_S32 GetConfig(const AX_SKEL_CONFIG_T **ppstConfig) override;
            AX_S32 SetConfig(const AX_SKEL_CONFIG_T *pstConfig) override;
            AX_S32 GetResult(AX_SKEL_RESULT_T **ppstResult, AX_S32 nTimeout) override;
            AX_S32 Run() override;
            AX_S32 ResultCallbackThread() override;

        private:
            typedef struct {
                AX_SKEL_FRAME_T *pstFrame;
                std::vector<detection::Object> detResult;
            } DetQueueType;

            typedef struct {
                AX_SKEL_FRAME_T *pstFrame;
                tracker::TrackResultType trackResult;
            } TrackQueueType;

            AX_S32 InitDetector();
            AX_S32 InitTracker();
            AX_S32 DealWithParams(const AX_SKEL_HANDLE_PARAM_T *pstParam);
            AX_S32 GetDetectResult(AX_SKEL_RESULT_T **ppstResult, AX_S32 nTimeout);
            AX_S32 GetTrackResult(AX_SKEL_RESULT_T **ppstResult, AX_S32 nTimeout);
            AX_VOID FilterDetResult(std::vector<skel::detection::Object>& detResult);
            AX_VOID FilterTrackResult(tracker::TrackResultType& trackResult);
            AX_VOID ConvertTrackResult(AX_SKEL_FRAME_T* pstFrame, const tracker::TrackResultType& trackResult, AX_SKEL_RESULT_T **ppstResult);
            AX_VOID FreeResult(AX_SKEL_RESULT_T *pstResult);

        private:
            HVCPConfig m_config;
            AX_SKEL_CONFIG_T *m_pstApiConfig;
            HVCFPDetector m_detector;
            tracker::CBYTETracker m_tracker;
            tracker::BYTETrackerConfig m_tracker_config;
            utils::TimeoutQueue<DetQueueType> m_detect_result_queue;
            utils::TimeoutQueue<TrackQueueType> m_track_result_queue;
            utils::TrackerDealer *m_tracker_dealer;
            AX_SKEL_PARAM_T m_result_constrain;
        };
    }
}

#endif //SKEL_PIPELINE_HVCFP_H

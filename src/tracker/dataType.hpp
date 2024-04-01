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

#include <cstddef>
#include <vector>

#include "ax_global_type.h"
#include "Eigen/Core"
#include "Eigen/Dense"

namespace skel {
    namespace tracker {
        typedef Eigen::Matrix<float, 1, 4, Eigen::RowMajor> DETECT_BOX;
        typedef Eigen::Matrix<float, -1, 4, Eigen::RowMajor> DETECT_BOXES;
        typedef Eigen::Matrix<float, 1, 128, Eigen::RowMajor> FEATURE;
        typedef Eigen::Matrix<float, Eigen::Dynamic, 128, Eigen::RowMajor> FEATURES;
        // typedef std::vector<FEATURE> FEATURESS;

        // Kalmanfilter
        // typedef Eigen::Matrix<float, 8, 8, Eigen::RowMajor> KAL_FILTER;
        typedef Eigen::Matrix<float, 1, 8, Eigen::RowMajor> KAL_MEAN;
        typedef Eigen::Matrix<float, 8, 8, Eigen::RowMajor> KAL_COVA;
        typedef Eigen::Matrix<float, 1, 4, Eigen::RowMajor> KAL_HMEAN;
        typedef Eigen::Matrix<float, 4, 4, Eigen::RowMajor> KAL_HCOVA;
        using KAL_DATA = std::pair<KAL_MEAN, KAL_COVA>;
        using KAL_HDATA = std::pair<KAL_HMEAN, KAL_HCOVA>;

        // linear_assignment:
        typedef Eigen::Matrix<float, -1, -1, Eigen::RowMajor> DYNAMICM;
    }
}

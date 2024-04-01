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

#include "ax_engine_api.h"
#include "ax_skel_type.h"
#include "inference/cv_types.h"

#include <string>
#include <vector>
#include <cstring>
#include <array>

namespace skel
{
    namespace infer {

#define BLOB_NAME_LEN       32

        struct Blob {
            int size;
            void *data;
            char name[BLOB_NAME_LEN];

            Blob() :
                    size(0),
                    data(nullptr) {}

            Blob(const char *name_, int size_, void *data_) {
                snprintf(name, BLOB_NAME_LEN, "%s", name_);
                size = size_;
                data = new char[size];
                memcpy(data, data_, size);
            }

            Blob(const Blob &other) {
                if (&other != this) {
                    snprintf(name, BLOB_NAME_LEN, "%s", other.name);
                    size = other.size;
                    data = new char[size];
                    memcpy(data, other.data, size);
                }
            }

            Blob(Blob &&other) {
                snprintf(name, BLOB_NAME_LEN, "%s", other.name);
                data = other.data;
                size = other.size;
                other.data = nullptr;
                other.size = 0;
            }

            Blob &operator=(const Blob &other) {
                if (this == &other)
                    return *this;

                if (data)
                    delete[] (char *) data;
                snprintf(name, BLOB_NAME_LEN, "%s", other.name);
                size = other.size;
                data = new char[size];
                memcpy(data, other.data, size);
                return *this;
            }

            ~Blob() {
                if (data)
                    delete[] (char *) data;
            }
        };


        class EngineWrapper {
        public:
            EngineWrapper() :
                    m_hasInit(false),
                    m_handle(nullptr) {}

            ~EngineWrapper() = default;

            int Init(const std::string &strModelPath, AX_U32 nNpuType = 0);

            /// @brief Default preprocess: resize to m_input_size
            /// @param src
            /// @param dst
            /// @return
            int Preprocess(const AX_VIDEO_FRAME_T &src, AX_VIDEO_FRAME_T &dst, const Rect &crop_rect = Rect());

            int Run(const AX_VIDEO_FRAME_T &stFrame);

            int Release();

            inline std::array<int, 2> GetInputSize() const { return m_input_size; }

        protected:
            bool m_hasInit;
            std::array<int, 2> m_input_size;
            AX_ENGINE_HANDLE m_handle;
            AX_ENGINE_IO_INFO_T *m_io_info;
            AX_ENGINE_IO_T m_io;
            int m_input_num, m_output_num;
        };
    }
}
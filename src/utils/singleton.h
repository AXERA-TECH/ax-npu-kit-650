/**************************************************************************************************
 *
 * Copyright (c) 2019-2023 Axera Semiconductor (Ningbo) Co., Ltd. All Rights Reserved.
 *
 * This source file is the property of Axera Semiconductor (Ningbo) Co., Ltd. and
 * may not be copied or distributed in any isomorphic form without the prior
 * written consent of Axera Semiconductor (Ningbo) Co., Ltd.
 *
 **************************************************************************************************/

#ifndef SKEL_SINGLETON_H
#define SKEL_SINGLETON_H

#include <exception>
#include "ax_base_type.h"

namespace skel {
    namespace utils {
        /* https://stackoverflow.com/questions/34519073/inherit-singleton */
        template <typename T>
        class CSingleton {
        public:
            static T *GetInstance(AX_VOID) noexcept(std::is_nothrow_constructible<T>::value) {
                static T instance;
                return &instance;
            };

        protected:
            CSingleton(AX_VOID) noexcept = default;
            virtual ~CSingleton(AX_VOID) = default;

        private:
            CSingleton(const CSingleton &rhs) = delete;
            CSingleton &operator=(const CSingleton &rhs) = delete;
        };
    }
}

#endif //SKEL_SINGLETON_H

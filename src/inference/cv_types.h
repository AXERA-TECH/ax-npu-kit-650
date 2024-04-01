/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                          License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000-2008, Intel Corporation, all rights reserved.
// Copyright (C) 2009, Willow Garage Inc., all rights reserved.
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#pragma once

#ifndef __cplusplus
#  error types.hpp header must be compiled as C++
#endif

#include <climits>
#include <cfloat>
#include <vector>
#include <limits>
#include <cmath>

namespace skel
{
    namespace infer {
        template<typename _Tp> static inline _Tp saturate_cast(unsigned char v)    { return _Tp(v); }
/** @overload */
        template<typename _Tp> static inline _Tp saturate_cast(signed char v)    { return _Tp(v); }
/** @overload */
        template<typename _Tp> static inline _Tp saturate_cast(unsigned short v)   { return _Tp(v); }
/** @overload */
        template<typename _Tp> static inline _Tp saturate_cast(short v)    { return _Tp(v); }
/** @overload */
        template<typename _Tp> static inline _Tp saturate_cast(unsigned v) { return _Tp(v); }
/** @overload */
        template<typename _Tp> static inline _Tp saturate_cast(int v)      { return _Tp(v); }
/** @overload */
        template<typename _Tp> static inline _Tp saturate_cast(float v)    { return _Tp(v); }
/** @overload */
        template<typename _Tp> static inline _Tp saturate_cast(double v)   { return _Tp(v); }

        template<typename _Tp> class Complex;
        template<typename _Tp> class Point_;
        template<typename _Tp> class Point3_;
        template<typename _Tp> class Size_;
        template<typename _Tp> class Rect_;
        template<typename _Tp> class Scalar_;

        //////////////////////////////// Point_ ////////////////////////////////

        /** @brief Template class for 2D points specified by its coordinates `x` and `y`.
        
        An instance of the class is interchangeable with C structures, CvPoint and CvPoint2D32f . There is
        also a cast operator to convert point coordinates to the specified type. The conversion from
        floating-point coordinates to integer coordinates is done by rounding. Commonly, the conversion
        uses this operation for each of the coordinates. Besides the class members listed in the
        declaration above, the following operations on points are implemented:
        @code
            pt1 = pt2 + pt3;
            pt1 = pt2 - pt3;
            pt1 = pt2 * a;
            pt1 = a * pt2;
            pt1 = pt2 / a;
            pt1 += pt2;
            pt1 -= pt2;
            pt1 *= a;
            pt1 /= a;
            double value = norm(pt); // L2 norm
            pt1 == pt2;
            pt1 != pt2;
        @endcode
        For your convenience, the following type aliases are defined:
        @code
            typedef Point_<int> Point2i;
            typedef Point2i Point;
            typedef Point_<float> Point2f;
            typedef Point_<double> Point2d;
        @endcode
        Example:
        @code
            Point2f a(0.3f, 0.f), b(0.f, 0.4f);
            Point pt = (a + b)*10.f;
            cout << pt.x << ", " << pt.y << endl;
        @endcode
        */
        template<typename _Tp> class Point_
        {
        public:
            typedef _Tp value_type;

            //! default constructor
            Point_();
            Point_(_Tp _x, _Tp _y);
#if (defined(__GNUC__) && __GNUC__ < 5) && !defined(__clang__)  // GCC 4.x bug. Details: https://github.com/opencv/opencv/pull/20837
            Point_(const Point_& pt);
    Point_(Point_&& pt) noexcept = default;
#elif OPENCV_ABI_COMPATIBILITY < 500
            Point_(const Point_& pt) = default;
            Point_(Point_&& pt) noexcept = default;
#endif
            Point_(const Size_<_Tp>& sz);

#if (defined(__GNUC__) && __GNUC__ < 5) && !defined(__clang__)  // GCC 4.x bug. Details: https://github.com/opencv/opencv/pull/20837
            Point_& operator = (const Point_& pt);
            Point_& operator = (Point_&& pt) noexcept = default;
#elif OPENCV_ABI_COMPATIBILITY < 500
            Point_& operator = (const Point_& pt) = default;
            Point_& operator = (Point_&& pt) noexcept = default;
#endif
            //! conversion to another data type
            template<typename _Tp2> operator Point_<_Tp2>() const;

            //! dot product
            _Tp dot(const Point_& pt) const;
            //! dot product computed in double-precision arithmetics
            double ddot(const Point_& pt) const;
            //! cross-product
            double cross(const Point_& pt) const;
            //! checks whether the point is inside the specified rectangle
            bool inside(const Rect_<_Tp>& r) const;
            _Tp x; //!< x coordinate of the point
            _Tp y; //!< y coordinate of the point
        };

        typedef Point_<int> Point2i;
        typedef Point_<float> Point2f;
        typedef Point_<double> Point2d;
        typedef Point2i Point;
        
        //////////////////////////////// 2D Point ///////////////////////////////

        template<typename _Tp> inline
        Point_<_Tp>::Point_()
                : x(0), y(0) {}

        template<typename _Tp> inline
        Point_<_Tp>::Point_(_Tp _x, _Tp _y)
                : x(_x), y(_y) {}

#if (defined(__GNUC__) && __GNUC__ < 5) && !defined(__clang__)  // GCC 4.x bug. Details: https://github.com/opencv/opencv/pull/20837
        template<typename _Tp> inline
Point_<_Tp>::Point_(const Point_& pt)
    : x(pt.x), y(pt.y) {}
#endif

        template<typename _Tp> inline
        Point_<_Tp>::Point_(const Size_<_Tp>& sz)
                : x(sz.width), y(sz.height) {}

#if (defined(__GNUC__) && __GNUC__ < 5) && !defined(__clang__)  // GCC 4.x bug. Details: https://github.com/opencv/opencv/pull/20837
        template<typename _Tp> inline
Point_<_Tp>& Point_<_Tp>::operator = (const Point_& pt)
{
    x = pt.x; y = pt.y;
    return *this;
}
#endif

        template<typename _Tp> template<typename _Tp2> inline
        Point_<_Tp>::operator Point_<_Tp2>() const
        {
            return Point_<_Tp2>(saturate_cast<_Tp2>(x), saturate_cast<_Tp2>(y));
        }

        template<typename _Tp> inline
        _Tp Point_<_Tp>::dot(const Point_& pt) const
        {
            return saturate_cast<_Tp>(x*pt.x + y*pt.y);
        }

        template<typename _Tp> inline
        double Point_<_Tp>::ddot(const Point_& pt) const
        {
            return (double)x*(double)(pt.x) + (double)y*(double)(pt.y);
        }

        template<typename _Tp> inline
        double Point_<_Tp>::cross(const Point_& pt) const
        {
            return (double)x*pt.y - (double)y*pt.x;
        }

        template<typename _Tp> inline bool
        Point_<_Tp>::inside( const Rect_<_Tp>& r ) const
        {
            return r.contains(*this);
        }


        template<typename _Tp> static inline
        Point_<_Tp>& operator += (Point_<_Tp>& a, const Point_<_Tp>& b)
        {
            a.x += b.x;
            a.y += b.y;
            return a;
        }

        template<typename _Tp> static inline
        Point_<_Tp>& operator -= (Point_<_Tp>& a, const Point_<_Tp>& b)
        {
            a.x -= b.x;
            a.y -= b.y;
            return a;
        }

        template<typename _Tp> static inline
        Point_<_Tp>& operator *= (Point_<_Tp>& a, int b)
        {
            a.x = saturate_cast<_Tp>(a.x * b);
            a.y = saturate_cast<_Tp>(a.y * b);
            return a;
        }

        template<typename _Tp> static inline
        Point_<_Tp>& operator *= (Point_<_Tp>& a, float b)
        {
            a.x = saturate_cast<_Tp>(a.x * b);
            a.y = saturate_cast<_Tp>(a.y * b);
            return a;
        }

        template<typename _Tp> static inline
        Point_<_Tp>& operator *= (Point_<_Tp>& a, double b)
        {
            a.x = saturate_cast<_Tp>(a.x * b);
            a.y = saturate_cast<_Tp>(a.y * b);
            return a;
        }

        template<typename _Tp> static inline
        Point_<_Tp>& operator /= (Point_<_Tp>& a, int b)
        {
            a.x = saturate_cast<_Tp>(a.x / b);
            a.y = saturate_cast<_Tp>(a.y / b);
            return a;
        }

        template<typename _Tp> static inline
        Point_<_Tp>& operator /= (Point_<_Tp>& a, float b)
        {
            a.x = saturate_cast<_Tp>(a.x / b);
            a.y = saturate_cast<_Tp>(a.y / b);
            return a;
        }

        template<typename _Tp> static inline
        Point_<_Tp>& operator /= (Point_<_Tp>& a, double b)
        {
            a.x = saturate_cast<_Tp>(a.x / b);
            a.y = saturate_cast<_Tp>(a.y / b);
            return a;
        }

        template<typename _Tp> static inline
        double norm(const Point_<_Tp>& pt)
        {
            return std::sqrt((double)pt.x*pt.x + (double)pt.y*pt.y);
        }

        template<typename _Tp> static inline
        bool operator == (const Point_<_Tp>& a, const Point_<_Tp>& b)
        {
            return a.x == b.x && a.y == b.y;
        }

        template<typename _Tp> static inline
        bool operator != (const Point_<_Tp>& a, const Point_<_Tp>& b)
        {
            return a.x != b.x || a.y != b.y;
        }

        template<typename _Tp> static inline
        Point_<_Tp> operator + (const Point_<_Tp>& a, const Point_<_Tp>& b)
        {
            return Point_<_Tp>( saturate_cast<_Tp>(a.x + b.x), saturate_cast<_Tp>(a.y + b.y) );
        }

        template<typename _Tp> static inline
        Point_<_Tp> operator - (const Point_<_Tp>& a, const Point_<_Tp>& b)
        {
            return Point_<_Tp>( saturate_cast<_Tp>(a.x - b.x), saturate_cast<_Tp>(a.y - b.y) );
        }

        template<typename _Tp> static inline
        Point_<_Tp> operator - (const Point_<_Tp>& a)
        {
            return Point_<_Tp>( saturate_cast<_Tp>(-a.x), saturate_cast<_Tp>(-a.y) );
        }

        template<typename _Tp> static inline
        Point_<_Tp> operator * (const Point_<_Tp>& a, int b)
        {
            return Point_<_Tp>( saturate_cast<_Tp>(a.x*b), saturate_cast<_Tp>(a.y*b) );
        }

        template<typename _Tp> static inline
        Point_<_Tp> operator * (int a, const Point_<_Tp>& b)
        {
            return Point_<_Tp>( saturate_cast<_Tp>(b.x*a), saturate_cast<_Tp>(b.y*a) );
        }

        template<typename _Tp> static inline
        Point_<_Tp> operator * (const Point_<_Tp>& a, float b)
        {
            return Point_<_Tp>( saturate_cast<_Tp>(a.x*b), saturate_cast<_Tp>(a.y*b) );
        }

        template<typename _Tp> static inline
        Point_<_Tp> operator * (float a, const Point_<_Tp>& b)
        {
            return Point_<_Tp>( saturate_cast<_Tp>(b.x*a), saturate_cast<_Tp>(b.y*a) );
        }

        template<typename _Tp> static inline
        Point_<_Tp> operator * (const Point_<_Tp>& a, double b)
        {
            return Point_<_Tp>( saturate_cast<_Tp>(a.x*b), saturate_cast<_Tp>(a.y*b) );
        }

        template<typename _Tp> static inline
        Point_<_Tp> operator * (double a, const Point_<_Tp>& b)
        {
            return Point_<_Tp>( saturate_cast<_Tp>(b.x*a), saturate_cast<_Tp>(b.y*a) );
        }

        template<typename _Tp> static inline
        Point_<_Tp> operator / (const Point_<_Tp>& a, int b)
        {
            Point_<_Tp> tmp(a);
            tmp /= b;
            return tmp;
        }

        template<typename _Tp> static inline
        Point_<_Tp> operator / (const Point_<_Tp>& a, float b)
        {
            Point_<_Tp> tmp(a);
            tmp /= b;
            return tmp;
        }

        template<typename _Tp> static inline
        Point_<_Tp> operator / (const Point_<_Tp>& a, double b)
        {
            Point_<_Tp> tmp(a);
            tmp /= b;
            return tmp;
        }

        //////////////////////////////// Size_ ////////////////////////////////

        /** @brief Template class for specifying the size of an image or rectangle.

        The class includes two members called width and height. The structure can be converted to and from
        the old OpenCV structures CvSize and CvSize2D32f . The same set of arithmetic and comparison
        operations as for Point_ is available.

        OpenCV defines the following Size_\<\> aliases:
        @code
            typedef Size_<int> Size2i;
            typedef Size2i Size;
            typedef Size_<float> Size2f;
        @endcode
        */
        template<typename Tp> class Size_
        {
        public:
            typedef Tp value_type;

            //! default constructor
            Size_();
            Size_(Tp _width, Tp _height);
            Size_(const Size_& sz) = default;
            Size_(Size_&& sz) noexcept = default;
            Size_(const Point_<Tp>& pt);

            Size_& operator = (const Size_& sz) = default;
            Size_& operator = (Size_&& sz) noexcept = default;
            //! the area (width*height)
            Tp area() const;
            //! aspect ratio (width/height)
            double aspectRatio() const;
            //! true if empty
            bool empty() const;

            //! conversion of another data type.
            template<typename _Tp2> operator Size_<_Tp2>() const;

            Tp width; //!< the width
            Tp height; //!< the height
        };

        typedef Size_<int> Size2i;
        typedef Size_<float> Size2f;
        typedef Size_<double> Size2d;
        typedef Size2i Size;

        template<typename _Tp> inline
        Size_<_Tp>::Size_()
                : width(0), height(0) {}

        template<typename _Tp> inline
        Size_<_Tp>::Size_(_Tp _width, _Tp _height)
                : width(_width), height(_height) {}

        template<typename _Tp> inline
        Size_<_Tp>::Size_(const Point_<_Tp>& pt)
                : width(pt.x), height(pt.y) {}

        template<typename _Tp> template<typename _Tp2> inline
        Size_<_Tp>::operator Size_<_Tp2>() const
        {
            return Size_<_Tp2>(saturate_cast<_Tp2>(width), saturate_cast<_Tp2>(height));
        }

        template<typename _Tp> inline
        _Tp Size_<_Tp>::area() const
        {
            const _Tp result = width * height;
            CV_DbgAssert(!std::numeric_limits<_Tp>::is_integer
                         || width == 0 || result / width == height); // make sure the result fits in the return value
            return result;
        }

        template<typename _Tp> inline
        double Size_<_Tp>::aspectRatio() const
        {
            return width / static_cast<double>(height);
        }

        template<typename _Tp> inline
        bool Size_<_Tp>::empty() const
        {
            return width <= 0 || height <= 0;
        }


        template<typename _Tp> static inline
        Size_<_Tp>& operator *= (Size_<_Tp>& a, _Tp b)
        {
            a.width *= b;
            a.height *= b;
            return a;
        }

        template<typename _Tp> static inline
        Size_<_Tp> operator * (const Size_<_Tp>& a, _Tp b)
        {
            Size_<_Tp> tmp(a);
            tmp *= b;
            return tmp;
        }

        template<typename _Tp> static inline
        Size_<_Tp>& operator /= (Size_<_Tp>& a, _Tp b)
        {
            a.width /= b;
            a.height /= b;
            return a;
        }

        template<typename _Tp> static inline
        Size_<_Tp> operator / (const Size_<_Tp>& a, _Tp b)
        {
            Size_<_Tp> tmp(a);
            tmp /= b;
            return tmp;
        }

        template<typename _Tp> static inline
        Size_<_Tp>& operator += (Size_<_Tp>& a, const Size_<_Tp>& b)
        {
            a.width += b.width;
            a.height += b.height;
            return a;
        }

        template<typename _Tp> static inline
        Size_<_Tp> operator + (const Size_<_Tp>& a, const Size_<_Tp>& b)
        {
            Size_<_Tp> tmp(a);
            tmp += b;
            return tmp;
        }

        template<typename _Tp> static inline
        Size_<_Tp>& operator -= (Size_<_Tp>& a, const Size_<_Tp>& b)
        {
            a.width -= b.width;
            a.height -= b.height;
            return a;
        }

        template<typename _Tp> static inline
        Size_<_Tp> operator - (const Size_<_Tp>& a, const Size_<_Tp>& b)
        {
            Size_<_Tp> tmp(a);
            tmp -= b;
            return tmp;
        }

        template<typename _Tp> static inline
        bool operator == (const Size_<_Tp>& a, const Size_<_Tp>& b)
        {
            return a.width == b.width && a.height == b.height;
        }

        template<typename _Tp> static inline
        bool operator != (const Size_<_Tp>& a, const Size_<_Tp>& b)
        {
            return !(a == b);
        }

        ////////////////////////////////// Rect /////////////////////////////////
        template<typename Tp> class Rect_
        {
        public:
            typedef Tp value_type;

            //! default constructor
            Rect_();
            Rect_(Tp _x, Tp _y, Tp _width, Tp _height);

#if 0
            Rect_(const Rect_& r) = default;
    Rect_(Rect_&& r) noexcept = default;
#endif
#if 0
            Rect_& operator = (const Rect_& r) = default;
    Rect_& operator = (Rect_&& r) noexcept = default;
#endif

            //! area (width*height) of the rectangle
            Tp area() const;

            //! true if empty
            bool empty() const;

            template<typename _Tp2> operator Rect_<_Tp2>() const;

            Tp x; //!< x coordinate of the top-left corner
            Tp y; //!< y coordinate of the top-left corner
            Tp width; //!< width of the rectangle
            Tp height; //!< height of the rectangle
        };

        typedef Rect_<int> Rect2i;
        typedef Rect_<float> Rect2f;
        typedef Rect_<double> Rect2d;
        typedef Rect2i Rect;

        template<typename _Tp> inline
        Rect_<_Tp>::Rect_()
                : x(0), y(0), width(0), height(0) {}

        template<typename _Tp> inline
        Rect_<_Tp>::Rect_(_Tp _x, _Tp _y, _Tp _width, _Tp _height)
                : x(_x), y(_y), width(_width), height(_height) {}

        template<typename _Tp> inline
        _Tp Rect_<_Tp>::area() const
        {
            const _Tp result = width * height;

            return result;
        }

        template<typename _Tp> inline
        bool Rect_<_Tp>::empty() const
        {
            return width <= 0 || height <= 0;
        }

        template<typename _Tp> template<typename _Tp2> inline
        Rect_<_Tp>::operator Rect_<_Tp2>() const
        {
            return Rect_<_Tp2>(saturate_cast<_Tp2>(x), saturate_cast<_Tp2>(y), saturate_cast<_Tp2>(width), saturate_cast<_Tp2>(height));
        }

        template<typename _Tp> static inline
        Rect_<_Tp>& operator &= ( Rect_<_Tp>& a, const Rect_<_Tp>& b )
        {
            if (a.empty() || b.empty()) {
                a = Rect();
                return a;
            }
            const Rect_<_Tp>& Rx_min = (a.x < b.x) ? a : b;
            const Rect_<_Tp>& Rx_max = (a.x < b.x) ? b : a;
            const Rect_<_Tp>& Ry_min = (a.y < b.y) ? a : b;
            const Rect_<_Tp>& Ry_max = (a.y < b.y) ? b : a;
            // Looking at the formula below, we will compute Rx_min.width - (Rx_max.x - Rx_min.x)
            // but we want to avoid overflows. Rx_min.width >= 0 and (Rx_max.x - Rx_min.x) >= 0
            // by definition so the difference does not overflow. The only thing that can overflow
            // is (Rx_max.x - Rx_min.x). And it can only overflow if Rx_min.x < 0.
            // Let us first deal with the following case.
            if ((Rx_min.x < 0 && Rx_min.x + Rx_min.width < Rx_max.x) ||
                (Ry_min.y < 0 && Ry_min.y + Ry_min.height < Ry_max.y)) {
                a = Rect();
                return a;
            }
            // We now know that either Rx_min.x >= 0, or
            // Rx_min.x < 0 && Rx_min.x + Rx_min.width >= Rx_max.x and therefore
            // Rx_min.width >= (Rx_max.x - Rx_min.x) which means (Rx_max.x - Rx_min.x)
            // is inferior to a valid int and therefore does not overflow.
            a.width = std::min(Rx_min.width - (Rx_max.x - Rx_min.x), Rx_max.width);
            a.height = std::min(Ry_min.height - (Ry_max.y - Ry_min.y), Ry_max.height);
            a.x = Rx_max.x;
            a.y = Ry_max.y;
            if (a.empty())
                a = Rect();
            return a;
        }

        template<typename _Tp> static inline
        Rect_<_Tp>& operator |= ( Rect_<_Tp>& a, const Rect_<_Tp>& b )
        {
            if (a.empty()) {
                a = b;
            }
            else if (!b.empty()) {
                _Tp x1 = std::min(a.x, b.x);
                _Tp y1 = std::min(a.y, b.y);
                a.width = std::max(a.x + a.width, b.x + b.width) - x1;
                a.height = std::max(a.y + a.height, b.y + b.height) - y1;
                a.x = x1;
                a.y = y1;
            }
            return a;
        }

        template<typename _Tp> static inline
        bool operator == (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
        {
            return a.x == b.x && a.y == b.y && a.width == b.width && a.height == b.height;
        }

        template<typename _Tp> static inline
        bool operator != (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
        {
            return a.x != b.x || a.y != b.y || a.width != b.width || a.height != b.height;
        }

        template<typename _Tp> static inline
        Rect_<_Tp> operator & (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
        {
            Rect_<_Tp> c = a;
            return c &= b;
        }

        template<typename _Tp> static inline
        Rect_<_Tp> operator | (const Rect_<_Tp>& a, const Rect_<_Tp>& b)
        {
            Rect_<_Tp> c = a;
            return c |= b;
        }
    }
}
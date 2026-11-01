#pragma once

#include <cstdint>
#include <cstddef>

namespace Tenshi {

////////////////////////
//  Vector

template <typename T, std::size_t Size>
struct Vec;

// Vec2
template <typename T>
struct Vec<T, 2> {
    T x, y;

    constexpr Vec(T x, T y) : x(x), y(y) {} 
};

// Vec3
template <typename T>
struct Vec<T, 3> {
    T x, y, z;

    constexpr Vec(T x, T y, T z) : x(x), y(y), z(z) {}
    constexpr Vec(const Vec<T, 2>& v, T z) : x(v.x), y(v.y), z(z) {}
    constexpr Vec(T x, const Vec<T, 2>& v) : x(x), y(v.x), z(v.y) {}
};


////////////////////////
//  Rectangle

template <typename T>
struct Rect {
    T x, y, w, h;

    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(T x, T y, T w, T h) : x(x), y(y), w(w), h(h) {}

    // conversion constructor for different numberic types (e.g. Rect<double> to Rect<int>)
    template <typename U>
    explicit Rect(const Rect<U>& other)
    :   x(static_cast<T>(other.x)),
        y(static_cast<T>(other.y)),
        w(static_cast<T>(other.w)),
        h(static_cast<T>(other.h)) {}

    T area() const {
        return w * h;
    }

};


////////////////////////
//  Colour

struct rgba {
    int r = 0;
    int g = 0;
    int b = 0;
    int a = 0;
};

} // namespace Tenshi
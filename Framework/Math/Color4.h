#pragma once

class Color4 final
{
public:
    static const Color4 Zero;
    static const Color4 White;
    static const Color4 Black;
    static const Color4 Red;
    static const Color4 Green;
    static const Color4 Blue;

public:
    Color4();
    Color4(const float& r, const float& g, const float& b, const float& a);
    Color4(const float& value);
    Color4(const class Vector2& rhs);
    Color4(const class Vector3& rhs);
    Color4(const class Vector4& rhs);
    Color4(const Color4& rhs);
    Color4(const uint& value);
    ~Color4() = default;

    operator uint() const;
    operator float*() { return &r; }
    operator const float*() { return &r; }

    Color4& operator=(const Color4& rhs) { r = rhs.r; g = rhs.g; b = rhs.b; a = rhs.a; return *this; }

    const Color4 operator+(const Color4& rhs) const { return Color4(r + rhs.r, g + rhs.g, b + rhs.b, a + rhs.a); }
    const Color4 operator-(const Color4& rhs) const { return Color4(r - rhs.r, g - rhs.g, b - rhs.b, a - rhs.a); }
    const Color4 operator*(const Color4& rhs) const { return Color4(r * rhs.r, g * rhs.g, b * rhs.b, a * rhs.a); }
    const Color4 operator/(const Color4& rhs) const { return Color4(r / rhs.r, g / rhs.g, b / rhs.b, a / rhs.a); }

    const Color4 operator+(const float& rhs) const { return Color4(r + rhs, g + rhs, b + rhs, a + rhs); }
    const Color4 operator-(const float& rhs) const { return Color4(r - rhs, g - rhs, b - rhs, a - rhs); }
    const Color4 operator*(const float& rhs) const { return Color4(r * rhs, g * rhs, b * rhs, a * rhs); }
    const Color4 operator/(const float& rhs) const { return Color4(r / rhs, g / rhs, b / rhs, a / rhs); }

    void operator+=(const Color4& rhs) { r += rhs.r; g += rhs.g; b += rhs.b; a += rhs.a; }
    void operator-=(const Color4& rhs) { r -= rhs.r; g -= rhs.g; b -= rhs.b; a -= rhs.a; }
    void operator*=(const Color4& rhs) { r *= rhs.r; g *= rhs.g; b *= rhs.b; a *= rhs.a; }
    void operator/=(const Color4& rhs) { r /= rhs.r; g /= rhs.g; b /= rhs.b; a /= rhs.a; }

    void operator+=(const float& rhs) { r += rhs; g += rhs; b += rhs; a += rhs; }
    void operator-=(const float& rhs) { r -= rhs; g -= rhs; b -= rhs; a -= rhs; }
    void operator*=(const float& rhs) { r *= rhs; g *= rhs; b *= rhs; a *= rhs; }
    void operator/=(const float& rhs) { r /= rhs; g /= rhs; b /= rhs; a /= rhs; }

    const bool operator==(const Color4& rhs) const { return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a; }
    const bool operator!=(const Color4& rhs) const { return !(*this == rhs); }

public:
    float r;
    float g;
    float b;
    float a;
};
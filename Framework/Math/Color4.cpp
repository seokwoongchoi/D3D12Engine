#include "Framework.h"
#include "Color4.h"
#include "Vector2.h"
#include "Vector3.h"
#include "Vector4.h"

const Color4 Color4::Zero(0.0f);
const Color4 Color4::White(1.0f);
const Color4 Color4::Black(0.0f, 0.0f, 0.0f, 1.0f);
const Color4 Color4::Red(1.0f, 0.0f, 0.0f, 1.0f);
const Color4 Color4::Green(0.0f, 1.0f, 0.0f, 1.0f);
const Color4 Color4::Blue(0.0f, 0.0f, 1.0f, 1.0f);

Color4::Color4()
    : r(0.0f)
    , g(0.0f)
    , b(0.0f)
    , a(0.0f)
{
}

Color4::Color4(const float & r, const float & g, const float & b, const float & a)
    : r(r)
    , g(g)
    , b(b)
    , a(a)
{
}

Color4::Color4(const float & value)
    : r(value)
    , g(value)
    , b(value)
    , a(value)
{
}

Color4::Color4(const Vector2 & rhs)
    : r(rhs.x)
    , g(rhs.y)
    , b(0.0f)
    , a(0.0f)
{
}

Color4::Color4(const Vector3 & rhs)
    : r(rhs.x)
    , g(rhs.y)
    , b(rhs.z)
    , a(0.0f)
{
}

Color4::Color4(const Vector4 & rhs)
    : r(rhs.x)
    , g(rhs.y)
    , b(rhs.z)
    , a(rhs.w)
{
}

Color4::Color4(const Color4 & rhs)
    : r(rhs.r)
    , g(rhs.g)
    , b(rhs.b)
    , a(rhs.a)
{
}

//0xff556655

//0x0000ff55
//0x000000ff &

Color4::Color4(const uint & value)
{
    const float f = 1.0f / 255.0f;

    r = f * static_cast<float>(static_cast<unsigned char>(value >> 16));
    g = f * static_cast<float>(static_cast<unsigned char>(value >> 8));
    b = f * static_cast<float>(static_cast<unsigned char>(value >> 0));
    a = f * static_cast<float>(static_cast<unsigned char>(value >> 24));
}

Color4::operator uint() const
{
    uint temp_r = r >= 1.0f ? 0xff : r <= 0.0f ? 0x00 : static_cast<uint>(r * 255.0f);
    uint temp_g = g >= 1.0f ? 0xff : g <= 0.0f ? 0x00 : static_cast<uint>(g * 255.0f);
    uint temp_b = b >= 1.0f ? 0xff : b <= 0.0f ? 0x00 : static_cast<uint>(b * 255.0f);
    uint temp_a = a >= 1.0f ? 0xff : a <= 0.0f ? 0x00 : static_cast<uint>(a * 255.0f);

    return (temp_a << 24) | (temp_r << 16) | (temp_g << 8) | (temp_b << 0);
}

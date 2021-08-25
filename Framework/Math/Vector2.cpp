#include "Framework.h"
#include "Vector2.h"

const Vector2 Vector2::Zero(0.0f);
const Vector2 Vector2::One(1.0f);

Vector2::Vector2()
    : x(0.0f)
    , y(0.0f)
{
}

Vector2::Vector2(const float & x, const float & y)
    : x(x)
    , y(y)
{
}

Vector2::Vector2(const float & value)
    : x(value)
    , y(value)
{
}

Vector2::Vector2(const Vector2 & rhs)
    : x(rhs.x)
    , y(rhs.y)
{
}

auto Vector2::Normalize() const -> const Vector2
{
    auto factor = Length();
    factor = 1.0f / factor;

    return *this * factor;
}

void Vector2::Normalize()
{
    auto factor = Length();
    factor = 1.0f / factor;

    *this *= factor;
}

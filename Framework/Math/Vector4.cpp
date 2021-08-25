#include "Framework.h"
#include "Vector4.h"
#include "Vector2.h"
#include "Vector3.h"

const Vector4 Vector4::Zero(0.0f);
const Vector4 Vector4::One(1.0f);

Vector4::Vector4()
    : x(0.0f)
    , y(0.0f)
    , z(0.0f)
    , w(0.0f)
{

}

Vector4::Vector4(const float & x, const float & y, const float & z, const float & w)
    : x(x)
    , y(y)
    , z(z)
    , w(w)
{
	
}

Vector4::Vector4(const float & value)
    : x(value)
    , y(value)
    , z(value)
    , w(value)
{
	
}

Vector4::Vector4(const Vector2 & rhs)
    : x(rhs.x)
    , y(rhs.y)
    , z(0.0f)
    , w(0.0f)
{
	
}

Vector4::Vector4(const Vector3 & rhs)
    : x(rhs.x)
    , y(rhs.y)
    , z(rhs.z)
    , w(0.0f)
{
	
}

Vector4::Vector4(const Vector4 & rhs)
    : x(rhs.x)
    , y(rhs.y)
    , z(rhs.z)
    , w(rhs.w)
{
	
}


auto Vector4::Normalize() const -> const Vector4
{
    auto factor = Length();
    factor = 1.0f / factor;

    return *this * factor;
}

void Vector4::Normalize()
{
    auto factor = Length();
    factor = 1.0f / factor;

    *this *= factor;
}

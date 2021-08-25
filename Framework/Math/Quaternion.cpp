#include "Framework.h"
#include "Quaternion.h"
#include "Vector3.h"
#include "Matrix.h"

const Quaternion Quaternion::Identity(0.0f, 0.0f, 0.0f, 1.0f);

const Quaternion Quaternion::QuaternionFromAngleAxis(const float & angle, const Vector3 & axis)
{
    float half  = angle * 0.5f;
    float sin   = sinf(half);
    float cos   = cosf(half);

    return Quaternion
    (
        axis.x * sin,
        axis.y * sin,
        axis.z * sin,
        cos
    );
}

const Quaternion Quaternion::QuaternionFromYawPitchRoll(const float & yaw, const float & pitch, const float & roll)
{
    float halfRoll  = roll * 0.5f;
    float halfPitch = pitch * 0.5f;
    float halfYaw   = yaw * 0.5f;

    float sinRoll   = sin(halfRoll);
    float cosRoll   = cos(halfRoll);
    float sinPitch  = sin(halfPitch);
    float cosPitch  = cos(halfPitch);
    float sinYaw    = sin(halfYaw);
    float cosYaw    = cos(halfYaw);

    return Quaternion
    (
        cosYaw * sinPitch * cosRoll + sinYaw * cosPitch * sinRoll,
        sinYaw * cosPitch * cosRoll - cosYaw * sinPitch * sinRoll,
        cosYaw * cosPitch * sinRoll - sinYaw * sinPitch * cosRoll,
        cosYaw * cosPitch * cosRoll + sinYaw * sinPitch * sinRoll
    );
}

const Quaternion Quaternion::QuaternionFromEulerAngle(const Vector3 & rotation)
{
    return QuaternionFromYawPitchRoll
    (
        Math::ToRadian(rotation.y),
        Math::ToRadian(rotation.x),
        Math::ToRadian(rotation.z)
    );
}

const Quaternion Quaternion::QuaternionFromEulerAngle(const float & x, const float & y, const float & z)
{
    return QuaternionFromYawPitchRoll
    (
        Math::ToRadian(y),
        Math::ToRadian(x),
        Math::ToRadian(z)
    );
}

const Quaternion Quaternion::QuaternionFromRotation(const Vector3 & start, const Vector3 & end)
{
	Vector3 normStart;
	 Vector3::Normalize(&normStart,&start);
	 Vector3 normEnd;
	  Vector3::Normalize(&normEnd,&end);

    float d = normStart.Dot(normEnd);

    if (d > -1.0f + std::numeric_limits<float>::epsilon())
    {
        Vector3 c = normStart.Cross(normEnd);
        float s = sqrtf((1.0f + d) * 2.0f);
        float invS = 1.0f / s;

        return Quaternion
        (
            c.x * invS,
            c.y * invS,
            c.z * invS,
            0.5f * s
        );
    }
    else
    {
        Vector3 axis = Vector3::Right.Cross(normStart);
        if (axis.Length() < std::numeric_limits<float>::epsilon())
            axis = Vector3::Up.Cross(normStart);

        return QuaternionFromAngleAxis(180.0f, axis);
    }
}

const Quaternion Quaternion::QuaternionFromRotation(const Quaternion & start, const Quaternion & end)
{
    return QuaternionFromInverse(start) * end;
}

const Quaternion Quaternion::QuaternionFromLookRotation(const Vector3 & direction, const Vector3 & up)
{
    Quaternion ret;
	Vector3 forward;
	Vector3::Normalize(&forward,&direction);

    Vector3 v = forward.Cross(up);
    if (v.LengthSq() >= std::numeric_limits<float>::epsilon())
    {
        v.Normalize();
        Vector3 up      = v.Cross(forward);
        Vector3 right   = up.Cross(forward);
        ret.QuaternionFromAxis(right, up, forward);
    }
    else
        ret.QuaternionFromRotation(Vector3::Forward, forward);

    return ret;
}

const Quaternion Quaternion::QuaternionFromInverse(const Quaternion & rhs)
{
    float inverseLength = 1.0f / static_cast<float>(sqrt(rhs.LengthSq()));

    return Quaternion
    (
        -rhs.x * inverseLength,
        -rhs.y * inverseLength,
        -rhs.z * inverseLength,
        rhs.w * inverseLength
    );
}

Quaternion::Quaternion()
    : x(0.0f)
    , y(0.0f)
    , z(0.0f)
    , w(1.0f)
{
}

Quaternion::Quaternion(const float & x, const float & y, const float & z, const float & w)
    : x(x)
    , y(y)
    , z(z)
    , w(w)
{
}

void Quaternion::QuaternionFromAxis(const Vector3 & x_axis, const Vector3 & y_axis, const Vector3 & z_axis)
{
    *this = Matrix
    (
        x_axis.x, y_axis.x, z_axis.x, 0.0f,
        x_axis.y, y_axis.y, z_axis.y, 0.0f,
        x_axis.z, y_axis.z, z_axis.z, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    ).GetRotation();
}

const Vector3 Quaternion::ToEulerAngle() const
{
    // Derivation from http://www.geometrictools.com/Documentation/EulerAngles.pdf
    // Order of rotations: Z first, then X, then Y
    float check = 2.0f * (-y * z + w * x);

    if (check < -0.995f)
    {
        return Vector3
        (
            -90.0f,
            0.0f,
            -atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)) * Math::TO_DEG
        );
    }

    if (check > 0.995f)
    {
        return Vector3
        (
            90.0f,
            0.0f,
            atan2f(2.0f * (x * z - w * y), 1.0f - 2.0f * (y * y + z * z)) * Math::TO_DEG
        );
    }

    return Vector3
    (
        asinf(check) * Math::TO_DEG,
        atan2f(2.0f * (x * z + w * y), 1.0f - 2.0f * (x * x + y * y)) * Math::TO_DEG,
        atan2f(2.0f * (x * y + w * z), 1.0f - 2.0f * (x * x + z * z)) * Math::TO_DEG
    );
}

const float Quaternion::Yaw() const
{
    return ToEulerAngle().y;
}

const float Quaternion::Pitch() const
{
    return ToEulerAngle().x;
}

const float Quaternion::Roll() const
{
    return ToEulerAngle().z;
}

void Quaternion::Normalize()
{
    float inverseLength = 1.0f / static_cast<float>(sqrt(LengthSq()));

    x *= inverseLength;
    y *= inverseLength;
    z *= inverseLength;
    w *= inverseLength;
}

const Quaternion Quaternion::Normalize() const
{
    float inverseLength = 1.0f / static_cast<float>(sqrt(LengthSq()));

    return Quaternion
    (
        x * inverseLength,
        y * inverseLength,
        z * inverseLength,
        w * inverseLength
    );
}

const Quaternion Quaternion::Inverse() const
{
    float inverseLength = 1.0f / static_cast<float>(sqrt(LengthSq()));

    return Quaternion
    (
        -x * inverseLength,
        -y * inverseLength,
        -z * inverseLength,
        w * inverseLength
    );
}

Vector3 Quaternion::operator*(const Vector3 & rhs) const
{
    Vector3 qVec(x, y, z);
	
    Vector3 cross1(qVec.Cross(rhs));
    Vector3 cross2(qVec.Cross(cross1));

    return rhs + (cross1 * w + cross2) * 2.0f;
}

Quaternion Quaternion::operator*(const Quaternion & rhs) const
{
    float num4 = rhs.x;
    float num3 = rhs.y;
    float num2 = rhs.z;
    float num = rhs.w;
    float num12 = (y * num2) - (z * num3);
    float num11 = (z * num4) - (x * num2);
    float num10 = (x * num3) - (y * num4);
    float num9 = ((x * num4) + (y * num3)) + (z * num2);

    Quaternion quaternion;
    quaternion.x = ((x * num) + (num4 * w)) + num12;
    quaternion.y = ((y * num) + (num3 * w)) + num11;
    quaternion.z = ((z * num) + (num2 * w)) + num10;
    quaternion.w = (w * num) - num9;

    return quaternion;
}

Quaternion & Quaternion::operator=(const Quaternion & rhs)
{
    w = rhs.w;
    x = rhs.x;
    y = rhs.y;
    z = rhs.z;

    return *this;
}

Quaternion & Quaternion::operator*=(const float & rhs)
{
    w *= rhs;
    x *= rhs;
    y *= rhs;
    z *= rhs;

    return *this;
}

void Quaternion::operator*=(const Quaternion & rhs)
{
    float num4 = rhs.x;
    float num3 = rhs.y;
    float num2 = rhs.z;
    float num = rhs.w;
    float num12 = (y * num2) - (z * num3);
    float num11 = (z * num4) - (x * num2);
    float num10 = (x * num3) - (y * num4);
    float num9 = ((x * num4) + (y * num3)) + (z * num2);

    x = ((x * num) + (num4 * w)) + num12;
    y = ((y * num) + (num3 * w)) + num11;
    z = ((z * num) + (num2 * w)) + num10;
    w = (w * num) - num9;
}

const bool Quaternion::operator==(const Quaternion & rhs) const
{
    return 
        Math::Equals(w, rhs.w) && 
        Math::Equals(x, rhs.x) && 
        Math::Equals(y, rhs.y) && 
        Math::Equals(z, rhs.z);
}

const bool Quaternion::operator!=(const Quaternion & rhs) const
{
    return !(*this == rhs);;
}

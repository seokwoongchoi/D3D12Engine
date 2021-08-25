#pragma once

class Quaternion final
{
public:
    static const Quaternion Identity;

public:
    static const Quaternion QuaternionFromAngleAxis(const float& angle, const class Vector3& axis);
    static const Quaternion QuaternionFromYawPitchRoll(const float& yaw, const float& pitch, const float& roll);

    static const Quaternion QuaternionFromEulerAngle(const class Vector3& rotation);
    static const Quaternion QuaternionFromEulerAngle(const float& x, const float& y, const float& z);

    static const Quaternion QuaternionFromRotation(const class Vector3& start, const class Vector3& end);
    static const Quaternion QuaternionFromRotation(const Quaternion& start, const Quaternion& end);

    static const Quaternion QuaternionFromLookRotation(const class Vector3& direction, const class Vector3& up = Vector3::Up);
    static const Quaternion QuaternionFromInverse(const Quaternion& rhs);

public:
    Quaternion();
    Quaternion(const float& x, const float& y, const float& z, const float& w);
    ~Quaternion() = default;

    void QuaternionFromAxis(const class Vector3& x_axis, const class Vector3& y_axis, const class Vector3& z_axis);


    const class Vector3 ToEulerAngle() const;
    const float Yaw() const;
    const float Pitch() const;
    const float Roll() const;

    const Quaternion Conjugate()  const { return Quaternion(w, -x, -y, -z); }
    const float LengthSq() const { return x * x + y * y + z * z + w * w; }

    void Normalize();
    const Quaternion Normalize() const;
    const Quaternion Inverse() const;

    Vector3 operator*(const Vector3& rhs) const;
    Quaternion operator*(const Quaternion& rhs) const;
    Quaternion operator*(const float& rhs) const { return Quaternion(w * rhs, x * rhs, y * rhs, z*rhs); }

    Quaternion& operator=(const Quaternion& rhs);
    Quaternion& operator*=(const float& rhs);
    void operator*=(const Quaternion& rhs);

    const bool operator==(const Quaternion& rhs) const;
    const bool operator!=(const Quaternion& rhs) const;

public:
    float x;
    float y;
    float z;
    float w;
};
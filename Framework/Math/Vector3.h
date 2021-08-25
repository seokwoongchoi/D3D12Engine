#pragma once

class Vector3 final
{
public:
    static const Vector3 Zero;
    static const Vector3 One;
    static const Vector3 Right;
    static const Vector3 Left;
    static const Vector3 Up;
    static const Vector3 Down;
    static const Vector3 Forward;
    static const Vector3 Back;
    static const Vector3 Infinity;
    static const Vector3 NegInfinity;

public:
    static auto Distance(const Vector3& lhs, const Vector3& rhs) -> const float;
    static auto DistanceSq(const Vector3& lhs, const Vector3& rhs) -> const float;
    static auto Dot(const Vector3& lhs, const Vector3& rhs) -> const float;
   // static auto Cross(const Vector3& lhs, const Vector3& rhs) -> const Vector3;
	inline static void Cross(Vector3* const out, const Vector3* const lhs, const Vector3* const rhs)
	{

		float num1 = lhs->y * rhs->z - lhs->z * rhs->y;
		float num2 = lhs->z * rhs->x - lhs->x * rhs->z;
		float num3 = lhs->x * rhs->y - lhs->y * rhs->x;

		*out = Vector3(num1, num2, num3);
	}
    static void TransformCoord(Vector3* const out,const Vector3& lhs, const class Matrix* const inMatrix);
    static void TransformNormal(Vector3* const out, const Vector3& lhs, const class Matrix* const inMatrix);

  //  static const Vector3& Normalize(const Vector3& rhs);
	inline static void Normalize(Vector3* const out, const Vector3* const rhs)
	{

		auto factor = rhs->Length();
		factor = 1.0f / factor;
		Vector3 temp = *rhs;
		*out = temp * factor;
	}
public:
    Vector3();
    Vector3(const float& x, const float& y, const float& z);
    Vector3(const float& value);
    Vector3(const Vector3& rhs);
	/*Vector3(const class Vector4& rhs);*/
    Vector3(const class Vector2& rhs);

    Vector3(const class Color4& rhs);
    ~Vector3() = default;

    auto Length() const -> const float { return sqrtf(x * x + y * y + z * z); }
    auto LengthSq() const -> const float { return x * x + y * y + z * z; }
    auto Volume() const -> const float { return x * y * z; }
    auto Dot(const Vector3& rhs) const -> const float { return Dot(*this, rhs); }
    auto Cross(const Vector3& rhs) const -> const Vector3 {
		Vector3 out;
		
		Cross(&out,this, &rhs);
		return out;

	}
    auto Absolute() const -> const Vector3 { return Vector3(fabs(x), fabs(y), fabs(z)); }
    auto Normalize() const -> const Vector3;
    void Normalize();
    void Floor();

    operator float*() { return &x; }
    operator const float*() { return &x; }

    Vector3& operator=(const Vector3& rhs) { x = rhs.x; y = rhs.y; z = rhs.z; return *this; }

    const Vector3 operator+(const Vector3& rhs) const { return Vector3(x + rhs.x, y + rhs.y, z + rhs.z); }
    const Vector3 operator-(const Vector3& rhs) const { return Vector3(x - rhs.x, y - rhs.y, z - rhs.z); }
    const Vector3 operator*(const Vector3& rhs) const { return Vector3(x * rhs.x, y * rhs.y, z * rhs.z); }
    const Vector3 operator/(const Vector3& rhs) const { return Vector3(x / rhs.x, y / rhs.y, z / rhs.z); }

    const Vector3 operator+(const float& rhs) const { return Vector3(x + rhs, y + rhs, z + rhs); }
    const Vector3 operator-(const float& rhs) const { return Vector3(x - rhs, y - rhs, z - rhs); }
    const Vector3 operator*(const float& rhs) const { return Vector3(x * rhs, y * rhs, z * rhs); }
    const Vector3 operator/(const float& rhs) const { return Vector3(x / rhs, y / rhs, z / rhs); }

    void operator+=(const Vector3& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; }
    void operator-=(const Vector3& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; }
    void operator*=(const Vector3& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; }
    void operator/=(const Vector3& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; }

    void operator+=(const float& rhs) { x += rhs; y += rhs; z += rhs; }
    void operator-=(const float& rhs) { x -= rhs; y -= rhs; z -= rhs; }
    void operator*=(const float& rhs) { x *= rhs; y *= rhs; z *= rhs; }
    void operator/=(const float& rhs) { x /= rhs; y /= rhs; z /= rhs; }

    const bool operator==(const Vector3& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z; }
    const bool operator!=(const Vector3& rhs) const { return !(*this == rhs); }

	const float & operator[] (const int index) {

		switch (index)
		{
		case 0:	return x;
			break;
		case 1:return y;
			break;
		case 2:return z;
			break;
	    }
	}
public:

    float x;
    float y;
    float z;
	
};

inline Vector3 operator*(const float& lhs, const Vector3& rhs) { return rhs * lhs; }
#pragma once

class Vector4 final
{
public:
    static const Vector4 Zero;
    static const Vector4 One;

public:
    Vector4();
    Vector4(const float& x, const float& y, const float& z, const float& w);
    Vector4(const float& value);
    Vector4(const class Vector2& rhs);
	
    Vector4(const class Vector3& rhs);
    Vector4(const Vector4& rhs);
	
    ~Vector4() = default;

    auto Length() const -> const float { return sqrt(x * x + y * y + z * z + w * w); }
    auto LengthSq() const -> const float { return x * x + y * y + z * z + w * w; }
    auto Normalize() const -> const Vector4;
    void Normalize();

    operator float*() { return &x; }
    operator const float*() { return &x; }

    Vector4& operator=(const Vector4& rhs) { x = rhs.x; y = rhs.y; z = rhs.z; w = rhs.w; return *this; }

    const Vector4 operator+(const Vector4& rhs) const { return Vector4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w); }
    const Vector4 operator-(const Vector4& rhs) const { return Vector4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w); }
    const Vector4 operator*(const Vector4& rhs) const { return Vector4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w); }
    const Vector4 operator/(const Vector4& rhs) const { return Vector4(x / rhs.x, y / rhs.y, z / rhs.z, w / rhs.w); }

    const Vector4 operator+(const float& rhs) const { return Vector4(x + rhs, y + rhs, z + rhs, w + rhs); }
    const Vector4 operator-(const float& rhs) const { return Vector4(x - rhs, y - rhs, z - rhs, w - rhs); }
    const Vector4 operator*(const float& rhs) const { return Vector4(x * rhs, y * rhs, z * rhs, w * rhs); }
    const Vector4 operator/(const float& rhs) const { return Vector4(x / rhs, y / rhs, z / rhs, w / rhs); }

    void operator+=(const Vector4& rhs) { x += rhs.x; y += rhs.y; z += rhs.z; w += rhs.w; }
    void operator-=(const Vector4& rhs) { x -= rhs.x; y -= rhs.y; z -= rhs.z; w -= rhs.w; }
    void operator*=(const Vector4& rhs) { x *= rhs.x; y *= rhs.y; z *= rhs.z; w *= rhs.w; }
    void operator/=(const Vector4& rhs) { x /= rhs.x; y /= rhs.y; z /= rhs.z; w /= rhs.w; }

    void operator+=(const float& rhs) { x += rhs; y += rhs; z += rhs; w += rhs; }
    void operator-=(const float& rhs) { x -= rhs; y -= rhs; z -= rhs; w -= rhs; }
    void operator*=(const float& rhs) { x *= rhs; y *= rhs; z *= rhs; w *= rhs; }
    void operator/=(const float& rhs) { x /= rhs; y /= rhs; z /= rhs; w /= rhs; }

    const bool operator==(const Vector4& rhs) const { return x == rhs.x && y == rhs.y && z == rhs.z && w == rhs.w; }
    const bool operator!=(const Vector4& rhs) const { return !(*this == rhs); }

	const float & operator[] (const int index) {

		switch (index)
		{
		case 0:return x;
			break;
		case 1:return y;
			break;
		case 2:return z;
			break;
		case 3:return w;
			break;
		}
	}
public:
    float x;
    float y;
    float z;
    float w;


};
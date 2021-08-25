#pragma once
#include "Vector3.h"


class Matrix final
{
public:
    static const Matrix Identity;

public:
	inline static void Inverse(Matrix* const out, const Matrix* const rhs)
	{
		Matrix r;
		memcpy(&r, rhs, sizeof(class Matrix));
		float v0 = r._31 * r._42 - r._32 * r._41;
		float v1 = r._31 * r._43 - r._33 * r._41;
		float v2 = r._31 * r._44 - r._34 * r._41;
		float v3 = r._32 * r._43 - r._33 * r._42;
		float v4 = r._32 * r._44 - r._34 * r._42;
		float v5 = r._33 * r._44 - r._34 * r._43;

		float i11 = (v5 * r._22 - v4 * r._23 + v3 * r._24);
		float i21 = -(v5 * r._21 - v2 * r._23 + v1 * r._24);
		float i31 = (v4 * r._21 - v2 * r._22 + v0 * r._24);
		float i41 = -(v3 * r._21 - v1 * r._22 + v0 * r._23);

		float invDet = 1.0f / (i11 * r._11 + i21 * r._12 + i31 * r._13 + i41 * r._14);

		i11 *= invDet;
		i21 *= invDet;
		i31 *= invDet;
		i41 *= invDet;

		float i12 = -(v5 * r._12 - v4 * r._13 + v3 * r._14) * invDet;
		float i22 = (v5 * r._11 - v2 * r._13 + v1 * r._14) * invDet;
		float i32 = -(v4 * r._11 - v2 * r._12 + v0 * r._14) * invDet;
		float i42 = (v3 * r._11 - v1 * r._12 + v0 * r._13) * invDet;

		v0 = r._21 * r._42 - r._22 * r._41;
		v1 = r._21 * r._43 - r._23 * r._41;
		v2 = r._21 * r._44 - r._24 * r._41;
		v3 = r._22 * r._43 - r._23 * r._42;
		v4 = r._22 * r._44 - r._24 * r._42;
		v5 = r._23 * r._44 - r._24 * r._43;

		float i13 = (v5 * r._12 - v4 * r._13 + v3 * r._14) * invDet;
		float i23 = -(v5 * r._11 - v2 * r._13 + v1 * r._14) * invDet;
		float i33 = (v4 * r._11 - v2 * r._12 + v0 * r._14) * invDet;
		float i43 = -(v3 * r._11 - v1 * r._12 + v0 * r._13) * invDet;

		v0 = r._32 * r._21 - r._31 * r._22;
		v1 = r._33 * r._21 - r._31 * r._23;
		v2 = r._34 * r._21 - r._31 * r._24;
		v3 = r._33 * r._22 - r._32 * r._23;
		v4 = r._34 * r._22 - r._32 * r._24;
		v5 = r._34 * r._23 - r._33 * r._24;

		float i14 = -(v5 * r._12 - v4 * r._13 + v3 * r._14) * invDet;
		float i24 = (v5 * r._11 - v2 * r._13 + v1 * r._14) * invDet;
		float i34 = -(v4 * r._11 - v2 * r._12 + v0 * r._14) * invDet;
		float i44 = (v3 * r._11 - v1 * r._12 + v0 * r._13) * invDet;

		memcpy(out, &Matrix
		(
			i11, i12, i13, i14,
			i21, i22, i23, i24,
			i31, i32, i33, i34,
			i41, i42, i43, i44
		), sizeof(Matrix));
		
	}
	inline static void LookAtLH(Matrix* const view, const class Vector3& eye, const class Vector3& at, const class Vector3& up)
	{
		class Vector3 z_axis;
		class Vector3 dir = at - eye;
		Vector3::Normalize(&z_axis, &dir);

		class Vector3 x_axis;
		Vector3::Cross(&x_axis, &up, &z_axis);
		Vector3::Normalize(&x_axis, &x_axis);
		class Vector3 y_axis;
		Vector3::Cross(&y_axis, &z_axis, &x_axis);
		Vector3::Normalize(&y_axis, &y_axis);


		memcpy(view, &Matrix
		(
			x_axis.x, y_axis.x, z_axis.x, 0,
			x_axis.y, y_axis.y, z_axis.y, 0,
			x_axis.z, y_axis.z, z_axis.z, 0,
			-x_axis.Dot(eye), -y_axis.Dot(eye), -z_axis.Dot(eye), 1
		), sizeof(Matrix));
		
	}
	inline static void PerspectiveFovLH(Matrix* const out, const float& fov, const float& aspect, const float& zn, const float& zf)
	{
		auto sy = static_cast<float>(cosf(fov / 2) / sinf(fov / 2));
		auto sx = sy / aspect;

		memcpy(out, &Matrix
		(
			sx, 0, 0, 0,
			0, sy, 0, 0,
			0, 0, zf / (zf - zn), 1,
			0, 0, -zn * zf / (zf - zn), 0
		), sizeof(Matrix));
		
	}
	inline static void Scaling(Matrix* const S,const float& x, const float& y, const float& z)
	{
		memcpy(S, &Matrix
		(
			x, 0, 0, 0,
			0, y, 0, 0,
			0, 0, z, 0,
			0, 0, 0, 1
		), sizeof(Matrix));
	}
	inline static void Scaling(Matrix* const S, const class Vector3* const s)
	{
		Scaling(S,s->x, s->y, s->z);
	}

	inline static void RotationX(Matrix* const R, const float& radian)
	{
		auto s = sinf(radian);
		auto c = cosf(radian);

		memcpy(R, &Matrix
		(
			1, 0, 0, 0,
			0, c, s, 0,
			0, -s, c, 0,
			0, 0, 0, 1
		), sizeof(Matrix));
		
	}
	inline static void RotationY(Matrix* const R,const float& radian)
	{
		auto s = sinf(radian);
		auto c = cosf(radian);

		memcpy(R, &Matrix
		(
			c, 0, -s, 0,
			0, 1, 0, 0,
			s, 0, c, 0,
			0, 0, 0, 1
		), sizeof(Matrix));
	
	}
	inline static void RotationZ(Matrix* const R, const float& radian)
	{
		auto s = sinf(radian);
		auto c = cosf(radian);

		memcpy(R, &Matrix
		(
			c, s, 0, 0,
			-s, c, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		), sizeof(Matrix));
		
	}

    static const Matrix RotationQuaternion(const class Quaternion& rotation);
    static const class Quaternion RotationMatrixToQuaternion(const Matrix& rotationMat);

	inline static void Translation(Matrix* const T,const float& x, const float& y, const float& z)
	{
		T->_41 = x;
		T->_42 = y;
		T->_43 = z;
	}
	inline static void Translation(Matrix* const T, const class Vector3& t)
	{
		Translation(T, t.x, t.y, t.z);
	}

	
    static const Matrix OrthoLH(const float& w, const float& h, const float& zn, const float& zf);
    static const Matrix OrthoOffCenterLH(const float& l, const float& r, const float& b, const float& t, const float& zn, const float& zf);

	
    static const Matrix Transpose(const Matrix& rhs);

public:
    Matrix();
    Matrix
    (
        const float& _11, const float& _12, const float& _13, const float& _14,
        const float& _21, const float& _22, const float& _23, const float& _24,
        const float& _31, const float& _32, const float& _33, const float& _34,
        const float& _41, const float& _42, const float& _43, const float& _44
    );
    ~Matrix() = default;

	inline const class Vector3 GetScale()
	{
		int sign_x = Math::Sign(_11 * _12 * _13 * _14) < 0 ? -1 : 1;
		int sign_y = Math::Sign(_21 * _22 * _23 * _24) < 0 ? -1 : 1;
		int sign_z = Math::Sign(_31 * _32 * _33 * _34) < 0 ? -1 : 1;

		return Vector3
		(
			static_cast<float>(sign_x) * sqrtf(powf(_11, 2) + powf(_12, 2) + powf(_13, 2)),
			static_cast<float>(sign_y) * sqrtf(powf(_21, 2) + powf(_22, 2) + powf(_23, 2)),
			static_cast<float>(sign_z) * sqrtf(powf(_31, 2) + powf(_32, 2) + powf(_33, 2))
		);
	}
	inline const class Quaternion GetRotation()
	{
		Vector3 scale = GetScale();

		bool bCheck = false;
		bCheck |= scale.x == 0.0f;
		bCheck |= scale.y == 0.0f;
		bCheck |= scale.z == 0.0f;

		if (bCheck)
			return Quaternion::Identity;

		float factorX = 1.0f / scale.x;
		float factorY = 1.0f / scale.y;
		float factorZ = 1.0f / scale.z;

		Matrix mat;
		mat._11 = _11 * factorX; mat._12 = _12 * factorX; mat._13 = _13 * factorX; mat._14 = 0.0f;
		mat._21 = _21 * factorY; mat._22 = _22 * factorY; mat._23 = _23 * factorY; mat._24 = 0.0f;
		mat._31 = _31 * factorZ; mat._32 = _32 * factorZ; mat._33 = _33 * factorZ; mat._34 = 0.0f;
		mat._41 = 0.0f; mat._42 = 0.0f; mat._43 = 0.0f; mat._44 = 1.0f;

		return RotationMatrixToQuaternion(mat);
	}
	inline const class Vector3 GetTranslation()
	{
		return Vector3(_41, _42, _43);
	}

    const Matrix Inverse() const
	{
		Matrix out;
		Inverse(&out,this);
		return out;
	}
    const Matrix Transpose() const { return Transpose(*this); }
    void Transpose() { *this = Transpose(*this); }

    void Decompose(class Vector3& scale, class Quaternion& rotation, Vector3& translation);

    void SetIdentity();

    operator float*() { return &_11; }
    operator const float*() const { return &_11; }

    Vector3 operator*(const Vector3& rhs) const;
    const Matrix operator*(const Matrix& rhs) const;
    const bool operator==(const Matrix& rhs) const;
    const bool operator!=(const Matrix& rhs) const { return !(*this == rhs); }

public:


	union
	{
		struct
		{
			float _11, _21, _31, _41;
			float _12, _22, _32, _42;
			float _13, _23, _33, _43;
			float _14, _24, _34, _44;

		};
		
		float m[4][4];
	};
  
};

inline Vector3 operator*(const Vector3& lhs, const Matrix& rhs) { return rhs * lhs; }

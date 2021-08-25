#include "Framework.h"
#include "Matrix.h"
#include "Vector3.h"
#include "Quaternion.h"

const Matrix Matrix::Identity
(
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
);


auto Matrix::RotationQuaternion(const Quaternion & rotation) -> const Matrix
{
    float num9 = rotation.x * rotation.x;
    float num8 = rotation.y * rotation.y;
    float num7 = rotation.z * rotation.z;
    float num6 = rotation.x * rotation.y;
    float num5 = rotation.z * rotation.w;
    float num4 = rotation.z * rotation.x;
    float num3 = rotation.y * rotation.w;
    float num2 = rotation.y * rotation.z;
    float num = rotation.x * rotation.w;

    return Matrix
    (
        1.0f - (2.0f * (num8 + num7)),
        2.0f * (num6 + num5),
        2.0f * (num4 - num3),
        0.0f,
        2.0f * (num6 - num5),
        1.0f - (2.0f * (num7 + num9)),
        2.0f * (num2 + num),
        0.0f,
        2.0f * (num4 + num3),
        2.0f * (num2 - num),
        1.0f - (2.0f * (num8 + num9)),
        0.0f,
        0.0f,
        0.0f,
        0.0f,
        1.0f
    );
}

auto Matrix::RotationMatrixToQuaternion(const Matrix & rotation) -> const class Quaternion
{
    Quaternion quaternion;

    float sq;
    float half;
    float scale = rotation._11 + rotation._22 + rotation._33;

    if (scale > 0.0f)
    {
        sq = sqrt(scale + 1.0f);
        quaternion.w = sq * 0.5f;
        sq = 0.5f / sq;

        quaternion.x = (rotation._23 - rotation._32) * sq;
        quaternion.y = (rotation._31 - rotation._13) * sq;
        quaternion.z = (rotation._12 - rotation._21) * sq;

        return quaternion;
    }
    if ((rotation._11 >= rotation._22) && (rotation._11 >= rotation._33))
    {
        sq = sqrt(1.0f + rotation._11 - rotation._22 - rotation._33);
        half = 0.5f / sq;

        quaternion.x = 0.5f * sq;
        quaternion.y = (rotation._12 + rotation._21) * half;
        quaternion.z = (rotation._13 + rotation._31) * half;
        quaternion.w = (rotation._23 - rotation._32) * half;

        return quaternion;
    }
    if (rotation._22 > rotation._33)
    {
        sq = sqrt(1.0f + rotation._22 - rotation._11 - rotation._33);
        half = 0.5f / sq;

        quaternion.x = (rotation._21 + rotation._12) * half;
        quaternion.y = 0.5f * sq;
        quaternion.z = (rotation._32 + rotation._23) * half;
        quaternion.w = (rotation._31 - rotation._13) * half;

        return quaternion;
    }
    sq = sqrt(1.0f + rotation._33 - rotation._11 - rotation._22);
    half = 0.5f / sq;

    quaternion.x = (rotation._31 + rotation._13) * half;
    quaternion.y = (rotation._32 + rotation._23) * half;
    quaternion.z = 0.5f * sq;
    quaternion.w = (rotation._12 - rotation._21) * half;

    return quaternion;
}



auto Matrix::OrthoLH(const float & w, const float & h, const float & zn, const float & zf) -> const Matrix
{
    return Matrix
    (
        2 / w, 0, 0, 0,
        0, 2 / h, 0, 0,
        0, 0, 1 / (zf - zn), 0,
        0, 0, zn / (zn - zf), 1
    );
}

auto Matrix::OrthoOffCenterLH(const float & l, const float & r, const float & b, const float & t, const float & zn, const float & zf) -> const Matrix
{
    return Matrix
    (
        2 / (r - l), 0, 0, 0,
        0, 2 / (t - b), 0, 0,
        0, 0, 1 / (zf - zn), 0,
        -(r + l) / 2, -(t + b) / 2, zn / (zn - zf), 1
    );
}



auto Matrix::Transpose(const Matrix & rhs) -> const Matrix
{
    return Matrix
    (
        rhs._11, rhs._21, rhs._31, rhs._41,
        rhs._12, rhs._22, rhs._32, rhs._42,
        rhs._13, rhs._23, rhs._33, rhs._43,
        rhs._14, rhs._24, rhs._34, rhs._44
    );
}

Matrix::Matrix()
{
    SetIdentity();
}

Matrix::Matrix(const float & _11, const float & _12, const float & _13, const float & _14, const float & _21, const float & _22, const float & _23, const float & _24, const float & _31, const float & _32, const float & _33, const float & _34, const float & _41, const float & _42, const float & _43, const float & _44)
    : _11(_11), _12(_12), _13(_13), _14(_14)
    , _21(_21), _22(_22), _23(_23), _24(_24)
    , _31(_31), _32(_32), _33(_33), _34(_34)
    , _41(_41), _42(_42), _43(_43), _44(_44)
{
	/*m[0][0] = _11; m[0][1] = _12; m[0][2] = _13; m[0][3] = _14;
	m[1][0] = _21; m[1][1] = _22; m[1][2] = _23; m[1][3] = _24;
	m[2][0] = _31; m[2][1] = _32; m[2][2] = _33; m[2][3] = _34;
	m[3][0] = _41; m[3][1] = _42; m[3][2] = _43; m[3][3] = _44;*/
}




void Matrix::Decompose(Vector3 & scale, Quaternion & rotation, Vector3 & translation)
{
    scale = GetScale();
    rotation = GetRotation();
    translation = GetTranslation();
}

void Matrix::SetIdentity()
{
    _11 = 1; _12 = 0; _13 = 0; _14 = 0;
    _21 = 0; _22 = 1; _23 = 0; _24 = 0;
    _31 = 0; _32 = 0; _33 = 1; _34 = 0;
    _41 = 0; _42 = 0; _43 = 0; _44 = 1;

	/*m[0][0] = _11; m[0][1] = _12; m[0][2] = _13; m[0][3] = _14;
	m[1][0] = _21; m[1][1] = _22; m[1][2] = _23; m[1][3] = _24;
	m[2][0] = _31; m[2][1] = _32; m[2][2] = _33; m[2][3] = _34;
	m[3][0] = _41; m[3][1] = _42; m[3][2] = _43; m[3][3] = _44;*/
}

Vector3 Matrix::operator*(const Vector3 & rhs) const
{
    Vector4 temp;

    temp.x = (rhs.x * _11) + (rhs.y * _21) + (rhs.z * _31) + _41;
    temp.y = (rhs.x * _12) + (rhs.y * _22) + (rhs.z * _32) + _42;
    temp.z = (rhs.x * _13) + (rhs.y * _23) + (rhs.z * _33) + _43;
    temp.w = 1 / ((rhs.x * _14) + (rhs.y * _24) + (rhs.z * _34) + _44);

    return Vector3(temp.x * temp.w, temp.y * temp.w, temp.z * temp.w);
}

const Matrix Matrix::operator*(const Matrix & rhs) const
{
    return Matrix
    (
        _11 * rhs._11 + _12 * rhs._21 + _13 * rhs._31 + _14 * rhs._41,
        _11 * rhs._12 + _12 * rhs._22 + _13 * rhs._32 + _14 * rhs._42,
        _11 * rhs._13 + _12 * rhs._23 + _13 * rhs._33 + _14 * rhs._43,
        _11 * rhs._14 + _12 * rhs._24 + _13 * rhs._34 + _14 * rhs._44,
        _21 * rhs._11 + _22 * rhs._21 + _23 * rhs._31 + _24 * rhs._41,
        _21 * rhs._12 + _22 * rhs._22 + _23 * rhs._32 + _24 * rhs._42,
        _21 * rhs._13 + _22 * rhs._23 + _23 * rhs._33 + _24 * rhs._43,
        _21 * rhs._14 + _22 * rhs._24 + _23 * rhs._34 + _24 * rhs._44,
        _31 * rhs._11 + _32 * rhs._21 + _33 * rhs._31 + _34 * rhs._41,
        _31 * rhs._12 + _32 * rhs._22 + _33 * rhs._32 + _34 * rhs._42,
        _31 * rhs._13 + _32 * rhs._23 + _33 * rhs._33 + _34 * rhs._43,
        _31 * rhs._14 + _32 * rhs._24 + _33 * rhs._34 + _34 * rhs._44,
        _41 * rhs._11 + _42 * rhs._21 + _43 * rhs._31 + _44 * rhs._41,
        _41 * rhs._12 + _42 * rhs._22 + _43 * rhs._32 + _44 * rhs._42,
        _41 * rhs._13 + _42 * rhs._23 + _43 * rhs._33 + _44 * rhs._43,
        _41 * rhs._14 + _42 * rhs._24 + _43 * rhs._34 + _44 * rhs._44
    );
}

const bool Matrix::operator==(const Matrix & rhs) const
{
    const float* lhs_ptr = *this;
    const float* rhs_ptr = rhs;

    for (uint i = 0; i < 16; i++)
    {
        if (!Math::Equals(lhs_ptr[i], rhs_ptr[i]))
            return false;
    }

    return true;
}
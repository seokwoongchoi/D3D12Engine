//=============================================================================================
// [Global Constant]
//=============================================================================================
#define PI 3.1415926535897932384626433832795
#define INV_PI 1.0 / PI
#define EPSILON 0.00000001
#define FLT_MAX 3.402823e+38
#define FLT_MIN 1.175494e-38




///////////////////////////////////////////////////////////////////////////////
// 행렬 분해, 쿼터니온과 행렬 변환
float4 MattoQuat(matrix input)
{
    float mat[16]
    =
    {
        input._11, input._12, input._13, input._14,
        input._21, input._22, input._23, input._24,
        input._31, input._32, input._33, input._34,
        input._41, input._42, input._43, input._44,
    };
    
    float T = 1 + mat[0] + mat[5] + mat[10];
    float S, X, Y, Z, W;
    
    [branch]
    if (T > 0.00000001)      //  to avoid    large distortions
    {
        S = sqrt(T) * 2;
        X = (mat[9] - mat[6]) / S;
        Y = (mat[2] - mat[8]) / S;
        Z = (mat[4] - mat[1]) / S;
        W = 0.25 * S;
    }
    else if (mat[0] > mat[5] && mat[0] > mat[10])
    { // Column 0:
        S = sqrt(1.0 + mat[0] - mat[5] - mat[10]) * 2;
        X = 0.25 * S;
        Y = (mat[4] + mat[1]) / S;
        Z = (mat[2] + mat[8]) / S;
        W = (mat[9] - mat[6]) / S;
    }
    else if (mat[5] > mat[10])
    { // Column 1: 
        S = sqrt(1.0 + mat[5] - mat[0] - mat[10]) * 2;
        X = (mat[4] + mat[1]) / S;
        Y = 0.25 * S;
        Z = (mat[9] + mat[6]) / S;
        W = (mat[2] - mat[8]) / S;
    }
    else
    { // Column 2: 
        S = sqrt(1.0 + mat[10] - mat[0] - mat[5]) * 2;
        X = (mat[2] + mat[8]) / S;
        Y = (mat[9] + mat[6]) / S;
        Z = 0.25 * S;
        W = (mat[4] - mat[1]) / S;
    }

    
    return float4(X, Y, Z, W);
}

matrix QuattoMat(float4 quat)
{
    float mat[16];
    
    float X = quat.x;
    float Y = quat.y;
    float Z = quat.z;
    float W = quat.w;
    
    float xx = X * X;
    float xy = X * Y;
    float xz = X * Z;
    float xw = X * W;
    float yy = Y * Y;
    float yz = Y * Z;
    float yw = Y * W;
    float zz = Z * Z;
    float zw = Z * W;
    
    mat[0] = 1 - 2 * (yy + zz);
    mat[1] = 2 * (xy - zw);
    mat[2] = 2 * (xz + yw);
    mat[4] = 2 * (xy + zw);
    mat[5] = 1 - 2 * (xx + zz);
    mat[6] = 2 * (yz - xw);
    mat[8] = 2 * (xz - yw);
    mat[9] = 2 * (yz + xw);
    mat[10] = 1 - 2 * (xx + yy);
    
    mat[3] = mat[7] = mat[11] = mat[12] = mat[13] = mat[14] = 0;
    mat[15] = 1;

    matrix result;
    result._11 = mat[0];
    result._12 = mat[1];
    result._13 = mat[2];
    result._14 = mat[3];
    result._21 = mat[4];
    result._22 = mat[5];
    result._23 = mat[6];
    result._24 = mat[7];
    result._31 = mat[8];
    result._32 = mat[9];
    result._33 = mat[10];
    result._34 = mat[11];
    result._41 = mat[12];
    result._42 = mat[13];
    result._43 = mat[14];
    result._44 = mat[15];
    return result;
}

///////////////////////////////////////////////////////////////////////////////

void MatrixDecompose(out matrix S, out float4 Q, out matrix T, matrix mat)
{
    S = T = 0;
    
    matrix R = mat;
    R._14 = R._24 = R._34 = R._41 = R._42 = R._43 = 0;
    
    float3 x = R._11_12_13;
    float3 y = R._21_22_23;
    float3 z = R._31_32_33;
    
    S._11 = length(x);
    S._22 = length(y);
    S._33 = length(z);
    S._44 = mat._44;
    
    Q = MattoQuat(R);
    
    
    T._11 = T._22 = T._33 = 1;
    T._41 = mat._41;
    T._42 = mat._42;
    T._43 = mat._43;
    T._44 = mat._44;
}
float4x4 inverse(float4x4 m)
{
    float n11 = m[0][0], n12 = m[1][0], n13 = m[2][0], n14 = m[3][0];
    float n21 = m[0][1], n22 = m[1][1], n23 = m[2][1], n24 = m[3][1];
    float n31 = m[0][2], n32 = m[1][2], n33 = m[2][2], n34 = m[3][2];
    float n41 = m[0][3], n42 = m[1][3], n43 = m[2][3], n44 = m[3][3];

    float t11 = n23 * n34 * n42 - n24 * n33 * n42 + n24 * n32 * n43 - n22 * n34 * n43 - n23 * n32 * n44 + n22 * n33 * n44;
    float t12 = n14 * n33 * n42 - n13 * n34 * n42 - n14 * n32 * n43 + n12 * n34 * n43 + n13 * n32 * n44 - n12 * n33 * n44;
    float t13 = n13 * n24 * n42 - n14 * n23 * n42 + n14 * n22 * n43 - n12 * n24 * n43 - n13 * n22 * n44 + n12 * n23 * n44;
    float t14 = n14 * n23 * n32 - n13 * n24 * n32 - n14 * n22 * n33 + n12 * n24 * n33 + n13 * n22 * n34 - n12 * n23 * n34;

    float det = n11 * t11 + n21 * t12 + n31 * t13 + n41 * t14;
    float idet = 1.0f / det;

    float4x4 ret;

    ret[0][0] = t11 * idet;
    ret[0][1] = (n24 * n33 * n41 - n23 * n34 * n41 - n24 * n31 * n43 + n21 * n34 * n43 + n23 * n31 * n44 - n21 * n33 * n44) * idet;
    ret[0][2] = (n22 * n34 * n41 - n24 * n32 * n41 + n24 * n31 * n42 - n21 * n34 * n42 - n22 * n31 * n44 + n21 * n32 * n44) * idet;
    ret[0][3] = (n23 * n32 * n41 - n22 * n33 * n41 - n23 * n31 * n42 + n21 * n33 * n42 + n22 * n31 * n43 - n21 * n32 * n43) * idet;

    ret[1][0] = t12 * idet;
    ret[1][1] = (n13 * n34 * n41 - n14 * n33 * n41 + n14 * n31 * n43 - n11 * n34 * n43 - n13 * n31 * n44 + n11 * n33 * n44) * idet;
    ret[1][2] = (n14 * n32 * n41 - n12 * n34 * n41 - n14 * n31 * n42 + n11 * n34 * n42 + n12 * n31 * n44 - n11 * n32 * n44) * idet;
    ret[1][3] = (n12 * n33 * n41 - n13 * n32 * n41 + n13 * n31 * n42 - n11 * n33 * n42 - n12 * n31 * n43 + n11 * n32 * n43) * idet;

    ret[2][0] = t13 * idet;
    ret[2][1] = (n14 * n23 * n41 - n13 * n24 * n41 - n14 * n21 * n43 + n11 * n24 * n43 + n13 * n21 * n44 - n11 * n23 * n44) * idet;
    ret[2][2] = (n12 * n24 * n41 - n14 * n22 * n41 + n14 * n21 * n42 - n11 * n24 * n42 - n12 * n21 * n44 + n11 * n22 * n44) * idet;
    ret[2][3] = (n13 * n22 * n41 - n12 * n23 * n41 - n13 * n21 * n42 + n11 * n23 * n42 + n12 * n21 * n43 - n11 * n22 * n43) * idet;

    ret[3][0] = t14 * idet;
    ret[3][1] = (n13 * n24 * n31 - n14 * n23 * n31 + n14 * n21 * n33 - n11 * n24 * n33 - n13 * n21 * n34 + n11 * n23 * n34) * idet;
    ret[3][2] = (n14 * n22 * n31 - n12 * n24 * n31 - n14 * n21 * n32 + n11 * n24 * n32 + n12 * n21 * n34 - n11 * n22 * n34) * idet;
    ret[3][3] = (n12 * n23 * n31 - n13 * n22 * n31 + n13 * n21 * n32 - n11 * n23 * n32 - n12 * n21 * n33 + n11 * n22 * n33) * idet;

    return ret;
}

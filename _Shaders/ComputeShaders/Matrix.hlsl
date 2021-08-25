#define IDENTITY_MATRIX float4x4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1)
#define QUATERNION_IDENTITY float4(0, 0, 0, 1)



float UE4_FloatSelect(float4 Comparand, float4 ValueGEZero, float4 ValueLTZero)
{
    return Comparand >= 0 ? ValueGEZero : ValueLTZero;
}


float4 UE4_Slerp(float4 Quat1, float4  Quat2,float Slerp)
{
    // Get cosine of angle between quats.
    float4 RawCosom= 
            Quat1[0] * Quat2[0] +
            Quat1[1] * Quat2[1] +
            Quat1[2] * Quat2[2] +
            Quat1[3] * Quat2[3];

    // Unaligned quats - compensate, results in taking shorter route.
    float Cosom= UE4_FloatSelect(RawCosom, RawCosom, -RawCosom);
    
    float Scale0 = 0;
    float Scale1 = 0;

    if (Cosom < 0.9999)
    {
        float Omega= acos(Cosom);
        float InvSin = 1.0 / sin(Omega);

        Scale0 = sin((1.0 - Slerp) * Omega) * InvSin;
        Scale1 = sin(Slerp * Omega) * InvSin;
    }
    else
    { // Use linear interpolation.
        Scale0 = 1.0 - Slerp;
        Scale1 = Slerp;
        
    }

    // In keeping with our flipped Cosom:
    Scale1 = UE4_FloatSelect(RawCosom, Scale1, -Scale1);

    return normalize(float4(Scale0 * Quat1[0] + Scale1 * Quat2[0],
                            Scale0 * Quat1[1] + Scale1 * Quat2[1],
                            Scale0 * Quat1[2] + Scale1 * Quat2[2],
                            Scale0 * Quat1[3] + Scale1 * Quat2[3]
    ));

}

float4 q_slerp(in float4 a, in float4 b, float t)
{
    // if either input is zero, return the other.
     [branch]
    if (length(a) == 0.0)
    {
        if (length(b) == 0.0)
        {
            return QUATERNION_IDENTITY;
        }
        return b;
    }
    else if (length(b) == 0.0)
    {
        return a;
    }

    float cosHalfAngle = a.w * b.w + dot(a.xyz, b.xyz);
     [branch]
    if (cosHalfAngle >= 1.0 || cosHalfAngle <= -1.0)
    {
        return a;
    }
    else if (cosHalfAngle < 0.0)
    {
        b.xyz = -b.xyz;
        b.w = -b.w;
        cosHalfAngle = -cosHalfAngle;
    }

    float blendA;
    float blendB;
    [branch]
    if (cosHalfAngle < 0.99)
    {
        // do proper slerp for big angles
        float halfAngle = acos(cosHalfAngle);
        float sinHalfAngle = sin(halfAngle);
        float oneOverSinHalfAngle = 1.0 / sinHalfAngle;
        blendA = sin(halfAngle * (1.0 - t)) * oneOverSinHalfAngle;
        blendB = sin(halfAngle * t) * oneOverSinHalfAngle;
    }
    else
    {
        // do lerp if angle is really small.
        blendA = 1.0 - t;
        blendB = t;
    }

    float4 result = float4(blendA * a.xyz + blendB * b.xyz, blendA * a.w + blendB * b.w);
    [flatten]
    if (length(result) > 0.0)
    {
        return normalize(result);
    }
    return QUATERNION_IDENTITY;
}
float4 matrix_to_quaternion(float4x4 m)
{
    float tr = m[0][0] + m[1][1] + m[2][2];
    float4 q = float4(0, 0, 0, 0);
     [branch]
    if (tr > 0)
    {
        float s = sqrt(tr + 1.0) * 2; // S=4*qw 
        q.w = 0.25 * s;
        q.x = (m[1][2] - m[2][1]) / s;
        q.y = (m[2][0] - m[0][2]) / s;
        q.z = (m[0][1] - m[1][0]) / s;
    }
    else if ((m[0][0] > m[1][1]) && (m[0][0] > m[2][2]))
    {
        float s = sqrt(1.0 + m[0][0] - m[1][1] - m[2][2]) * 2; // S=4*qx 
        q.w = (m[1][2] - m[2][1]) / s;
        q.x = 0.25 * s;
        q.y = (m[1][0] + m[0][1]) / s;
        q.z = (m[2][0] + m[0][2]) / s;
    }
    else if (m[1][1] > m[2][2])
    {
        float s = sqrt(1.0 + m[1][1] - m[0][0] - m[2][2]) * 2; // S=4*qy
        q.w = (m[2][0] - m[0][2]) / s;
        q.x = (m[1][0] + m[0][1]) / s;
        q.y = 0.25 * s;
        q.z = (m[2][1] + m[1][2]) / s;
    }
    else
    {
        float s = sqrt(1.0 + m[2][2] - m[0][0] - m[1][1]) * 2; // S=4*qz
        q.w = (m[0][1] - m[1][0]) / s;
        q.x = (m[2][0] + m[0][2]) / s;
        q.y = (m[2][1] + m[1][2]) / s;
        q.z = 0.25 * s;
    }

    return q;
}

float4x4 m_scale(float4x4 m, float3 v)
{
    float x = v.x, y = v.y, z = v.z;

    m[0][0] *= x;
    m[0][1] *= y;
    m[0][2] *= z;
    m[1][0] *= x;
    m[1][1] *= y;
    m[1][2] *= z;
    m[2][0] *= x;
    m[2][1] *= y;
    m[2][2] *= z;
    m[3][0] *= x;
    m[3][1] *= y;
    m[3][2] *= z;

    return m;
}

float4x4 quaternion_to_matrix(float4 quat)
{
    float4x4 m = float4x4(float4(0, 0, 0, 0), float4(0, 0, 0, 0), float4(0, 0, 0, 0), float4(0, 0, 0, 0));

    float x = quat.x, y = quat.y, z = quat.z, w = quat.w;
    float x2 = x + x, y2 = y + y, z2 = z + z;
    float xx = x * x2, xy = x * y2, xz = x * z2;
    float yy = y * y2, yz = y * z2, zz = z * z2;
    float wx = w * x2, wy = w * y2, wz = w * z2;

    m[0][0] = 1.0 - (yy + zz);
    m[1][0] = xy - wz;
    m[2][0] = xz + wy;

    m[0][1] = xy + wz;
    m[1][1] = 1.0 - (xx + zz);
    m[2][1] = yz - wx;

    m[0][2] = xz - wy;
    m[1][2] = yz + wx;
    m[2][2] = 1.0 - (xx + yy);
    
    //m[0][0] = 1.0 - (yy + zz);
    //m[0][1] = xy - wz;
    //m[0][2] = xz + wy;

    //m[1][0] = xy + wz;
    //m[1][1] = 1.0 - (xx + zz);
    //m[1][2] = yz - wx;

    //m[2][0] = xz - wy;
    //m[2][1] = yz + wx;
    //m[2][2] = 1.0 - (xx + yy);

    m[3][3] = 1.0;

    return m;
}

float4x4 m_translate(float4x4 m, float3 v)
{
    float x = v.x, y = v.y, z = v.z;
    m._41 = x;
    m._42 = y;
    m._43 = z;
    return m;
}

float4x4 compose(float3 position, float4 quat, float3 scale)
{
    float4x4 m = quaternion_to_matrix(quat);
    m = m_scale(m, scale);
    m = m_translate(m, position);
    return m;
}

void decompose(in float4x4 m, out float3 position, out float4 rotation, out float3 scale)
{
    float sx = length(float3(m._11, m._12, m._13));
    float sy = length(float3(m._21, m._22, m._23));
    float sz = length(float3(m._31, m._32, m._33));

    // if determine is negative, we need to invert one scale
    float det = determinant(m);
    [flatten]
    if (det < 0)
    {
        sx = -sx;
    }

    position.x = m._41;
    position.y = m._42;
    position.z = m._43;

    // scale the rotation part

    float invSX = 1.0 / sx;
    float invSY = 1.0 / sy;
    float invSZ = 1.0 / sz;

    m[0][0] *= invSX;
    m[1][0] *= invSX;
    m[2][0] *= invSX;

    m[0][1] *= invSY;
    m[1][1] *= invSY;
    m[2][1] *= invSY;

    m[0][2] *= invSZ;
    m[1][2] *= invSZ;
    m[2][2] *= invSZ;

    rotation = matrix_to_quaternion(m);

    scale.x = sx;
    scale.y = sy;
    scale.z = sz;
}

float4x4 axis_matrix(float3 right, float3 up, float3 forward)
{
    float3 xaxis = right;
    float3 yaxis = up;
    float3 zaxis = forward;
    return float4x4(
		xaxis.x, yaxis.x, zaxis.x, 0,
		xaxis.y, yaxis.y, zaxis.y, 0,
		xaxis.z, yaxis.z, zaxis.z, 0,
		0, 0, 0, 1
	);
}

// http://stackoverflow.com/questions/349050/calculating-a-lookat-matrix
float4x4 look_at_matrix(float3 forward, float3 up)
{
    float3 xaxis = normalize(cross(forward, up));
    float3 yaxis = up;
    float3 zaxis = forward;
    return axis_matrix(xaxis, yaxis, zaxis);
}

float4x4 look_at_matrix(float3 at, float3 eye, float3 up)
{
    float3 zaxis = normalize(at - eye);
    float3 xaxis = normalize(cross(up, zaxis));
    float3 yaxis = cross(zaxis, xaxis);
    return axis_matrix(xaxis, yaxis, zaxis);
}

float4x4 extract_rotation_matrix(float4x4 m)
{
    float sx = length(float3(m[0][0], m[0][1], m[0][2]));
    float sy = length(float3(m[1][0], m[1][1], m[1][2]));
    float sz = length(float3(m[2][0], m[2][1], m[2][2]));

    // if determine is negative, we need to invert one scale
    float det = determinant(m);
    [faltten]
    if (det < 0)
    {
        sx = -sx;
    }

    float invSX = 1.0 / sx;
    float invSY = 1.0 / sy;
    float invSZ = 1.0 / sz;

    m[0][0] *= invSX;
    m[0][1] *= invSX;
    m[0][2] *= invSX;
    m[0][3] = 0;

    m[1][0] *= invSY;
    m[1][1] *= invSY;
    m[1][2] *= invSY;
    m[1][3] = 0;

    m[2][0] *= invSZ;
    m[2][1] *= invSZ;
    m[2][2] *= invSZ;
    m[2][3] = 0;

    m[3][0] = 0;
    m[3][1] = 0;
    m[3][2] = 0;
    m[3][3] = 1;

    return m;
}

float4 qmul(float4 q1, float4 q2)
{
    return float4(
        q2.xyz * q1.w + q1.xyz * q2.w + cross(q1.xyz, q2.xyz),
        q1.w * q2.w - dot(q1.xyz, q2.xyz)
    );
}
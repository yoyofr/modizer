

#include <math.h>
#include <memory.h>
#include <stdio.h>
#include "matrix.h"

Matrix44 MatrixMultiply(const Matrix44 &pm1, const Matrix44 &pm2)
{
    Matrix44 result;
    for (int i = 0; i<4; i++)
    {
        for (int j = 0; j<4; j++)
        {
            result.m[i][j] = pm1.m[i][0] * pm2.m[0][j] + pm1.m[i][1] * pm2.m[1][j] + pm1.m[i][2] * pm2.m[2][j] + pm1.m[i][3] * pm2.m[3][j];
        }
    }
    return result;
}

Matrix44 MatrixRotationX(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);

    return Matrix44 {
        1,0,0,0,
        0,c,s,0,
        0,-s,c,0,
        0,0,0,1
    };
}

Matrix44 MatrixRotationY(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    
    return Matrix44 {
        c,0,-s,0,
        0,1,0,0,
        s,0,c,0,
        0,0,0,1
    };
}


Matrix44 MatrixRotationZ(float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    
    return Matrix44 {
        c,s,0,0,
       -s,c,0,0,
        0,0,1,0,
        0,0,0,1
    };

}


Matrix44 MatrixTranslation(float x, float y, float z)
{
	 return Matrix44 {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        x,y,z,1
    };
}

Matrix44  MatrixTranslation(const Vector3 &v)
{
    return MatrixTranslation(v.x, v.y, v.z);
}


Matrix44  MatrixTranspose(const Matrix44 &m)
{
     return Matrix44 {
        m._11, m._21, m._31, m._41,
        m._12, m._22, m._32, m._42,
        m._13, m._23, m._33, m._43,
        m._14, m._24, m._34, m._44
    };
}


Matrix44 MatrixIdentity()
{
    return Matrix44 {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
}

Matrix44 MatrixOrthoLH(float l, float r, float b, float t, float zn, float zf)
{
    return Matrix44 {
        2.0f / (r - l),
        0.0f,
        0.0f,
        0.0f,
        
        0.0f,
        2.0f / (t - b),
        0.0f,
        0.0f,
        
        0.0f,
        0.0f,
//        2.0f / (zf - zn),  // -1,1 depth range
        1.0f / (zf - zn),  // 0,1 depth range
        0.0f,
        
        (l+r)/(l-r),
        (t+b)/(b-t) ,
//        (zn + zf) / (zn - zf) ,  // -1,1 depth range
        (zn) / (zn - zf) ,  // 0,1 depth range
        1.0f
    };
}


Matrix44 MatrixOrthoLH(float w, float h, float zn, float zf)
{
    w *= 0.5f;
    h *= 0.5f;
    return MatrixOrthoLH(-w, w, -h, h, zn, zf);
}


Matrix44 PerspectiveFovAspectLH(float fovY, float aspect, float nearZ, float farZ)
{
    float yscale = 1.0f / tanf(fovY * 0.5f); // 1 / tan == cot
    float xscale = yscale / aspect;
    float q = farZ / (farZ - nearZ);
    
    Matrix44 m = {
        xscale, 0.0f, 0.0f, 0.0f ,
        0.0f, yscale, 0.0f, 0.0f ,
        0.0f, 0.0f, q, 1.0f ,
        0.0f, 0.0f, q * -nearZ, 0.0f
    };
    return m;
}





void Matrix44::Print() const
{
    printf("[");
    for (int i=0; i < 16; i++)
        printf("%6.2f ", f[i]);
    printf("]\n");

}

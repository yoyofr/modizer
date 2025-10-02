

#pragma once

#include <stdint.h>
#include <math.h>

struct Matrix44
{
	union {
		struct {
			float        _11, _12, _13, _14;
			float        _21, _22, _23, _24;
			float        _31, _32, _33, _34;
			float        _41, _42, _43, _44;

		};
		float m[4][4];
        float f[16];
	};
    
    inline Matrix44() {}
    
    inline Matrix44(float f0, float f1, float f2, float f3,
             float f4, float f5, float f6, float f7,
             float f8, float f9, float f10, float f11,
             float f12, float f13, float f14, float f15)
    {
        f[0] = f0;
        f[1] = f1;
        f[2] = f2;
        f[3] = f3;
        f[4] = f4;
        f[5] = f5;
        f[6] = f6;
        f[7] = f7;
        f[8] = f8;
        f[9] = f9;
        f[10] = f10;
        f[11] = f11;
        f[12] = f12;
        f[13] = f13;
        f[14] = f14;
        f[15] = f15;
    }
    
    void Print() const;
};



struct Vector2
{
    float x, y;
    
    inline Vector2() :x(0), y(0) {}
    inline Vector2(float x, float y) : x(x), y(y) {}
    
    float Length()  const
    {
        return sqrtf( (x*x) + (y*y) );
    }
    
    float LengthSquared() const
    {
        return (x*x) + (y*y);
    }
    
 
    
    Vector2 Normalized() const
    {
        float l = Length();
        if (l > 0.0f) {
            float inv_len = 1.0f / l;
            return Vector2(x * inv_len, y * inv_len);
        } else {
            return *this;
        }
    }
    

    inline const Vector2 &operator*=(float s)
    {
        x *= s;
        y *= s;
        return *this;
    }
};

inline Vector2 operator+(const Vector2 &a, const Vector2 & b)
{
    return Vector2(a.x + b.x, a.y + b.y);
}

inline Vector2 operator-(const Vector2 & a, const Vector2 & b)
{
    return Vector2(a.x - b.x, a.y - b.y);
}

inline Vector2 operator*(const Vector2 &a, float s)
{
    return Vector2(a.x * s, a.y * s);
}


static inline float Dot(const Vector2 &p0, const Vector2 &p1)
{
    return (p0.x * p1.x) +  (p0.y * p1.y);
}



struct Vector3
{
    float x, y, z;
    
    inline Vector3() :x(0), y(0), z(0) {}
    inline Vector3(float x, float y, float z) : x(x), y(y), z(z) {}
};

struct Vector4
{
    float x, y, z, w;
    
    inline Vector4() :x(0), y(0), z(0), w(0) {}
    inline Vector4(float x, float y, float z, float w) :x(x), y(y), z(z), w(w) {}
};



Matrix44  MatrixIdentity();
Matrix44  MatrixTranslation(float x, float y, float z);
Matrix44  MatrixTranslation(const Vector3 &v);
Matrix44  MatrixRotationX(float Angle);
Matrix44  MatrixRotationY(float Angle);
Matrix44  MatrixRotationZ(float Angle);

Matrix44  MatrixMultiply(const Matrix44 &pm1, const Matrix44 &pm2);

Matrix44  MatrixOrthoLH(float w, float h, float zn, float zf);
Matrix44  MatrixOrthoLH(float left, float right, float bottom, float top, float near, float far);
Matrix44  MatrixTranspose(const Matrix44 &m);

Matrix44  PerspectiveFovAspectLH(float fovY, float aspect, float nearZ, float farZ);

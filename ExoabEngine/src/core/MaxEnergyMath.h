#pragma once
#include <cmath>
#include <string>

// FPing point precision
#define FP float

struct Vec2 {

    FP x, y;

    Vec2() : x(0), y(0) {}
    Vec2(FP x, FP y) : x(x), y(y) {}

     Vec2& add(Vec2& b) noexcept;

     Vec2& sub(Vec2& b) noexcept;

     Vec2& add(FP x, FP y);

     static Vec2 Add(Vec2& a, Vec2& b);

     static Vec2 Add(Vec2& v, FP x, FP y);

     static Vec2 Sub(Vec2& a, Vec2& b);

     Vec2& sub(FP x, FP y);

     static Vec2 Sub(Vec2& v, FP x, FP y);

     FP dot(Vec2& b);

     static FP Dot(Vec2& a, Vec2& b);

     FP magnitude();

     Vec2& normalize();

     Vec2& mul(FP x, FP y);

     static Vec2 Mul(Vec2& v, FP x, FP y);

     Vec2& div(FP x, FP y);

     static Vec2 Div(Vec2& v, FP x, FP y);

     Vec2& mul(FP a);

     static Vec2 Mul(Vec2 v, FP a);

     Vec2& div(FP a);

     static Vec2 Div(Vec2& v, FP a);

     Vec2& mul(Vec2& b);

     Vec2& div(Vec2& b);

     static Vec2 Mul(Vec2& a, Vec2& b);

     static Vec2 Div(Vec2& a, Vec2& b);

     Vec2& operator+(Vec2& b);

     Vec2& operator-(Vec2& b);

     Vec2& operator*(FP& b);

     Vec2& operator/(FP& b);

     void Set(FP x, FP y);

     std::string toString();

};

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
////////////////////////VECTOR 3/////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////


struct Vec3 {

    FP x, y, z;

    Vec3() : x(0), y(0), z(0) {}
    Vec3(FP x, FP y, FP z) : x(x), y(y), z(z) {}

    Vec3& add(Vec2& b) noexcept;
    Vec3& add(Vec3& b) noexcept;

    Vec3& sub(Vec2& b) noexcept;

    Vec3& sub(Vec3& b) noexcept;
    Vec3& add(FP x, FP y, FP z);
    Vec3& sub(FP x, FP y, FP z);
    FP dot(Vec3& b);


    static Vec3 Add(Vec3& a, Vec3& b) noexcept;
    static Vec3 Sub(Vec3& a, Vec3& b) noexcept;
    static Vec3 Add(Vec3& v, FP x, FP y, FP z);
    static Vec3 Sub(Vec3& v, FP x, FP y, FP z);
    static FP Dot(Vec3& a, Vec3& b);
    FP magnitude();

    Vec3& normalize();

    Vec3& mul(FP x, FP y, FP z);

    Vec3& div(FP x, FP y, FP z);

    Vec3& mul(FP a);

    Vec3& div(FP a);

    Vec3& mul(Vec2& b);

    Vec3& mul(Vec3& b);

    Vec3& div(Vec2& a);

    Vec3& div(Vec3& a);

    static Vec3 Mul(Vec3& v, FP x, FP y, FP z);

    static Vec3 Div(Vec3& v, FP x, FP y, FP z);

    static Vec3 Mul(Vec3& v, FP a);
    static Vec3 Div(Vec3& v, FP a);

    static Vec3 Mul(Vec3& a, Vec3& b);

    static Vec3 Div(Vec3& a, Vec3& b);


    static  Vec3 crossproduct(Vec3& a, Vec3& b);

    Vec3& operator+(Vec2& b);

    Vec3& operator+(Vec3& b);

    Vec3& operator-(Vec2& b);

    Vec3& operator-(Vec3& b);

     Vec3& operator*(FP& b);

     Vec3& operator/(FP& b);

     void Set(FP x, FP y, FP z);

     std::string toString();

};

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
////////////////////////VECTOR 4/////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////


struct Vec4 {

    FP x, y, z, w;

    Vec4() : x(0), y(0), z(0), w(1.0) {}
    Vec4(FP x, FP y, FP z, FP w) : x(x), y(y), z(z), w(w) {}
    Vec4(Vec2& v, float z, float w) : x(v.x), y(v.y), z(z), w(w) {}
    Vec4(Vec3& v) : x(v.x), y(v.y), z(v.z), w(1.0) {}

    Vec4& add(Vec2& b) noexcept;
    Vec4& add(Vec3& b) noexcept;
    Vec4& add(Vec4& b) noexcept;
    Vec4& sub(Vec2& b) noexcept;
    Vec4& sub(Vec3& b) noexcept;
    Vec4& sub(Vec4& b) noexcept;
    Vec4& add(FP x, FP y, FP z, FP w);
    Vec4& sub(FP x, FP y, FP z, FP w);
    FP dot(Vec4& b);
    FP magnitude();

     Vec4& normalize();
     Vec4& mul(FP x, FP y, FP z, FP w);
     Vec4& div(FP x, FP y, FP z, FP w);
     Vec4& mul(FP a);
     Vec4& div(FP a);
     Vec4& mul(Vec2& b);
     Vec4& mul(Vec3& b);
     Vec4& mul(Vec4& b);
     Vec4& div(Vec2& a);
     Vec4& div(Vec3& a);
     Vec4& div(Vec4& a);
     Vec4& operator+(Vec2& b);
     Vec4& operator+(Vec3& b);
     Vec4& operator+(Vec4& b);
     Vec4& operator-(Vec2& b);
     Vec4& operator-(Vec3& b);
     Vec4& operator-(Vec4& b);
     Vec4& operator*(FP& b);
     Vec4& operator/(FP& b);

     void Set(FP x, FP y, FP z, FP w);

     std::string toString();

};



/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
///////////////////////MATRIX 3x3////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

struct Mat3 {

    // first number is the hallway
    // second number is the elevator
    FP data[3][3];

    Mat3() {
        for (int i = 0; i < 3; i++)
            for (int ii = 0; ii < 3; ii++)
                data[i][ii] = 0;
        data[0][0] = 1;
        data[1][1] = 1;
        data[2][2] = 1;
    }

    Mat3(int no_identity) {}

    FP determinant();

};

// row-major
struct Mat4 {

    // first number is the hallway
    // second number is the elevator
    FP data[4][4];

    Mat4() { MakeIdentity(); }

     Mat4& MakeIdentity() {
        for (int j = 0; j < 4; j++)
            for (int k = 0; k < 4; k++)
                data[j][k] = 0;
        data[0][0] = 1.0;
        data[1][1] = 1.0;
        data[2][2] = 1.0;
        data[3][3] = 1.0;
        return *this;
    }

     Mat4& MakeTranslation(FP x, FP y, FP z);

     static  Mat4 Translation(FP x, FP y, FP z);

     Mat4& MakeTranspose();

     static  Mat4 Transpose(Mat4& mat);
    // TODO: Make Rotation Matrices

     static  Vec4& MulVec4ByMat4(Vec4& in, Vec4& out, Mat4& mat);

     static  Vec3& MulVec4ByMat4(Vec3& in, Vec3& out, Mat4& mat);

     static  Mat4 MulMat4ByMat4(Mat4& a, Mat4& b);

     static  Mat4 MulMat4ByMat4(Mat4* a, Mat4* b);

     static Mat4 MakeProjection(FP fov, FP width, FP height, FP fNear, FP fFar);
     static Mat4 MakeLookToMatrix(Vec3 position, Vec3 up, Vec3 target);
    // Only for translation and rotation (not scaling or other stuff)
     static Mat4 QuickInverse(Mat4& in);

     static Mat4 MakeLookAtMatrix(Vec3 position, Vec3 up, Vec3 target);

     static Mat4 MakeRotationZ(FP fAngle, bool isDegree);
    
     static Mat4 MakeRotationY(FP fAngle, bool isDegree);

     static Mat4 MakeRotationX(FP fAngle, bool isDegree);

     static Mat4 MakeScale(FP x, FP y, FP z);

     FP determinant();

    // please don't call this function
     Mat4 inverse();

     Vec4 operator*(Vec4& in);

     Mat4 operator*(Mat4& b);

     std::string toString();

};

Vec2 normalize_screen_point(FP x, FP y, FP window_width, FP window_height);

template<typename T>
static inline T clamp(T val, T min, T max) {
    if (val > max)
        return max;
    if (val < min)
        return min;
    return val;
}

struct Ray {
    Vec3 Origin;
    Vec3 Direction;
};
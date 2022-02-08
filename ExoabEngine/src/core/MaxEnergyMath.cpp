#include "MaxEnergyMath.h"
#if defined(_WIN32)
#include <DirectXMath.h>
#else
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cstring>
#endif

Vec2& Vec2::add(Vec2& b) noexcept {
    x += b.x;
    y += b.y;
    return *this;
}

Vec2& Vec2::sub(Vec2& b) noexcept {
    x -= b.x;
    y -= b.y;
    return *this;
}

Vec2& Vec2::add(FP x, FP y) {
    this->x += x;
    this->y += y;
    return *this;
}

Vec2 Vec2::Add(Vec2& a, Vec2& b) {
    Vec2 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    return r;
}

Vec2 Vec2::Add(Vec2& v, FP x, FP y) {
    Vec2 r;
    r.x = v.x + x;
    r.y = v.y + y;
    return r;
}

Vec2 Vec2::Sub(Vec2& a, Vec2& b) {
    Vec2 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    return r;
}

Vec2& Vec2::sub(FP x, FP y) {
    this->x -= x;
    this->y -= y;
    return *this;
}

 Vec2 Vec2::Sub(Vec2& v, FP x, FP y) {
    Vec2 r;
    r.x = v.x - x;
    r.y = v.y - y;
    return r;
}

FP Vec2::dot(Vec2& b) {
    return (x * b.x) + (y * b.y);
}

FP Vec2::Dot(Vec2& a, Vec2& b) {
    return (a.x * b.x) + (a.y * b.y);
}

FP Vec2::magnitude() {
    return sqrt(dot(*this));
}

Vec2& Vec2::normalize() {
    FP l = ((FP)1.0) / magnitude();
    x *= l;
    y *= l;
    return *this;
}

Vec2& Vec2::mul(FP x, FP y) {
    this->x *= x;
    this->y *= y;
    return *this;
}

 Vec2 Vec2::Mul(Vec2& v, FP x, FP y) {
    Vec2 r;
    r.x = v.x * x;
    r.y = v.y * y;
    return r;
}

Vec2& Vec2::div(FP x, FP y) {
    this->x /= x;
    this->y /= y;
    return *this;
}

 Vec2 Vec2::Div(Vec2& v, FP x, FP y) {
    Vec2 r;
    r.x = v.x / x;
    r.y = v.y / y;
    return r;
}

Vec2& Vec2::mul(FP a) {
    this->x *= a;
    this->y *= a;
    return *this;
}

Vec2 Vec2::Mul(Vec2 v, FP a) {
    Vec2 r;
    r.x = v.x * a;
    r.y = v.y * a;
    return r;
}

Vec2& Vec2::div(FP a) {
    this->x /= a;
    this->y /= a;
    return *this;
}

Vec2 Vec2::Div(Vec2& v, FP a) {
    Vec2 r;
    r.x = v.x / a;
    r.y = v.y / a;
    return r;
}

Vec2& Vec2::mul(Vec2& b) {
    x *= b.x;
    y *= b.y;
    return *this;
}

Vec2& Vec2::div(Vec2& b) {
    x /= b.x;
    y /= b.y;
    return *this;
}

 Vec2 Mul(Vec2& a, Vec2& b) {
    Vec2 r;
    r.x = a.x * b.x;
    r.y = a.y * b.y;
    return r;
}

Vec2 Vec2::Div(Vec2& a, Vec2& b) {
    Vec2 r;
    r.x = a.x / b.x;
    r.y = a.y / b.y;
    return r;
}

Vec2& Vec2::operator+(Vec2& b) {
    return add(b);
}

Vec2& Vec2::operator-(Vec2& b) {
    return sub(b);
}

Vec2& Vec2::operator*(FP& b) {
    return mul(b);
}

Vec2& Vec2::operator/(FP& b) {
    return div(b);
}

void Vec2::Set(FP x, FP y)
{
    this->x = x;
    this->y = y;
}

std::string Vec2::toString() {
    using namespace std;
    string str_val = "[x: " + to_string(x);
    str_val += ", y: " + to_string(y);
    str_val += "]";
    return str_val;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
////////////////////////VECTOR 3/////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

Vec3& Vec3::add(Vec2& b) noexcept {
    x += b.x;
    y += b.y;
    return *this;
}

Vec3& Vec3::add(Vec3& b) noexcept {
    x += b.x;
    y += b.y;
    z += b.z;
    return *this;
}

Vec3& Vec3::sub(Vec2& b) noexcept {
    x -= b.x;
    y -= b.y;
    return *this;
}

Vec3& Vec3::sub(Vec3& b) noexcept {
    x -= b.x;
    y -= b.y;
    z -= b.z;
    return *this;
}

Vec3& Vec3::add(FP x, FP y, FP z) {
    this->x += x;
    this->y += y;
    this->z += z;
    return *this;
}

Vec3& Vec3::sub(FP x, FP y, FP z) {
    this->x -= x;
    this->y -= y;
    this->z -= z;
    return *this;
}

FP Vec3::dot(Vec3& b) {
    return (x * b.x) + (y * b.y) + (z * b.z);
}


Vec3 Vec3::Add(Vec3& a, Vec3& b) noexcept {
    Vec3 r;
    r.x = a.x + b.x;
    r.y = a.y + b.y;
    r.z = a.z + b.z;
    return r;
}

Vec3 Vec3::Sub(Vec3& a, Vec3& b) noexcept {
    Vec3 r;
    r.x = a.x - b.x;
    r.y = a.y - b.y;
    r.z = a.z - b.z;
    return r;
}

Vec3 Vec3::Add(Vec3& v, FP x, FP y, FP z) {
    Vec3 r;
    r.x = v.x + x;
    r.y = v.y + y;
    r.z = v.z + z;
    return r;
}

Vec3 Vec3::Sub(Vec3& v, FP x, FP y, FP z) {
    Vec3 r;
    r.x = v.x - x;
    r.y = v.y - y;
    r.z = v.z - z;
    return r;
}

FP Vec3::Dot(Vec3& a, Vec3& b) {
    return (a.x * b.x) + (a.y * b.y) + (a.z * b.z);
}

FP Vec3::magnitude() {
    return sqrt(dot(*this));
}

Vec3& Vec3::normalize() {
    FP l = ((FP)1.0) / magnitude();
    x *= l;
    y *= l;
    z *= l;
    return *this;
}

Vec3& Vec3::mul(FP x, FP y, FP z) {
    this->x *= x;
    this->y *= y;
    this->z *= z;
    return *this;
}

Vec3& Vec3::div(FP x, FP y, FP z) {
    this->x /= x;
    this->y /= y;
    this->z /= z;
    return *this;
}

Vec3& Vec3::mul(FP a) {
    this->x *= a;
    this->y *= a;
    this->z *= a;
    return *this;
}

Vec3& Vec3::div(FP a) {
    FP inv_a = 1.0 / a;
    x *= inv_a;
    y *= inv_a;
    z *= inv_a;
    return *this;
}

Vec3& Vec3::mul(Vec2& b) {
    x *= b.x;
    y *= b.y;
    return *this;
}

Vec3& Vec3::mul(Vec3& b) {
    x *= b.x;
    y *= b.y;
    z *= b.z;
    return *this;
}

Vec3& Vec3::div(Vec2& a) {
    this->x /= a.x;
    this->y /= a.y;
    return *this;
}

Vec3& Vec3::div(Vec3& a) {
    this->x /= a.x;
    this->y /= a.y;
    this->z /= a.z;
    return *this;
}

Vec3 Vec3::Mul(Vec3& v, FP x, FP y, FP z) {
    Vec3 r;
    r.x = v.x * x;
    r.y = v.y * y;
    r.z = v.z * z;
    return r;
}

Vec3 Vec3::Div(Vec3& v, FP x, FP y, FP z) {
    Vec3 r;
    r.x = v.x / x;
    r.y = v.y / y;
    r.z = v.z / z;
    return r;
}

Vec3 Vec3::Mul(Vec3& v, FP a) {
    Vec3 r;
    r.x = v.x * a;
    r.y = v.y * a;
    r.z = v.z * a;
    return r;
}

Vec3 Vec3::Div(Vec3& v, FP a) {
    Vec3 r;
    r.x = v.x / a;
    r.y = v.y / a;
    r.z = v.z / a;
    return r;
}

Vec3 Vec3::Mul(Vec3& a, Vec3& b) {
    Vec3 r;
    r.x = a.x * b.x;
    r.y = a.y * b.y;
    r.z = a.z * b.z;
    return r;
}


Vec3 Vec3::Div(Vec3& a, Vec3& b) {
    Vec3 r;
    r.x = a.x / b.x;
    r.y = a.y / b.y;
    r.z = a.z / b.z;
    return r;
}


Vec3 Vec3::crossproduct(Vec3& a, Vec3& b) {
    /*
        [ i  j  k ]
        [ a1 a2 a3 ]
        [ b1 b2 b3 ]
        i: a2b3 - a3b2
        j: a1b3 - a3b1
        k: a1b2 - a2b1
    */
    FP x = a.y * b.z - a.z * b.y;
    FP y = a.x * b.z - a.z * b.x;
    FP z = a.x * b.y - a.y * b.x;
    return Vec3(x, y, z);
}

Vec3& Vec3::operator+(Vec2& b) {
    return add(b);
}

Vec3& Vec3::operator+(Vec3& b) {
    return add(b);
}

Vec3& Vec3::operator-(Vec2& b) {
    return sub(b);
}

Vec3& Vec3::operator-(Vec3& b) {
    return sub(b);
}

Vec3& Vec3::operator*(FP& b) {
    return mul(b);
}

Vec3& Vec3::operator/(FP& b) {
    return div(b);
}

void Vec3::Set(FP x, FP y, FP z)
{
    this->x = x;
    this->y = y;
    this->z = z;
}

std::string Vec3::toString() {
    using namespace std;
    string str_val = "[x: " + to_string(x);
    str_val += ", y: " + to_string(y);
    str_val += ", z: " + to_string(z);
    str_val += "]";
    return str_val;
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
////////////////////////VECTOR 4/////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
///////////////////////MATRIX 3x3////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

Vec4& Vec4::add(Vec2& b) noexcept {
    x += b.x;
    y += b.y;
    return *this;
}

Vec4& Vec4::add(Vec3& b) noexcept {
    x += b.x;
    y += b.y;
    z += b.z;
    return *this;
}

Vec4& Vec4::add(Vec4& b) noexcept {
    x += b.x;
    y += b.y;
    z += b.z;
    w += b.w;
    return *this;
}

Vec4& Vec4::sub(Vec2& b) noexcept {
    x -= b.x;
    y -= b.y;
    return *this;
}

Vec4& Vec4::sub(Vec3& b) noexcept {
    x -= b.x;
    y -= b.y;
    z -= b.z;
    return *this;
}

Vec4& Vec4::sub(Vec4& b) noexcept {
    x -= b.x;
    y -= b.y;
    z -= b.z;
    w -= b.w;
    return *this;
}

Vec4& Vec4::add(FP x, FP y, FP z, FP w) {
    this->x += x;
    this->y += y;
    this->z += z;
    this->w += w;
    return *this;
}

Vec4& Vec4::sub(FP x, FP y, FP z, FP w) {
    this->x -= x;
    this->y -= y;
    this->z -= z;
    this->w -= w;
    return *this;
}

FP Vec4::dot(Vec4& b) {
    return (x * b.x) + (y * b.y) + (z * b.z) + (w * b.w);
}

FP Vec4::magnitude() {
    return sqrt(dot(*this));
}

Vec4& Vec4::normalize() {
    FP l = ((FP)1.0) / magnitude();
    x *= l;
    y *= l;
    z *= l;
    w *= l;
    return *this;
}

Vec4& Vec4::mul(FP x, FP y, FP z, FP w) {
    this->x *= x;
    this->y *= y;
    this->z *= z;
    this->w *= w;
    return *this;
}

Vec4& Vec4::div(FP x, FP y, FP z, FP w) {
    this->x /= x;
    this->y /= y;
    this->z /= z;
    this->w /= w;
    return *this;
}

Vec4& Vec4::mul(FP a) {
    this->x *= a;
    this->y *= a;
    this->z *= a;
    this->w *= w;
    return *this;
}

Vec4& Vec4::div(FP a) {
    FP inv_a = 1.0 / a;
    x *= inv_a;
    y *= inv_a;
    z *= inv_a;
    w *= inv_a;
    return *this;
}

Vec4& Vec4::mul(Vec2& b) {
    x *= b.x;
    y *= b.y;
    return *this;
}

Vec4& Vec4::mul(Vec3& b) {
    x *= b.x;
    y *= b.y;
    z *= b.z;
    return *this;
}

Vec4& Vec4::mul(Vec4& b) {
    x *= b.x;
    y *= b.y;
    z *= b.z;
    w *= b.w;
    return *this;
}

Vec4& Vec4::div(Vec2& a) {
    this->x /= a.x;
    this->y /= a.y;
    return *this;
}

Vec4& Vec4::div(Vec3& a) {
    this->x /= a.x;
    this->y /= a.y;
    this->z /= a.z;
    return *this;
}

Vec4& Vec4::div(Vec4& a) {
    this->x /= a.x;
    this->y /= a.y;
    this->z /= a.z;
    this->w /= a.w;
    return *this;
}

Vec4& Vec4::operator+(Vec2& b) {
    return add(b);
}

Vec4& Vec4::operator+(Vec3& b) {
    return add(b);
}

Vec4& Vec4::operator+(Vec4& b) {
    return add(b);
}

Vec4& Vec4::operator-(Vec2& b) {
    return sub(b);
}

Vec4& Vec4::operator-(Vec3& b) {
    return sub(b);
}

Vec4& Vec4::operator-(Vec4& b) {
    return sub(b);
}

Vec4& Vec4::operator*(FP& b) {
    return mul(b);
}

Vec4& Vec4::operator/(FP& b) {
    return div(b);
}

void Vec4::Set(FP x, FP y, FP z, FP w)
{
    this->x = x;
    this->y = y;
    this->z = z;
    this->w = w;
}

std::string Vec4::toString() {
    using namespace std;
    string str_val = "[x: " + to_string(x);
    str_val += ", y: " + to_string(y);
    str_val += ", z: " + to_string(z);
    str_val += ", w: " + to_string(w);
    str_val += "]";
    return str_val;
}

FP Mat3::determinant() {
    /*
        [ 1 0 0 ]
        [ 0 1 0 ]
        [ 0 0 1 ]
    */
    return   data[0][0] * ((data[1][1] * data[2][2]) - (data[2][1] * data[1][2]))
        - data[1][0] * ((data[0][1] * data[2][2]) - (data[0][2] * data[2][1]))
        + data[2][0] * ((data[0][1] * data[1][2]) - (data[1][1] * data[0][2]));
}


/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////
///////////////////////MATRIX 4x4////////////////////////
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

Mat4& Mat4::MakeTranslation(FP x, FP y, FP z)
{
    data[0][3] = x;
    data[1][3] = y;
    data[2][3] = z;
    return *this;
}

  Mat4 Mat4::Translation(FP x, FP y, FP z)
{
    Mat4 m;
    m.data[0][3] = x;
    m.data[1][3] = y;
    m.data[2][3] = z;
    return m;
}

Mat4& Mat4::MakeTranspose() {
    Mat4 tmp;
    for (int k = 0; k < 4; k++)
        for (int j = 0; j < 4; j++)
            tmp.data[j][k] = data[k][j];
    memcpy(data, tmp.data, sizeof(data));
    return *this;
}

  Mat4 Mat4::Transpose(Mat4& mat) {
    Mat4 tmp;
    for (int k = 0; k < 4; k++)
        for (int j = 0; j < 4; j++)
            tmp.data[j][k] = mat.data[k][j];
    return tmp;
}

// TODO: Make Rotation Matrices

  Vec4& Mat4::MulVec4ByMat4(Vec4& in, Vec4& out, Mat4& mat) {
    /*
        [x y z w] [...matrix...]
        horizontal dotproduct * vertical dotproduct
    */
    FP x = (in.x * mat.data[0][0]) + (in.y * mat.data[1][0]) + (in.z * mat.data[2][0]) + (in.w * mat.data[3][0]);
    FP y = (in.x * mat.data[0][1]) + (in.y * mat.data[1][1]) + (in.z * mat.data[2][1]) + (in.w * mat.data[3][1]);
    FP z = (in.x * mat.data[0][2]) + (in.y * mat.data[1][2]) + (in.z * mat.data[2][2]) + (in.w * mat.data[3][2]);
    FP w = (in.x * mat.data[0][3]) + (in.y * mat.data[1][3]) + (in.z * mat.data[2][3]) + (in.w * mat.data[3][3]);
    out.x = x; out.y = y; out.z = z; out.w = w;
    return out;
}

  Vec3& Mat4::MulVec4ByMat4(Vec3& in, Vec3& out, Mat4& mat) {
    /*
        [x y z w] [...matrix...]
        horizontal dotproduct * vertical dotproduct
    */
    FP x = (in.x * mat.data[0][0]) + (in.y * mat.data[1][0]) + (in.z * mat.data[2][0]) + (1.0 * mat.data[3][0]);
    FP y = (in.x * mat.data[0][1]) + (in.y * mat.data[1][1]) + (in.z * mat.data[2][1]) + (1.0 * mat.data[3][1]);
    FP z = (in.x * mat.data[0][2]) + (in.y * mat.data[1][2]) + (in.z * mat.data[2][2]) + (1.0 * mat.data[3][2]);
    out.x = x; out.y = y; out.z = z;
    return out;
}

  Mat4 Mat4::MulMat4ByMat4(Mat4& a, Mat4& b) {
    Mat4 out;
    // dotproducts between rows(a) and cols(b)
    // first number is the hallway
    // second number is the elevator
    for (int i = 0; i < 4; i++) {
        out.data[0][i] = (a.data[0][i] * b.data[0][0]) + (a.data[1][i] * b.data[0][1]) + (a.data[2][i] * b.data[0][2]) + (a.data[3][i] * b.data[0][3]);
        out.data[1][i] = (a.data[0][i] * b.data[1][0]) + (a.data[1][i] * b.data[1][1]) + (a.data[2][i] * b.data[1][2]) + (a.data[3][i] * b.data[1][3]);
        out.data[2][i] = (a.data[0][i] * b.data[2][0]) + (a.data[1][i] * b.data[2][1]) + (a.data[2][i] * b.data[2][2]) + (a.data[3][i] * b.data[2][3]);
        out.data[3][i] = (a.data[0][i] * b.data[3][0]) + (a.data[1][i] * b.data[3][1]) + (a.data[2][i] * b.data[3][2]) + (a.data[3][i] * b.data[3][3]);
    }
    return out;
}

  Mat4 Mat4::MulMat4ByMat4(Mat4* a, Mat4* b) {
    Mat4 out;
    // dotproducts between rows(a) and cols(b)
    // first number is the hallway
    // second number is the elevator
    for (int i = 0; i < 4; i++) {
        out.data[0][i] = (a->data[0][i] * b->data[0][0]) + (a->data[1][i] * b->data[0][1]) + (a->data[2][i] * b->data[0][2]) + (a->data[3][i] * b->data[0][3]);
        out.data[1][i] = (a->data[0][i] * b->data[1][0]) + (a->data[1][i] * b->data[1][1]) + (a->data[2][i] * b->data[1][2]) + (a->data[3][i] * b->data[1][3]);
        out.data[2][i] = (a->data[0][i] * b->data[2][0]) + (a->data[1][i] * b->data[2][1]) + (a->data[2][i] * b->data[2][2]) + (a->data[3][i] * b->data[2][3]);
        out.data[3][i] = (a->data[0][i] * b->data[3][0]) + (a->data[1][i] * b->data[3][1]) + (a->data[2][i] * b->data[3][2]) + (a->data[3][i] * b->data[3][3]);
    }
    return out;
}

 Mat4 Mat4::MakeProjection(FP fov, FP width, FP height, FP fNear, FP fFar)
{
    fov = (fov * 3.142) / 180.0;
    Mat4 proj;
    proj.data[0][0] = (1.0 / (tan(fov / 2.0) * (width / height)));
    proj.data[1][1] = 1.0 / tan(fov / 2.0);
    proj.data[2][2] = fFar / (fFar - fNear);
    proj.data[2][3] = -(fFar * fNear) / (fFar - fNear);
    proj.data[3][3] = 0.0;
    proj.data[3][2] = 1.0;
    //DirectX::XMMATRIX projj = DirectX::XMMatrixPerspectiveFovLH(fov, width / height, fNear, fFar);
    //Mat4 proj2;
    //memcpy(&proj2, &projj, sizeof(proj));
    return proj;
}

 Mat4 Mat4::MakeLookToMatrix(Vec3 position, Vec3 up, Vec3 target) {
    Vec3 forward = Vec3::Sub(target, position);
    forward.normalize();

    Vec3 a = Vec3::Mul(forward, up.dot(forward));
    Vec3 newUp = Vec3::Sub(up, a);
    newUp.normalize();

    Vec3 newRight = Vec3::crossproduct(newUp, forward);

    // first number is the hallway
    // second number is the elevator
    Mat4 lookTo;
    lookTo.data[0][0] = newRight.x; lookTo.data[1][0] = newRight.y; lookTo.data[2][0] = newRight.z;
    lookTo.data[0][1] = newUp.x;    lookTo.data[1][1] = newUp.y;    lookTo.data[2][1] = newUp.z;
    lookTo.data[0][2] = forward.x;  lookTo.data[1][2] = forward.y;  lookTo.data[2][2] = forward.z;
    lookTo.data[0][3] = position.x; lookTo.data[1][3] = position.y; lookTo.data[2][3] = position.z;

    return lookTo;
}

// Only for translation and rotation (not scaling or other stuff)
 Mat4 Mat4::QuickInverse(Mat4& in) {
    Mat4 result;
    // first number is the hallway
    // second number is the elevator
    result.data[0][0] = in.data[0][0];
    result.data[1][0] = in.data[0][1];
    result.data[2][0] = in.data[0][2];
    result.data[3][0] = 0.0;

    result.data[0][1] = in.data[1][0];
    result.data[1][1] = in.data[1][1];
    result.data[2][1] = in.data[1][2];
    result.data[3][1] = 0.0;

    result.data[0][2] = in.data[2][0];
    result.data[1][2] = in.data[2][1];
    result.data[2][2] = in.data[2][2];
    result.data[3][2] = 0.0;

    Vec3 T(in.data[0][3], in.data[1][3], in.data[2][3]);
    Vec3 A(in.data[0][0], in.data[1][0], in.data[2][0]);
    Vec3 B(in.data[0][1], in.data[1][1], in.data[2][1]);
    Vec3 C(in.data[0][2], in.data[1][2], in.data[2][2]);

    result.data[0][3] = -(T.dot(A));
    result.data[1][3] = -(T.dot(B));
    result.data[2][3] = -(T.dot(C));
    result.data[3][3] = 1.0;
    return result;
}

 Mat4 Mat4::MakeLookAtMatrix(Vec3 position, Vec3 up, Vec3 target) {
    Mat4 lookTo = MakeLookToMatrix(position, up, target);
    return QuickInverse(lookTo);
}

 Mat4 Mat4::MakeRotationZ(FP fAngle, bool isDegree) {
    if (isDegree) {
        fAngle = (fAngle * 3.142) / 180.0;
    }
    FP s = sin(fAngle);
    FP c = cos(fAngle);
    Mat4 rot;
    // 1: hall
    // 2: elevator
    rot.data[0][0] = c;
    rot.data[0][1] = s;
    rot.data[0][1] = -s;
    rot.data[1][1] = c;
    return rot;
}

 Mat4 Mat4::MakeRotationY(FP fAngle, bool isDegree) {
    if (isDegree) {
        fAngle = (fAngle * 3.142) / 180.0;
    }
    FP s = sin(fAngle);
    FP c = cos(fAngle);
    Mat4 rot;
    // 1: hall
    // 2: elevator
    rot.data[0][0] = c;
    rot.data[2][0] = s;
    rot.data[0][2] = -s;
    rot.data[2][2] = c;
    return rot;
}

 Mat4 Mat4::MakeRotationX(FP fAngle, bool isDegree) {
    if (isDegree) {
        fAngle = (fAngle * 3.142) / 180.0;
    }
    FP s = (FP)sin(fAngle);
    FP c = (FP)cos(fAngle);
    Mat4 rot;

    rot.data[1][1] = c;
    rot.data[1][2] = s;
    rot.data[2][1] = -s;
    rot.data[2][2] = c;

    return rot;
}

 Mat4 Mat4::MakeScale(FP x, FP y, FP z) {
    Mat4 m;
    m.data[0][0] = x;
    m.data[1][1] = y;
    m.data[2][2] = z;
    return m;
}

FP Mat4::determinant() {
    // first number is the hallway
    // second number is the elevator
    /*
        4x4
        [ 1 0 0 0 ]
        [ 0 1 0 0 ]
        [ 0 0 1 0 ]
        [ 0 0 0 1 ]
        3x3
        [ 1 0 0 ]
        [ 0 1 0 ]
        [ 0 0 1 ]
    */
    // - + - + ...
    Mat3 a(0);
    a.data[0][0] = data[1][1];      a.data[1][0] = data[2][1];       a.data[2][0] = data[3][1];
    a.data[0][1] = data[1][2];      a.data[1][1] = data[2][2];       a.data[2][1] = data[3][2];
    a.data[0][2] = data[1][3];      a.data[1][2] = data[2][3];       a.data[2][2] = data[3][3];

    Mat3 b(0);
    b.data[0][0] = data[0][1];      b.data[1][0] = data[2][1];       b.data[2][0] = data[3][1];
    b.data[0][1] = data[0][2];      b.data[1][1] = data[2][2];       b.data[2][1] = data[3][2];
    b.data[0][2] = data[0][3];      b.data[1][2] = data[2][3];       b.data[2][2] = data[3][3];

    Mat3 c(0);
    c.data[0][0] = data[0][1];      c.data[1][0] = data[1][1];       c.data[2][0] = data[3][1];
    c.data[0][1] = data[0][2];      c.data[1][1] = data[1][2];       c.data[2][1] = data[3][2];
    c.data[0][2] = data[0][3];      c.data[1][2] = data[1][3];       c.data[2][2] = data[3][3];

    Mat3 d(0);
    d.data[0][0] = data[0][1];      d.data[1][0] = data[1][1];       d.data[2][0] = data[2][1];
    d.data[0][1] = data[0][2];      d.data[1][1] = data[1][2];       d.data[2][1] = data[2][2];
    d.data[0][2] = data[0][3];      d.data[1][2] = data[1][3];       d.data[2][2] = data[2][3];

    return (data[0][0] * a.determinant()) - (data[1][0] * b.determinant()) + (data[2][0] * c.determinant()) - (data[3][0] * d.determinant());
}

Mat4 Mat4::inverse() {
#if defined(_WIN32)
    DirectX::XMMATRIX m;
    memcpy(&m, this->data, sizeof(float) * 16);
    DirectX::XMVECTOR det = DirectX::XMMatrixDeterminant(m);
    DirectX::XMMATRIX inverse_matrix = DirectX::XMMatrixInverse(&det, m);
    Mat4 result;
    memcpy(result.data, &inverse_matrix, sizeof(float) * 16);
    return result;
#else
    glm::mat4 matrix;
    memcpy(&matrix, this->data, sizeof(float) * 16);
    glm::mat4 invmat = glm::inverse(matrix);
    Mat4 result;
    memcpy(&result, &invmat, sizeof(float) * 16);
    return result;
#endif
}

Vec4 Mat4::operator*(Vec4& in) {
    Vec4 result;
    MulVec4ByMat4(in, result, *this);
    return result;
}

Mat4 Mat4::operator*(Mat4& b) {
    return MulMat4ByMat4(*this, b);
}

std::string Mat4::toString() {
    using namespace std;
    string space = " ";
    string str_val
        = "[ " + to_string(data[0][0]) + space + to_string(data[1][0]) + space + to_string(data[2][0]) + space + to_string(data[3][0]) + " ]\n";
    str_val += "[ " + to_string(data[0][1]) + space + to_string(data[1][1]) + space + to_string(data[2][1]) + space + to_string(data[3][1]) + " ]\n";
    str_val += "[ " + to_string(data[0][2]) + space + to_string(data[1][2]) + space + to_string(data[2][2]) + space + to_string(data[3][2]) + " ]\n";
    str_val += "[ " + to_string(data[0][3]) + space + to_string(data[1][3]) + space + to_string(data[2][3]) + space + to_string(data[3][3]) + " ]\n";
    return str_val;
}

Vec2 normalize_screen_point(FP x, FP y, FP window_width, FP window_height) {

    FP _x = (x / (window_width * 0.5f)) - 1.0f;
    FP _y = (y / (window_height * 0.5f)) - 1.0f;
    return { _x, _y };
}

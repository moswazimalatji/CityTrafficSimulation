#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <GL/gl.h>
#include <cmath>
#include <iostream>

class Vec3
{
public:

    Vec3();
    Vec3(float a, float b, float c);

    float x,y,z;
    static float dst(Vec3 b, Vec3 e);
    static float length(Vec3 a);
    static Vec3 lerp(Vec3 b, Vec3 e, float s);
    static Vec3 cross(Vec3 u, Vec3 v);
    static float angleDiff(float b, float e);

    float angleXZ();

    void normalize();

    Vec3& operator += (const Vec3& right);
    Vec3& operator -= (const Vec3& right);
    Vec3& operator *= (const float right);
    Vec3& operator /= (const float right);

    //friend std::istream& operator >> (std::istream& in, Vec3& right);
};

Vec3 operator + (Vec3 left, const Vec3& right);
Vec3 operator - (Vec3 left, const Vec3& right);
Vec3 operator * (Vec3 left, const float right);
Vec3 operator / (Vec3 left, const float right);

std::ostream& operator << (std::ostream& out, const Vec3& right);

//std::istream& operator >> (std::istream& in, Vec3& right);

class Graphics
{
protected: public:
    virtual void draw();
    void drawCube(float a);
    void drawCube(float x, float y, float z);
    void drawLine(Vec3 begP, Vec3 endP);
    void setColor(float r, float g, float b);

    void drawVertex(Vec3 a);
    void drawQuad(Vec3 a1, Vec3 a2, Vec3 a3, Vec3 a4);
    void drawRoof();

    float lerp(float a, float b, float s);
    float lerpAngle(float a, float b, float s);
    int rotateDirection(float a, float b);
};

#endif // GRAPHICS_H

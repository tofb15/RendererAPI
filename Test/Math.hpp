#pragma once
#include <cmath>

typedef unsigned short     uint16_t;

typedef union Float3
{
	struct { float x; float y; float z; };
	struct { float r; float g; float b; };
	Float3() { x = y = z = 0; }
	Float3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

	Float3 operator+(Float3 other) const { return Float3(x + other.x, y + other.y, z + other.z); }
	Float3 operator-(Float3 other) const { return Float3(x - other.x, y - other.y, z - other.z); };
	Float3 operator*(float other) const { return Float3(x * other, y * other, z * other); };
	Float3 operator/(float other) const { float a = 1.0f / other; return Float3(x * a, y * a, z * a); };

	float length2() const { return x * x + y * y + z * z; }
	float length() const { return std::sqrtf(length2()); }
	Float3 normalized() const { return (*this / length()); }
	Float3 crossRH(Float3 other) const { return { y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x }; }
	Float3 crossLH(Float3 other) const { return crossRH(other) * -1; }
} Float3;

typedef union Float2
{
	Float2(float x_, float y_) : x(x_), y(y_) {}
	Float2() { x = y = 0.0f; }
	struct { float x; float y; };
	struct { float u; float v; };
	struct { float s; float t; };

	Float2 operator+(Float2 other) { return Float2(x + other.x, y + other.y); }
	Float2 operator-(Float2 other) { return Float2(x - other.x, y - other.y); };
	Float2 operator*(float other) { return Float2(x * other, y * other); };
	Float2 operator/(float other) { float a = 1.0f / other; return Float2(x * a, y * a); };
} Float2;


typedef union Int2
{
	Int2(int x_, int y_) : x(x_), y(y_) {}
	Int2() { x = y = 0; }

	struct { int x; int y;};
	struct { int u; int v;};
	struct { int s; int t;};

	Int2 operator+(Int2 other) { return Int2(x + other.x, y + other.y); }
	Int2 operator-(Int2 other) { return Int2(x - other.x, y - other.y); };
	Int2 operator*(float other) { return Int2(static_cast<int>(x * other), static_cast<int>(y * other)); };
	Int2 operator/(float other) { float a = 1.0f / other; return Int2(static_cast<int>(x * a), static_cast<int>(y * a)); };
} Int2;

struct Transform {
	Float3 pos;
	//Float3 rotation; //rotation should be represented as a quaternion insteed of Float3.
	Float3 scale;
	
};
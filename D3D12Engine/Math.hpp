#pragma once
#ifndef _MyMath
#define _MyMath

#include <cmath>
#include <iostream>

typedef unsigned short     USHORT;
typedef unsigned short     UINT16;
typedef unsigned short     uint16_t;

typedef unsigned int       UINT;
typedef unsigned __int32   UINT32;

struct UINT128 {
	unsigned __int64 least;
	unsigned __int64 most;
	UINT128() { least = most = 0; };
	UINT128(const unsigned __int32 i) { most = 0; least = i; };
	UINT128(const unsigned __int32 m, const unsigned __int32 l) { most = m; least = l; };
	UINT128(const unsigned __int64 i) { most = 0; least = i; };
	UINT128(const unsigned __int64 m, const unsigned __int64 l) { most = m; least = l; };
	UINT128(const UINT128& other) { most = other.most; least = other.least; };

	bool operator < (const UINT128& other) const { return (most < other.most) || ((most == other.most) && (least < other.least)); };
	bool operator <= (const UINT128& other) const { return (most < other.most) || ((most == other.most) && (least <= other.least)); };
	bool operator > (const UINT128& other) const { return (most > other.most) || ((most == other.most) && (least > other.least)); };
	bool operator >= (const UINT128& other) const { return (most > other.most) || ((most == other.most) && (least <= other.least)); };
	bool operator == (const UINT128& other) const { return (most == other.most) && (least == other.least); };
	UINT128& operator = (const UINT128& other) {
		if (this != &other) {
			most = other.most;
			least = other.least;
		}
		return *this;
	};
	UINT128& operator = (const unsigned short other) { most = 0; least = other; return *this; };
	UINT128& operator = (const unsigned int other) { most = 0; least = other; return *this; };
	UINT128& operator = (const unsigned long long other) { most = 0; least = other; return *this; };

	friend std::ostream& operator << (std::ostream& out, const UINT128& num) {
		return out << "(0x" << std::hex << num.most << ", 0x" << std::hex << num.least << ")" << std::dec;
	};
};

typedef union Float4 {
	struct { float x; float y; float z; float w; };
	struct { float r; float g; float b; float a; };

	Float4() { x = y = z = w = 0; }
	Float4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}

	inline Float4 operator+(const Float4& other) const { return Float4(x + other.x, y + other.y, z + other.z, w + other.w); }
	inline Float4 operator-(const Float4& other) const { return Float4(x - other.x, y - other.y, z - other.z, w - other.w); };
	inline Float4 operator*(float other) const { return Float4(x * other, y * other, z * other, w * other); };
	inline Float4 operator/(float other) const { float a = 1.0f / other; return Float4(x * a, y * a, z * a, w * a); };
	inline void operator/=(float other) { *this = *this / other; };

	inline float length2() const { return x * x + y * y + z * z + w * w; }
	inline float length() const { return std::sqrtf(length2()); }
	inline Float4 normalized() const { return (*this / length()); }
	inline void normalize() { *this = this->normalized(); }
	inline float dot(const Float4& other) const { return x * other.x + y * other.y + z * other.z + w * other.w; }
	//Float4 crossRH(Float4 other) const { return { y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x }; }
	//Float4 crossLH(Float4 other) const { return crossRH(other) * -1; }
} Float4;

typedef union Float3 {
	struct { float x; float y; float z; };
	struct { float r; float g; float b; };
	Float3() { x = y = z = 0; }
	Float3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

	inline Float3 operator+(const Float3& other) const { return Float3(x + other.x, y + other.y, z + other.z); }
	inline void operator+=(const Float3& other) { x += other.x; y += other.y; z += other.z; }
	inline Float3 operator-(const Float3& other) const { return Float3(x - other.x, y - other.y, z - other.z); };
	inline void operator-=(const Float3& other) { x -= other.x; y -= other.y; z -= other.z; }
	inline Float3 operator*(float other) const { return Float3(x * other, y * other, z * other); };
	inline void operator*=(float other) { x *= other; y *= other; z *= other; }
	inline void operator*=(const Float3& other) { x *= other.x; y *= other.y; z *= other.z; }
	inline Float3 operator/(float other) const { float a = 1.0f / other; return Float3(x * a, y * a, z * a); };
	inline void operator/=(float other) { *this = *this / other; };
	inline void operator=(const Float3& other) { x = other.x; y = other.y; z = other.z; };

	inline float length2() const { return x * x + y * y + z * z; }
	inline float length() const { return std::sqrtf(length2()); }
	inline Float3 normalized() const { return (*this / length()); }
	inline void normalize() { *this = this->normalized(); }
	inline float dot(const Float3& other) const { return x * other.x + y * other.y + z * other.z; }
	inline Float3 crossRH(const Float3& other) const { return { y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x }; }
	inline Float3 crossLH(const Float3& other) const { return crossRH(other) * -1; }
} Float3;

typedef union Float2 {
	Float2(float x_, float y_) : x(x_), y(y_) {}
	Float2() { x = y = 0.0f; }
	struct { float x; float y; };
	struct { float u; float v; };
	struct { float s; float t; };

	inline Float2 operator+(const Float2& other) { return Float2(x + other.x, y + other.y); }
	inline Float2 operator-(const Float2& other) { return Float2(x - other.x, y - other.y); };
	inline Float2 operator*(float other) { return Float2(x * other, y * other); };
	inline Float2 operator/(float other) { float a = 1.0f / other; return Float2(x * a, y * a); };
} Float2;


typedef union Int2 {
	Int2(int x_, int y_) : x(x_), y(y_) {}
	Int2() { x = y = 0; }

	struct { int x; int y; };
	struct { int u; int v; };
	struct { int s; int t; };

	inline Int2 operator+(const Int2& other) { return Int2(x + other.x, y + other.y); }
	inline Int2 operator-(const Int2& other) { return Int2(x - other.x, y - other.y); };
	inline Int2 operator*(float other) { return Int2(static_cast<int>(x * other), static_cast<int>(y * other)); };
	inline Int2 operator/(float other) { float a = 1.0f / other; return Int2(static_cast<int>(x * a), static_cast<int>(y * a)); };
	inline bool operator==(const Int2& other) { return (x == other.x && y == other.y); };
} Int2;

struct Transform {
	Transform() {
		scale = Float3(1.0f, 1.0f, 1.0f);
	}
	Float3 pos;
	Float3 rotation; //rotation should be represented as a quaternion insteed of Float3.
	Float3 scale;
};

struct MyRay {
public:
	MyRay() {};
	~MyRay() {};
	void SetDirection(const Float3& direction);
	void SetOrigin(const Float3& origin);
	Float3 GetDirection() const;
	Float3 GetOrigin() const;
	/*
		Project a point to the ray.
	*/
	Float3 ProjectPointToRay(const Float3& point) const;
private:
	Float3 m_origin;
	Float3 m_direction;
};
#endif // _MyMath
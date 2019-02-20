#pragma once

typedef unsigned short     uint16_t;

typedef union Float3
{
	struct { float x; float y; float z; };
	struct { float r; float g; float b; };
	Float3() { x = y = z = 0; }
	Float3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}

} Float3;

typedef union Float2
{
	Float2(float x_, float y_) : x(x_), y(y_) {}
	Float2() { x = y = 0.0f; }
	struct { float x; float y; };
	struct { float u; float v; };
	struct { float s; float t; };
} Float2;


typedef union Int2
{
	Int2(int x_, int y_) : x(x_), y(y_) {}
	Int2() { x = y = 0; }

	struct { int x; int y;};
	struct { int u; int v;};
	struct { int s; int t;};
} Int2;

struct Transform {
	Float3 pos;
	//Float3 rotation; //rotation should be represented as a quaternion insteed of Float3.
	Float3 scale;
	
};
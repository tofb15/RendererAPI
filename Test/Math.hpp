#pragma once

typedef unsigned short     uint16_t;

typedef union Float3
{
	struct { float x; float y; float z; };
	struct { float r; float g; float b; };

} Float3;

struct Int2
{
	union
	{
		int x, u, s;
	};
	union
	{
		int y, v, t;
	};
};

struct Transform {
	Float3 pos;
	//Float3 rotation; //rotation should be represented as a quaternion insteed of Float3.
	Float3 scale;
	
};
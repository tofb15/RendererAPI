#pragma once
struct Float3
{
	union
	{
		float x, r;
	};
	union
	{
		float y, g;
	};
	union
	{
		float z, b;
	};
};
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
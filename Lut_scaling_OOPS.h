#pragma once
#define _CRT_SECURE_NO_DEPRECATE
#include<iostream>
using namespace std;

#define FRACTIONAL_BITS 8
#define FLOAT2FIXED(x) ((int)((x)<<FRACTIONAL_BITS))

class LUT
{
public:
	//original dimensions
	int srcWidth, srcHeight, dstWidth, dstHeight;
	//downscaled parameters
	int downscaledWidth, downscaledHeight, remWidth, remHeight;
	float x_ratio, y_ratio;
	int divisor;

	struct Point
	{
		unsigned short x;                       //structure inside class
		unsigned short y;
	};
	struct Point* lut_downscaled, * lut_upscaled;
public:
	LUT(int srcwidth, int srcheight, int dstwidth, int dstheight, int divisor);                 //parameterized constructor
	~LUT();
	void lutGeneration();                                                         //lut genaration mem function
	void display_downscale();
	void lutUpscale(int Ax, int Cx, int Ay, int By, int Cy, int Dy);
};

//Sub class 1 : YUV444 
class ScalingYuv444 :public LUT
{
public:
	ScalingYuv444(int srcwidth, int srcheight, int dstwidth, int dstheight, int divisor);
	int scale(const char* srcFile, const char* dstFile);
};

//Sub class 2 : YUV422 
class ScalingYuv422 :public LUT
{
public:
	ScalingYuv422(int srcwidth, int srcheight, int dstwidth, int dstheight, int divisor);
	int scale(const char* srcFile, const char* dstFile);
};

//Sub class 3 : YUV420 
class ScalingYuv420 :public LUT
{
public:
	ScalingYuv420(int srcwidth, int srcheight, int dstwidth, int dstheight, int divisor);
	int scale(const char* srcFile, const char* dstFile);
};
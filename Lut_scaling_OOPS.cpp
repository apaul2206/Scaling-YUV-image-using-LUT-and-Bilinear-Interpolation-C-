#include"Lut_scaling_OOPS.h"
#include <fstream>
#include <sstream>
#include <string>

LUT::LUT(int srcwidth, int srcheight, int dstwidth, int dstheight, int div)                    //parameterized constructor
{
	cout << "!!!INSIDE BASE constructor!!!\n";
	srcWidth = srcwidth;
	srcHeight = srcheight;
	dstWidth = dstwidth;
	dstHeight = dstheight;
	divisor = div;
	downscaledWidth = (dstWidth / divisor) + 1;
	remWidth = dstWidth % divisor;
	downscaledHeight = (dstHeight / divisor) + 1;
	remHeight = dstHeight % divisor;
	if (remWidth)
		downscaledWidth++;
	if (remHeight)
		downscaledHeight++;
	cout << "\nrem :" << downscaledHeight << downscaledWidth << endl;
	x_ratio = (float)srcWidth / dstWidth;
	y_ratio = (float)srcHeight / dstHeight;

	lut_downscaled = new Point[downscaledWidth * downscaledHeight];
	lut_upscaled = new Point[(divisor + 1) * (divisor + 1)];
}

void LUT::lutGeneration()                                                          //lut genaration mem function
{
	cout << "!!!INSIDE LUT GEN!!!\n";
	//printf("x_ratio : %f  y_ratio : %f", x_ratio,y_ratio);
	for (int i = 0; i < downscaledHeight; i++)		//rows
	{
		for (int j = 0; j < downscaledWidth; j++)		//columns
		{
			lut_downscaled[i * downscaledWidth + j].x = (int)(y_ratio * i * divisor);
			lut_downscaled[i * downscaledWidth + j].y = (int)(x_ratio * j * divisor);	//row=0 col=0 to 10  xr=1.2
		}
	}
}

void LUT::display_downscale()
{
	cout << "Downscaled LUT :-\n";
	for (int i = 0; i < downscaledHeight; i++)
	{
		for (int j = 0; j < downscaledWidth; j++)
		{
			cout << "x[" << i << "] : " << lut_downscaled[i * downscaledWidth + j].x << " y[" << j << "] : " << lut_downscaled[i * downscaledWidth + j].y << " \t";
		}
		cout << "line : " << i << "\n";
	}
}

void LUT::lutUpscale(int Ax, int Cx, int Ay, int By, int Cy, int Dy)
{
	//index_out = i * upscaledWidth * 16 + j * 16;
	for (int row = 0; row <= divisor; row++)
	{
		int lineX = (Ax * (divisor - row) + Cx * (row)) / divisor;
		//y position calculation using bilinear 
		for (int col = 0; col <= divisor; col++)
		{
			lut_upscaled[row * (divisor + 1) + col].x = lineX;
			// Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + Dwh
			lut_upscaled[row * (divisor + 1) + col].y = (int)(
				Ay * (divisor - row) * (divisor - col) + By * (divisor - row) * (col)+
				Cy * (row) * (divisor - col) + Dy * (row * col)) / (divisor * divisor);
		}
	}
}

LUT::~LUT()
{
	delete[] lut_downscaled;
	delete[] lut_upscaled;
	cout << "\nDONE\n";
}

//sub class constructor
ScalingYuv420::ScalingYuv420(int srcwidth, int srcheight, int dstwidth, int dstheight, int divisor) :LUT(srcwidth, srcheight, dstwidth, dstheight, divisor)
{
	cout << "!!!INSIDE YUV420 constructor!!!\n";
}

//function to scale yuv420 image (calculating PP of Y & calculating UV PP from LUT)
int ScalingYuv420::scale(const char* srcFile, const char* dstFile)
{
	FILE* f1 = fopen(srcFile, "rb");
	unsigned char* srcBuffer = NULL;
	int frameSize = srcWidth * srcHeight;
	int srcSize = srcWidth * srcHeight * 3 / 2;
	srcBuffer = (unsigned char*)malloc(sizeof(unsigned char) * srcSize);
	if (srcBuffer == NULL)
	{
		cout << "malloc failed.\n";
		return -1;
	}
	memset(srcBuffer, '\0', srcSize);
	int ret = fread(srcBuffer, 1, srcSize, f1);

	unsigned char* dstBuffer = NULL;
	int dstSize = dstWidth * dstHeight * 3 / 2;
	dstBuffer = (unsigned char*)malloc(sizeof(unsigned char) * dstSize);
	if (dstBuffer == NULL)
	{
		cout << "malloc failed.\n";
		return -1;
	}
	memset(dstBuffer, '\0', dstSize);

	//bilinear interpolation
	int A, B, C, D, x, y = 0, index, gray;
	int x_ratio = (int)(FLOAT2FIXED(srcWidth) / dstWidth);
	int y_ratio = (int)(FLOAT2FIXED(srcHeight) / dstHeight);
	int x_diff, y_diff, one_min_y_diff, one_min_x_diff;
	int offset = 0;
	unsigned char* y_ptr = dstBuffer;
	unsigned char* u_ptr = y_ptr + dstWidth * dstHeight;
	unsigned char* v_ptr = u_ptr + (dstWidth * dstHeight) / 4;
	int w, h, lw, lh, wU, hU, lwU, lhU, wV, hV, lwV, lhV;
	int Ay, By, Cy, Dy, Ax, Cx;
	//bilinear interpolation
	h = 0;
	hU = 0;
	hV = 0;
	for (int i = 0; i < downscaledHeight - 1; i++) {
		w = 0; wU = 0; wV = 0;
		for (int j = 0; j < downscaledWidth - 1; j++) {
			index = i * downscaledWidth + j;
			Ay = lut_downscaled[index].y;
			By = lut_downscaled[index + 1].y;
			Cy = lut_downscaled[index + downscaledWidth].y;
			Dy = lut_downscaled[index + downscaledWidth + 1].y;
			Ax = lut_downscaled[index].x;
			Cx = lut_downscaled[index + downscaledWidth].x;
			lutUpscale(Ax, Cx, Ay, By, Cy, Dy);
			//Y
			int y_col = 0;
			lh = h;
			y = 0;
			for (int y_row = 0; y_row < divisor && h < dstHeight; y_row++) {
				y_diff = (y & 255);
				one_min_y_diff = 256 - y_diff;
				x = 0;
				lw = w;
				for (y_col = 0; y_col < divisor && w < dstWidth; y_col++) {
					x_diff = (x & 255);
					one_min_x_diff = 256 - x_diff;
					index = (lut_upscaled[y_row * (divisor + 1) + y_col].x * srcWidth) + (lut_upscaled[y_row * (divisor + 1) + y_col].y);
					// range is 0 to 255 thus bitwise AND with 0xff
					A = srcBuffer[index] & 0xff;
					B = srcBuffer[index + 1] & 0xff;
					C = srcBuffer[index + srcWidth] & 0xff;
					D = srcBuffer[index + srcWidth + 1] & 0xff;

					// Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + Dwh
					gray = (int)((
						A * one_min_x_diff * one_min_y_diff +
						B * x_diff * one_min_y_diff +
						C * y_diff * one_min_x_diff +
						D * x_diff * y_diff
						) >> 16);

					y_ptr[y_row * dstWidth + y_col] = gray;
					x += x_ratio;
					w++;
				}
				h++;
				w = lw;
				y += y_ratio;
			}
			h = lh;
			w += divisor;
			y_ptr += y_col;

			//U
			int u_col = 0;
			lhU = hU;
			y = 0;
			for (int u_row = 0; u_row < divisor && hU < dstHeight; u_row += 2) {
				y_diff = (y & 255);
				one_min_y_diff = 256 - y_diff;
				x = 0;
				lwU = wU;
				for (u_col = 0; u_col < divisor && wU < dstWidth; u_col += 2) {
					x_diff = (x & 255);
					one_min_x_diff = 256 - x_diff;
					index = ((lut_upscaled[u_row * (divisor + 1) + u_col].x >> 1) * (srcWidth >> 1)) + (lut_upscaled[u_row * (divisor + 1) + u_col].y >> 1);
					// range is 0 to 255 thus bitwise AND with 0xff
					A = srcBuffer[frameSize + index] & 0xff;
					B = srcBuffer[frameSize + index + 1] & 0xff;
					C = srcBuffer[frameSize + index + srcWidth / 2] & 0xff;
					D = srcBuffer[frameSize + index + srcWidth / 2 + 1] & 0xff;
					// Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + Dwh
					gray = (int)((
						A * one_min_x_diff * one_min_y_diff +
						B * x_diff * one_min_y_diff +
						C * y_diff * one_min_x_diff +
						D * x_diff * y_diff
						) >> 16);

					u_ptr[(u_row / 2) * (dstWidth / 2) + (u_col / 2)] = gray;
					x += x_ratio;
					wU += 2;
				}
				hU += 2;
				wU = lwU;
				y += y_ratio;
			}
			hU = lhU;
			wU += divisor;
			u_ptr += (u_col >> 1);

			//V
			int v_col = 0;
			lhV = hV;
			y = 0;
			for (int v_row = 0; v_row < divisor && hV < dstHeight; v_row += 2) {
				y_diff = (y & 255);
				one_min_y_diff = 256 - y_diff;
				x = 0;
				lwV = wV;
				for (v_col = 0; v_col < divisor && wV < dstWidth; v_col += 2) {
					x_diff = (x & 255);
					one_min_x_diff = 256 - x_diff;
					index = ((lut_upscaled[v_row * (divisor + 1) + v_col].x >> 1) * (srcWidth >> 1)) + (lut_upscaled[v_row * (divisor + 1) + v_col].y >> 1);
					// range is 0 to 255 thus bitwise AND with 0xff
					A = srcBuffer[frameSize + frameSize / 4 + index] & 0xff;
					B = srcBuffer[frameSize + frameSize / 4 + index + 1] & 0xff;
					C = srcBuffer[frameSize + frameSize / 4 + index + srcWidth / 2] & 0xff;
					D = srcBuffer[frameSize + frameSize / 4 + index + srcWidth / 2 + 1] & 0xff;
					// Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + Dwh
					gray = (int)((
						A * one_min_x_diff * one_min_y_diff +
						B * x_diff * one_min_y_diff +
						C * y_diff * one_min_x_diff +
						D * x_diff * y_diff
						) >> 16);

					v_ptr[(v_row / 2) * (dstWidth / 2) + (v_col / 2)] = gray;
					x += x_ratio;
					wV += 2;
				}
				hV += 2;
				wV = lwV;
				y += y_ratio;
			}
			hV = lhV;
			wV += divisor;
			v_ptr += (v_col >> 1);
		}
		h += divisor; hU += divisor; hV += divisor;
		y_ptr += dstWidth * (divisor - 1);
		u_ptr += (dstWidth >> 1) * ((divisor >> 1) - 1);
		v_ptr += (dstWidth >> 1) * ((divisor >> 1) - 1);
	}

	FILE* f2 = fopen(dstFile, "wb");
	ret = (int)fwrite(dstBuffer, 1, dstSize, f2);

	fclose(f1);
	fclose(f2);
	free(srcBuffer);
	free(dstBuffer);
	return 0;
}

//sub class constructor
ScalingYuv444::ScalingYuv444(int srcwidth, int srcheight, int dstwidth, int dstheight, int divisor) :LUT(srcwidth, srcheight, dstwidth, dstheight, divisor)
{
	cout << "!!!INSIDE YUV444 constructor!!!\n";
}

int ScalingYuv444::scale(const char* srcFile, const char* dstFile)
{
	//scaling yuv444
	FILE* f1 = fopen(srcFile, "rb");
	unsigned char* srcBuffer = NULL;
	int frameSize = srcWidth * srcHeight;
	int srcSize = srcWidth * srcHeight * 3;
	srcBuffer = (unsigned char*)malloc(sizeof(unsigned char) * srcSize);
	if (srcBuffer == NULL)
	{
		cout << "malloc failed.\n";
		return -1;
	}
	memset(srcBuffer, '\0', srcSize);
	int ret = fread(srcBuffer, 1, srcSize, f1);

	unsigned char* dstBuffer = NULL;
	int dstSize = dstWidth * dstHeight * 3;
	dstBuffer = (unsigned char*)malloc(sizeof(unsigned char) * dstSize);
	if (dstBuffer == NULL)
	{
		cout << "malloc failed.\n";
		return -1;
	}
	memset(dstBuffer, '\0', dstSize);

	//bilinear interpolation
	int A, B, C, D, x, y = 0, index, gray;
	int x_ratio = (int)(FLOAT2FIXED(srcWidth) / dstWidth);
	int y_ratio = (int)(FLOAT2FIXED(srcHeight) / dstHeight);
	int x_diff, y_diff, one_min_y_diff, one_min_x_diff;
	int offset = 0;
	unsigned char* y_ptr = dstBuffer;
	unsigned char* u_ptr = y_ptr + dstWidth * dstHeight;
	unsigned char* v_ptr = u_ptr + dstWidth * dstHeight;
	int w, h, lw, lh;
	int Ay, By, Cy, Dy, Ax, Cx;
	//bilinear interpolation
	h = 0;
	for (int i = 0; i < downscaledHeight - 1; i++) {
		w = 0;
		for (int j = 0; j < downscaledWidth - 1; j++) {
			index = i * downscaledWidth + j;
			Ay = lut_downscaled[index].y;
			By = lut_downscaled[index + 1].y;
			Cy = lut_downscaled[index + downscaledWidth].y;
			Dy = lut_downscaled[index + downscaledWidth + 1].y;
			Ax = lut_downscaled[index].x;
			Cx = lut_downscaled[index + downscaledWidth].x;
			lutUpscale(Ax, Cx, Ay, By, Cy, Dy);
			//Y
			int y_col = 0;
			y = 0;
			lh = h;
			for (int y_row = 0; y_row < divisor && h < dstHeight; y_row++) {
				y_diff = (y & 255);
				one_min_y_diff = 256 - y_diff;
				x = 0;
				lw = w;

				for (y_col = 0; y_col < divisor && w < dstWidth; y_col++) {
					x_diff = (x & 255);
					one_min_x_diff = 256 - x_diff;
					index = (lut_upscaled[y_row * (divisor + 1) + y_col].x * srcWidth) + (lut_upscaled[y_row * (divisor + 1) + y_col].y);
					// range is 0 to 255 thus bitwise AND with 0xff
					//Y
					A = srcBuffer[index] & 0xff;
					B = srcBuffer[index + 1] & 0xff;
					C = srcBuffer[index + srcWidth] & 0xff;
					D = srcBuffer[index + srcWidth + 1] & 0xff;

					// Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + Dwh
					gray = (int)((
						A * one_min_x_diff * one_min_y_diff +
						B * x_diff * one_min_y_diff +
						C * y_diff * one_min_x_diff +
						D * x_diff * y_diff
						) >> 16);
					y_ptr[y_row * dstWidth + y_col] = gray;

					//U
					A = srcBuffer[frameSize + index] & 0xff;
					B = srcBuffer[frameSize + index + 1] & 0xff;
					C = srcBuffer[frameSize + index + srcWidth] & 0xff;
					D = srcBuffer[frameSize + index + srcWidth + 1] & 0xff;

					// Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + Dwh
					gray = (int)((
						A * one_min_x_diff * one_min_y_diff +
						B * x_diff * one_min_y_diff +
						C * y_diff * one_min_x_diff +
						D * x_diff * y_diff
						) >> 16);
					u_ptr[y_row * dstWidth + y_col] = gray;

					//V
					A = srcBuffer[2 * frameSize + index] & 0xff;
					B = srcBuffer[2 * frameSize + index + 1] & 0xff;
					C = srcBuffer[2 * frameSize + index + srcWidth] & 0xff;
					D = srcBuffer[2 * frameSize + index + srcWidth + 1] & 0xff;

					// Y = A(1-w)(1-h) + B(w)(1-h) + C(h)(1-w) + Dwh
					gray = (int)((
						A * one_min_x_diff * one_min_y_diff +
						B * x_diff * one_min_y_diff +
						C * y_diff * one_min_x_diff +
						D * x_diff * y_diff
						) >> 16);
					v_ptr[y_row * dstWidth + y_col] = gray;

					x += x_ratio;
					w++;
				}
				h++;
				w = lw;
				y += y_ratio;
			}
			h = lh;
			w += divisor;
			y_ptr += y_col;
			u_ptr += y_col;
			v_ptr += y_col;
		}
		y_ptr += dstWidth * (divisor - 1);
		u_ptr += dstWidth * (divisor - 1);
		v_ptr += dstWidth * (divisor - 1);
		h += divisor;
	}

	FILE* f2 = fopen(dstFile, "wb");
	ret = (int)fwrite(dstBuffer, 1, dstSize, f2);

	fclose(f1);
	fclose(f2);
	free(srcBuffer);
	free(dstBuffer);
	return 0;
}

//sub class constructor
ScalingYuv422::ScalingYuv422(int srcwidth, int srcheight, int dstwidth, int dstheight, int divisor) :LUT(srcwidth, srcheight, dstwidth, dstheight, divisor)
{
	cout << "!!!INSIDE YUV422 constructor!!!\n";
}

int ScalingYuv422::scale(const char* srcFile, const char* dstFile)
{
	//scaling yuv422
	return 0;
}

int main(int argc, char* argv[])
{
	clock_t t;
	double timetaken;
	//int c_srcwidth;
	//int c_srcheight;
	//int c_dstwidth;
	//int c_dstheight;
	int c_divisor;

	//CONFIG read
	ifstream fin("config.txt");
	string line;
	while (getline(fin, line)) {
		istringstream sin(line.substr(line.find("=") + 1));
		if (line.find("c_divisor") != -1)
			sin >> c_divisor;
		/*else if (line.find("c_srcwidth") != -1)
			sin >> c_srcwidth;
		else if (line.find("c_srcheight") != -1)
			sin >> c_srcheight;
		else if (line.find("c_dstwidth") != -1)
			sin >> c_dstwidth;
		else if (line.find("c_dstheight") != -1)
			sin >> c_dstheight;*/
	}
	//declaring object of class and passing values for  constructor parameters
	ScalingYuv420 lut1(1632, 1280, 938, 736, c_divisor);			//declaring object of class and passing values for  constructor parameters
	//ScalingYuv420 lut1(atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6]));
	lut1.lutGeneration();
	t = clock();
	lut1.scale("yuv420_1632x1280_img.yuv", "yuv420_938x736_test.yuv");
	//lut1.scale(argv[1], argv[2]);

	/*ScalingYuv444 lut1(c_srcwidth, c_srcheight, c_dstwidth, c_dstheight);
	lut1.lutGeneration();
	t = clock();*/
	//lut1.scale("yuv420_1632x1280_img.yuv", "yuv420_960x736_test.yuv");
	//lut1.scale("yuv444_1632x1280_img.yuv", "yuv444_1920x1080_test.yuv");
	t = clock() - t;
	timetaken = ((double)t) / CLOCKS_PER_SEC;
	cout << "Time taken to generate LUT : " << timetaken << endl;

	return 0;
}
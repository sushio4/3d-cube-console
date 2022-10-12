#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#define X_AXIS 0x1
#define Y_AXIS 0x2
#define Z_AXIS 0x4

int width, height;
float oneOverWidth, oneOverHeight;
float fontRatio;
float eyeDistance = 1;

const int triangleCount = 12;
const float cube[12][3][3] =
    {
        { {-1,1,-1}, {1,-1,-1}, {1,1,-1} },
        { {-1,1,-1}, {-1,-1,-1}, {1,-1,-1} },
        { {-1,1,1}, {1,1,1}, {1,-1,1} },
        { {-1,1,1}, {1,-1,1}, {-1,-1,1} },
        { {-1,1,-1}, {1,1,-1}, {1,1,1} },
        { {-1,1,-1}, {1,1,1}, {-1,1,1} },
        { {-1,-1,-1}, {1,-1,-1}, {1,-1,1} },
        { {-1,-1,-1}, {-1,-1,1}, {1,-1,1} },
        { {-1,1,-1}, {-1,-1,1}, {-1,-1,-1} },
        { {-1,1,-1}, {-1,1,1}, {-1,-1,1} },
        { {1,1,-1}, {1,-1,-1}, {1,-1,1} },
        { {1,1,-1}, {1,-1,1}, {1,1,1} }
    };


int IsInTriangle(const float point[2],const float A[3],const float B[3],const float C[3])
{
    float ab[2] = {B[0]-A[0],B[1]-A[1]};
    float bc[2] = {C[0]-B[0],C[1]-B[1]};
    float ca[2] = {A[0]-C[0],A[1]-C[1]};
    float ap[2] = {point[0]-A[0],point[1]-A[1]};
    float bp[2] = {point[0]-B[0],point[1]-B[1]};
    float cp[2] = {point[0]-C[0],point[1]-C[1]};
    return  (ca[0]*cp[1] < ca[1]*cp[0]&&
            bc[0]*bp[1] < bc[1]*bp[0]&&
            ab[0]*ap[1] < ab[1]*ap[0])||
            (ca[0]*cp[1] > ca[1]*cp[0]&&
            bc[0]*bp[1] > bc[1]*bp[0]&&
            ab[0]*ap[1] > ab[1]*ap[0]);
}

void ToScreenCoords(float point[2])
{
    point[1] = -point[1] * 2 * oneOverHeight + 1;
    point[0] = (point[0] * 2 * oneOverHeight - (width-1) * oneOverHeight) * fontRatio;
}

float* Projection(const float point[3]) //projection onto screen
{
    float* pp = (float*)malloc(2*sizeof(float));
    float coefficient = eyeDistance / (eyeDistance + point[2]);
    pp[0] = point[0] * coefficient;
    pp[1] = point[1] * coefficient;
    return pp;
}

float* CalculatePlaneEquation(const float a[3],const float b[3],const float c[3]) //compute screen equation coefficients
{
    float *pl = (float*)malloc(4*sizeof(float));
    pl[0] = (b[1]-a[1])*(c[2]-a[2])-(b[2]-a[2])*(c[1]-a[1]);
    pl[1] = (b[2]-a[2])*(c[0]-a[0])-(b[0]-a[0])*(c[2]-a[2]);
    pl[2] = (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0]);
    pl[3] = - pl[0]*a[0] - pl[1]*a[1] - pl[2]*a[2];
    return pl; //Ax + By + Cz + D = 0
}

float* Rotate(const float* vertex, short around, float angle)
{
	float fsin, fcos;
	float* result = (float*)malloc(3 * sizeof(float));
	if (around) 
    {
        fsin = sin(angle);
        fcos = cos(angle);
    }
	if (around & X_AXIS)
	{
        result[0] = vertex[0]; 
        result[1] = vertex[1] * fcos - vertex[2] * fsin;
        result[2] = vertex[2] * fcos + vertex[1] * fsin;
    }
	if (around & Y_AXIS)
    {
        result[0] = vertex[0] * fcos + vertex[2] * fsin;
        result[1] = vertex[1];
        result[2] = vertex[2] * fcos - vertex[0] * fsin;
    }
	if (around & Z_AXIS) 
    {
        result[0] = vertex[0] * fcos - vertex[1] * fsin;
        result[1] = vertex[1] * fcos + vertex[0] * fsin;
        result[2] = vertex[2];
    }
	return result;
}
void AddVector(float* a,const float* b) 
{
    a[0]+=b[0],a[1]+=b[1],a[2]+=b[2];
}

struct Triangle
{
    float* a;
    float* b;
    float* c;
};

int main()
{
    //handle to console
    void* handle = GetStdHandle((DWORD)-11);
    
    //set cursor to invisible (doesn't work for some reason..)
    CONSOLE_CURSOR_INFO cursorInfo;
    cursorInfo.bVisible = FALSE;
    SetConsoleCursorInfo(handle, &cursorInfo);
    
    //get width and height
    CONSOLE_SCREEN_BUFFER_INFO screenInfo;
    GetConsoleScreenBufferInfo(handle, &screenInfo);
    
    width = screenInfo.srWindow.Right + 1;
    height = screenInfo.srWindow.Bottom - 1;
    
    oneOverWidth = 1 / (float)width;
    oneOverHeight = 1 / (float)height;
    
    //font size ratio
    CONSOLE_FONT_INFO fontInfo;
    GetCurrentConsoleFont(handle , 0, &fontInfo);
    fontRatio = (float)fontInfo.dwFontSize.X / (float)fontInfo.dwFontSize.Y;
    
    char* frameBuffer =  (char*) malloc(width * height + 1);
    frameBuffer[width * height] = '\0';
    
    float* depthBuffer = (float*)malloc(width * height * sizeof(float));

    float pos = 2, dtheta = 0.0001, theta = 0;

    while (1)
    {
        //reset depth and fragment buffer
        for(int i = 0; i < width * height; i++) 
            depthBuffer[i] = 30, frameBuffer[i] = ' ';

        for(int i = 0; i < triangleCount; i++)
        {
            float* vertices[3];
            for(int j = 0; j < 3; j++)
            {
                vertices[j] = Rotate(cube[i][j], 2, theta);
                vertices[j] = Rotate(vertices[j], 1, -0.6);
                vertices[j][2] += pos;
            }
            //calculate plane equation for that triangle
            float* triangleEquation = CalculatePlaneEquation(vertices[0], vertices[1], vertices[2]);
            //triangle vertexes projected onto a 2d screen
            float* projecteda = Projection(vertices[0]);
            float* projectedb = Projection(vertices[1]);
            float* projectedc = Projection(vertices[2]);

            //for each pixel
            for(int x = 0; x < width; x++)
                for(int y = 0; y < height; y++)
                {
                    float point[2] = {x,y};
                    ToScreenCoords(point); //translate p to screen coords

                    float z = eyeDistance * (-triangleEquation[0] * point[0] - triangleEquation[1] * point[1] - triangleEquation[3])
                        / (triangleEquation[0] * point[0] + triangleEquation[1] * point[1] + triangleEquation[2] * eyeDistance);
                    //depth test
                    if(z <= depthBuffer[x + width * y] && z >= 0 && IsInTriangle(point, projecteda, projectedb, projectedc))
                    {
                        frameBuffer[x + width * y] = ".,-~:;=!*#$@"[(int)(7 - triangleEquation[0])]; //depth gradient
                        depthBuffer[x + width * y] = z;
                    }
                }
            //cleanup
            free(projecteda);
            free(projectedb);
            free(projectedc);
            free(triangleEquation);

            theta += dtheta;
        }

        //set cursor pos to 0,0 and display everything
        SetConsoleCursorPosition(handle, (COORD){0,0});
        puts(frameBuffer);
        
        usleep(500);
    }
    return 0;
}

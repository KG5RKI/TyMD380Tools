
#include <stdint.h>
#include "lcd_driver.h"

float xx, xy, xz;
float yx, yy, yz;
float zx, zy, zz;

float fact;

int Xan, Yan;

int Xoff;
int Yoff;
int Zoff;

typedef struct _Point3d
{
	int x;
	int y;
	int z;
} Point3d;

typedef struct _Point2d
{
	int x;
	int y;
	int z;
} Point2d;

int LinestoRender; // lines to render.
int OldLinestoRender; // lines to render just in case it changes. this makes sure the old lines all get erased.



typedef struct _Line3d
{
	Point3d p0;
	Point3d p1;
}Line3d;



typedef struct _Line2d
{
	Point2d p0;
	Point2d p1;
}Line2d;


Line3d Lines[20];
Line2d Render[20];
Line2d ORender[20];


void SetVars(void)
{
	float Xan2, Yan2, Zan2;
	float s1, s2, s3, c1, c2, c3;

	Xan2 = Xan / fact; // convert degrees to radians.
	Yan2 = Yan / fact;

	// Zan is assumed to be zero

	s1 = sin(Yan2);
	s2 = sin(Xan2);

	c1 = cos(Yan2);
	c2 = cos(Xan2);

	xx = c1;
	xy = 0;
	xz = -s1;

	yx = (s1 * s2);
	yy = c2;
	yz = (c1 * s2);

	zx = (s1 * c2);
	zy = -s2;
	zz = (c1 * c2);
}

void ProcessLine(Line2d *ret, Line3d vec)
{
	float zvt1;
	int xv1, yv1, zv1;

	float zvt2;
	int xv2, yv2, zv2;

	int rx1, ry1;
	int rx2, ry2;

	int x1;
	int y1;
	int z1;

	int x2;
	int y2;
	int z2;

	int Ok;

	x1 = vec.p0.x;
	y1 = vec.p0.y;
	z1 = vec.p0.z;

	x2 = vec.p1.x;
	y2 = vec.p1.y;
	z2 = vec.p1.z;

	Ok = 1; // defaults to not OK

	xv1 = (x1 * xx) + (y1 * xy) + (z1 * xz);
	yv1 = (x1 * yx) + (y1 * yy) + (z1 * yz);
	zv1 = (x1 * zx) + (y1 * zy) + (z1 * zz);

	zvt1 = zv1 - Zoff;


	if (zvt1 < -5) {
		rx1 = 256 * (xv1 / zvt1) + Xoff;
		ry1 = 256 * (yv1 / zvt1) + Yoff;
		Ok = 1; // ok we are alright for point 1.
	}


	xv2 = (x2 * xx) + (y2 * xy) + (z2 * xz);
	yv2 = (x2 * yx) + (y2 * yy) + (z2 * yz);
	zv2 = (x2 * zx) + (y2 * zy) + (z2 * zz);

	zvt2 = zv2 - Zoff;


	if (zvt2 < -5) {
		rx2 = 256 * (xv2 / zvt2) + Xoff;
		ry2 = 256 * (yv2 / zvt2) + Yoff;
	}
	else
	{
		Ok = 0;
	}


	if (Ok == 1) 
	{

		ret->p0.x = rx1;
		ret->p0.y = ry1;

		ret->p1.x = rx2;
		ret->p1.y = ry2;
	}
	// The ifs here are checks for out of bounds. needs a bit more code here to "safe" lines that will be way out of whack, so they dont get drawn and cause screen garbage.

}


void setupBootAnim() {



	
	fact = 180.0 / 3.14159259; // conversion from degrees to radians.

	Xoff = 63; // positions the center of the 3d conversion space into the center of the OLED screen. This is usally screen_x_size / 2.
	Yoff = 63; // screen_y_size /2
	Zoff = 500;


	// line segments to draw a cube. basically p0 to p1. p1 to p2. p2 to p3 so on.

	// Front Face.

	Lines[0].p0.x = -50;
	Lines[0].p0.y = -50;
	Lines[0].p0.z = 50;
	Lines[0].p1.x = 50;
	Lines[0].p1.y = -50;
	Lines[0].p1.z = 50;



	Lines[1].p0.x = 50;
	Lines[1].p0.y = -50;
	Lines[1].p0.z = 50;
	Lines[1].p1.x = 50;
	Lines[1].p1.y = 50;
	Lines[1].p1.z = 50;




	Lines[2].p0.x = 50;
	Lines[2].p0.y = 50;
	Lines[2].p0.z = 50;
	Lines[2].p1.x = -50;
	Lines[2].p1.y = 50;
	Lines[2].p1.z = 50;



	Lines[3].p0.x = -50;
	Lines[3].p0.y = 50;
	Lines[3].p0.z = 50;
	Lines[3].p1.x = -50;
	Lines[3].p1.y = -50;
	Lines[3].p1.z = 50;






	//back face.

	Lines[4].p0.x = -50;
	Lines[4].p0.y = -50;
	Lines[4].p0.z = -50;
	Lines[4].p1.x = 50;
	Lines[4].p1.y = -50;
	Lines[4].p1.z = -50;



	Lines[5].p0.x = 50;
	Lines[5].p0.y = -50;
	Lines[5].p0.z = -50;
	Lines[5].p1.x = 50;
	Lines[5].p1.y = 50;
	Lines[5].p1.z = -50;




	Lines[6].p0.x = 50;
	Lines[6].p0.y = 50;
	Lines[6].p0.z = -50;
	Lines[6].p1.x = -50;
	Lines[6].p1.y = 50;
	Lines[6].p1.z = -50;



	Lines[7].p0.x = -50;
	Lines[7].p0.y = 50;
	Lines[7].p0.z = -50;
	Lines[7].p1.x = -50;
	Lines[7].p1.y = -50;
	Lines[7].p1.z = -50;




	// now the 4 edge lines.

	Lines[8].p0.x = -50;
	Lines[8].p0.y = -50;
	Lines[8].p0.z = 50;
	Lines[8].p1.x = -50;
	Lines[8].p1.y = -50;
	Lines[8].p1.z = -50;


	Lines[9].p0.x = 50;
	Lines[9].p0.y = -50;
	Lines[9].p0.z = 50;
	Lines[9].p1.x = 50;
	Lines[9].p1.y = -50;
	Lines[9].p1.z = -50;


	Lines[10].p0.x = -50;
	Lines[10].p0.y = 50;
	Lines[10].p0.z = 50;
	Lines[10].p1.x = -50;
	Lines[10].p1.y = 50;
	Lines[10].p1.z = -50;


	Lines[11].p0.x = 50;
	Lines[11].p0.y = 50;
	Lines[11].p0.z = 50;
	Lines[11].p1.x = 50;
	Lines[11].p1.y = 50;
	Lines[11].p1.z = -50;



	LinestoRender = 12;
	OldLinestoRender = LinestoRender;

	LCD_FillRect(0, 0, LCD_SCREEN_WIDTH - 1, LCD_SCREEN_HEIGHT - 1, 0x0);
}


void renderBootAnim() {

	Xan++;
	Yan++;


	Yan = Yan % 360;
	Xan = Xan % 360; // prevents overflow.



	SetVars(); //sets up the global vars to do the conversion.

	for (int i = 0; i<LinestoRender; i++)
	{
		ORender[i] = Render[i]; // stores the old line segment so we can delete it later.
		ProcessLine(&Render[i], Lines[i]); // converts the 3d line segments to 2d.
	}

	for (int i = 0; i<OldLinestoRender; i++)
	{
		LCD_DrawLine(ORender[i].p0.x+1, ORender[i].p0.y+1, ORender[i].p1.x+2, ORender[i].p1.y+1, 0); // erase the old lines.
		LCD_DrawLine(ORender[i].p0.x, ORender[i].p0.y, ORender[i].p1.x, ORender[i].p1.y, 0); // erase the old lines.
		LCD_DrawLine(ORender[i].p0.x + 2, ORender[i].p0.y + 2, ORender[i].p1.x + 3, ORender[i].p1.y + 3, 0); // erase the old lines.
	}

	for (int i = 0; i<LinestoRender; i++)
	{
		LCD_DrawLine(Render[i].p0.x + 1, Render[i].p0.y + 1, Render[i].p1.x + 2, Render[i].p1.y + 1, 0x55EF); // erase the old lines.
		LCD_DrawLine(Render[i].p0.x, Render[i].p0.y, Render[i].p1.x, Render[i].p1.y, 0x55EF); // erase the old lines.
		LCD_DrawLine(Render[i].p0.x + 2, Render[i].p0.y + 2, Render[i].p1.x + 3, Render[i].p1.y + 3, 0x55EF); // erase the old lines.
		
	}
	OldLinestoRender = LinestoRender;
}
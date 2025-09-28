


#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include "vfont.h"
#include "primitives.h"





extern uint8_t renderBuffer[VWIDTH*VHEIGHT];



static inline int getPixel1 (const uint8_t *pixels, const int pitch, const int x, const int y)
{
	return *(pixels+((y * pitch)+(x>>3))) >>(x&7)&0x01;
}

void drawBitmap (image_t *img, int x, int y, const uint16_t colour)
{
	const int srcPitch = CALC_PITCH_1(img->width);
		
	// center image
	x -= img->width>>1;
	y -= img->height>>1;
		
	for (int y1 = 0; y1 < img->height; y1++, y++){
		int _x = x;
		for (int x1 = 0; x1 < img->width; x1++, _x++){
			if (getPixel1(img->pixels, srcPitch, x1, y1))
				drawPixel(_x, y, colour);
		}
	}
}

static inline void swap32 (int32_t *a, int32_t *b) 
{ 
#if 0
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
#else
	int32_t tmp = *a;
	*a = *b;
	*b = tmp;
#endif
}

static inline void swap32i (int *a, int *b) 
{ 
#if 0
	*a ^= *b;
	*b ^= *a;
	*a ^= *b;
#else
	int tmp = *a;
	*a = *b;
	*b = tmp;
#endif
}

static inline void drawHLine (const int y, int x1, int x2, const uint8_t colour)
{
	if (y < 0 || y >= VHEIGHT) return;
	if (x2 < x1) swap32i(&x1, &x2);
	if (x1 >= VWIDTH) return;
	if (x1 < 0) x1 = 0;
	if (x2 >= VWIDTH) x2 = VWIDTH - 1;

#if 0
	for (int x = x1; x <= x2; x++)
		drawPixel(x, y, colour);
#else
	uint8_t *pixel = &renderBuffer[(y*VWIDTH)+x1];
	for (int x = x1; x <= x2; x++){
		*pixel++ = colour;
	}
#endif
}

static inline int findRegion (const int x, const int y)
{
	int code = 0;
	
	if (y >= VHEIGHT)
		code = 1;		// top
	else if (y < 0)
		code |= 2;		// bottom

	if (x >= VWIDTH)
		code |= 4;		// right
	else if ( x < 0)
		code |= 8;		// left

	return code;
}


// clip using Cohen-Sutherland algorithm
static inline int clipLine (int x1, int y1, int x2, int y2, int *x3, int *y3, int *x4, int *y4)
{
  
	int accept = 0, done = 0;
	int code1 = findRegion(x1, y1); //the region outcodes for the endpoints
	int code2 = findRegion(x2, y2);
  
	const int h = VHEIGHT;
	const int w = VWIDTH;
  
	do{
		if (!(code1 | code2)){
			accept = done = 1;  //accept because both endpoints are in screen or on the border, trivial accept
    	
		}else if (code1&code2){
			done = 1; //the line isn't visible on screen, trivial reject
    	
		}else{  //if no trivial reject or accept, continue the loop
			int x, y;
			int codeout = code1 ? code1 : code2;
			if (codeout&1){			//top
				x = x1 + (x2 - x1) * (h - y1) / (y2 - y1);
				y = h - 1;
			}else if (codeout&2){	//bottom
				x = x1 + (x2 - x1) * -y1 / (y2 - y1);
				y = 0;
			}else if (codeout&4){	//right
				y = y1 + (y2 - y1) * (w - x1) / (x2 - x1);
				x = w - 1;
			}else{					//left
				y = y1 + (y2 - y1) * -x1 / (x2 - x1);
				x = 0;
			}
      
			if (codeout == code1){ //first endpoint was clipped
				x1 = x; y1 = y;
				code1 = findRegion(x1, y1);
			}else{ //second endpoint was clipped
				x2 = x; y2 = y;
        		code2 = findRegion(x2, y2);
			}
		}
	}while(!done);

	if (accept){
		*x3 = x1;
		*x4 = x2;
		*y3 = y1;
		*y4 = y2;
		return 1;
	}else{
		return 0;
	}
}


#ifdef LINE_FAST
static inline void drawLineFast (int x, int y, int x2, int y2, const int colour)
{
	if (!clipLine(x, y, x2, y2, &x, &y, &x2, &y2))
		return;

   	int yLonger = 0;
	int shortLen = y2 - y;
	int longLen = x2 - x;
	
	if (abs(shortLen) > abs(longLen)){
		swap32(&shortLen, &longLen);
		yLonger = 1;
	}

	int decInc;
	if (!longLen)
		decInc = 0;
	else
		decInc = (shortLen<<16) / longLen;


	if (yLonger) {
		if (longLen > 0) {
			longLen += y;
			
			for (int j = 0x8000+(x<<16); y <= longLen; ++y){
				drawPixel(j>>16, y, colour);
				j+=decInc;
			}
			return;
		}
		longLen += y;
		
		for (int j = 0x8000+(x<<16); y >= longLen; --y){
			drawPixel(j>>16, y, colour);
			j-=decInc;
		}
		return;
	}

	if (longLen > 0) {
		longLen += x;
		
		for (int j = 0x8000+(y<<16); x <= longLen; ++x){
			drawPixel(x, j>>16, colour);
			j+=decInc;
		}
		return;
	}
	
	longLen += x;
	
	for (int j = 0x8000+(y<<16); x >= longLen; --x){
		drawPixel(x, j>>16, colour);
		j-=decInc;
	}

}
#endif

#ifdef LINE_STD
static inline void drawLineStd (int x1, int y1, int x2, int y2, const uint16_t colour)
{
	if (!clipLine(x1, y1, x2, y2, &x1, &y1, &x2, &y2))
		return;

	
    const int dx = x2 - x1;
    const int dy = y2 - y1;

    if (dx || dy){
        if (abs(dx) >= abs(dy)){
            float y = y1 + 0.5f;
            float dly = dy / (float)dx;
            
            if (dx > 0){
                for (int xx = x1; xx<=x2; xx++){
                    drawPixel(xx, (int)y, colour);
                    y += dly;
                }
            }else{
                for (int xx = x1; xx>=x2; xx--){
                    drawPixel(xx, (int)y, colour);
                    y -= dly;
                }
			}
        }else{
           	float x = x1 + 0.5f;
           	float dlx = dx/(float)dy;

            if (dy > 0){
   	            for (int yy = y1; yy<=y2; yy++){
       	            drawPixel((int)x, yy, colour);
           	        x += dlx;
               	}
			}else{
                for (int yy = y1; yy >= y2; yy--){
   	                drawPixel((int)x, yy, colour);
       	            x -= dlx;
           	    }
			}
        }
    }else if (!(dx&dy)){
    	drawPixel(x1, y1, colour);
    }

}
#endif

#ifdef LINE_FASTEST8
static inline void drawLine8 (int x0, int y0, int x1, int y1, const uint8_t colour)
{
	if (!clipLine(x0, y0, x1, y1, &x0, &y0, &x1, &y1))
		return;
		
	int stepx, stepy;
	
	int dy = y1 - y0;
	if (dy < 0){
		dy = -dy;
		stepy = -VWIDTH;
	}else{
		stepy = VWIDTH;
	}
	dy <<= 1;
		
	int dx = x1 - x0;
	if (dx < 0){
		dx = -dx;
		stepx = -1;
	}else{
		stepx = 1;
	}
	dx <<= 1;
	
	y0 *= VWIDTH;
	y1 *= VWIDTH;

	uint8_t *pixels = (uint8_t*)renderBuffer;
	pixels[x0+y0] = colour;
	
	if (dx > dy){
	    int fraction = dy - (dx >> 1);
	    
	    while (x0 != x1){
	        if (fraction >= 0){
	            y0 += stepy;
	            fraction -= dx;
	        }
	        x0 += stepx;
	        fraction += dy;
	        pixels[x0+y0] = colour;
	    }
	}else{
	    int fraction = dx - (dy >> 1);
	    
	    while (y0 != y1){
	        if (fraction >= 0){
	            x0 += stepx;
	            fraction -= dy;
	        }
	        y0 += stepy;
	        fraction += dx;
	        pixels[x0+y0] = colour;
	    }
	}
	return;
}
#endif

#ifdef LINE_FASTEST16
static inline void drawLine16 (int x0, int y0, int x1, int y1, const uint16_t colour)
{
	if (!clipLine(x0, y0, x1, y1, &x0, &y0, &x1, &y1))
		return;
		
	int stepx, stepy;
	
	int dy = y1 - y0;
	if (dy < 0){
		dy = -dy;
		stepy = -VWIDTH;
	}else{
		stepy = VWIDTH;
	}
	dy <<= 1;
		
	int dx = x1 - x0;
	if (dx < 0){
		dx = -dx;
		stepx = -1;
	}else{
		stepx = 1;
	}
	dx <<= 1;
	
	y0 *= VWIDTH;
	y1 *= VWIDTH;

	uint16_t *pixels = (uint16_t*)renderBuffer;
	pixels[x0+y0] = colour;
	
	if (dx > dy){
	    int fraction = dy - (dx >> 1);
	    
	    while (x0 != x1){
	        if (fraction >= 0){
	            y0 += stepy;
	            fraction -= dx;
	        }
	        x0 += stepx;
	        fraction += dy;
	        pixels[x0+y0] = colour;
	    }
	}else{
	    int fraction = dx - (dy >> 1);
	    
	    while (y0 != y1){
	        if (fraction >= 0){
	            x0 += stepx;
	            fraction -= dy;
	        }
	        y0 += stepy;
	        fraction += dx;
	        pixels[x0+y0] = colour;
	    }
	}
	return;
}
#endif

void drawLine (const int x1, const int y1, const int x2, const int y2, const uint16_t colour)
{
#if LINE_STD
	// slowest: standard Bresenham algorithm
	drawLineStd(x1, y1, x2, y2, colour);		// 94.0
	
#elif LINE_FAST	
	// fast: similar to Bresenham but forgoes accuracy for performance
	drawLineFast(x1, y1, x2, y2, colour);		// 58.7

#elif LINE_FASTEST16
	// faster line rountine - draw direct to 16bit buffer
	drawLine16(x1, y1, x2, y2, colour);			// 27.9

#elif LINE_FASTEST8
	// fastest line rountine - draw direct to 8bit buffer
	drawLine8(x1, y1, x2, y2, colour);
#endif

}

static inline void drawVLine (const int x, int y1, const int h, const uint8_t colour)
{
	if (x < 0 || x >= VWIDTH) return;
	int y2 = y1+h;
	if (y2 < y1) swap32i(&y1, &y2);
	if (y1 < 0) y1 = 0;
	if (y2 >= VHEIGHT) y2 = VHEIGHT - 1;

	for (int32_t y = y1; y < y2; y++)
		drawPixel(x, y, colour);
}

void drawCircleFilled (const int x0, const int y0, const float radius, const uint16_t colour)
{

	if ((x0 - radius < 0.0f) || (x0 + radius >= (float)VWIDTH)) return;
	if ((y0 - radius < 0.0f) || (y0 + radius >= (float)VHEIGHT)) return;
	
	int f = 1 - radius;
	int ddF_x = 1;
	int ddF_y = -2 * radius;
	int x = 0;
	int y = radius;
	int ylm = x0 - radius;


	while (x < y){
		if (f >= 0){
			drawVLine(x0 + y, y0 - x, 2 * x + 1, colour);
			drawVLine(x0 - y, y0 - x, 2 * x + 1, colour);
			
			ylm = x0 - y;
			y--;
			ddF_y += 2;
			f += ddF_y;
		}
		
		x++;
		ddF_x += 2;
		f += ddF_x;

		if ((x0 - x) > ylm){
			drawVLine(x0 + x, y0 - y, 2 * y + 1, colour);
			drawVLine(x0 - x, y0 - y, 2 * y + 1, colour);
		}
	}
	drawVLine(x0, y0 - radius, 2 * radius + 1, colour);
}

static inline void drawCirclePts (const int xc, const int yc, const int x, const int y, const uint16_t colour)
{
	drawPixel(xc+y, yc-x, colour);
	drawPixel(xc-y, yc-x, colour);
	drawPixel(xc+y, yc+x, colour);
	drawPixel(xc-y, yc+x, colour);
	drawPixel(xc+x, yc+y, colour);
	drawPixel(xc-x, yc+y, colour);
	drawPixel(xc+x, yc-y, colour);
	drawPixel(xc-x, yc-y, colour);
}

void drawCircle (const int xc, const int yc, const float radius, const uint16_t colour)
{
	if ((xc - radius < 0) || (xc + radius >= VWIDTH)) return;
	if ((yc - radius < 0) || (yc + radius >= VHEIGHT)) return;
	
	float x = 0.0;
	float y = radius;
	float p = 1.25f - radius;

	drawCirclePts(xc, yc, x, y, colour);
	
	while (x < y){
		x += 1.0f;
		if (p < 0.0f){
			p += 2.0f*x+1.0f;
		}else{
			y -= 1.0f;
			p += 2.0f*x+1.0f-2.0f*y;
		}
		drawCirclePts(xc, yc, x, y, colour);
	}
}

static inline void clipRect (int *x1, int *y1, int *x2, int *y2)
{
	if (*x1 < 0)
		*x1 = 0;
	else if (*x1 >= VWIDTH)
		*x1 = VWIDTH-1;
	if (*x2 < 0)
		*x2 = 0;
	else if (*x2 >= VWIDTH)
		*x2 = VWIDTH-1;
	if (*y1 < 0)
		*y1 = 0;
	else if (*y1 >= VHEIGHT)
		*y1 = VHEIGHT-1;		
	if (*y2 < 0)
		*y2 = 0;
	else if (*y2 >= VHEIGHT)
		*y2 = VHEIGHT-1;
}

void drawRectangleFilled (int x1, int y1, int x2, int y2, const uint16_t colour)
{
	clipRect(&x1, &y1, &x2, &y2);
	
	for (int y = y1; y <= y2; y++){
		for (int x = x1; x <= x2; x++)
			drawPixel(x, y, colour);
	}
}

void drawRectangle (int x1, int y1, int x2, int y2, const uint16_t colour)
{
	clipRect(&x1, &y1, &x2, &y2);
	
	drawLine(x1, y1, x2, y1, colour);		// top
	drawLine(x1, y2, x2, y2, colour);		// bottom
	drawLine(x1, y1+1, x1, y2-1, colour);	// left
	drawLine(x2, y1+1, x2, y2-1, colour);	// right
}

void drawTriangle (const int x1, const int y1, const int x2, const int y2, const int x3, const int y3, const uint16_t colour)
{
	drawLine(x1, y1, x2, y2, colour);
	drawLine(x2, y2, x3, y3, colour);
	drawLine(x1, y1, x3, y3, colour);
}


// Fill a triangle - Bresenham method
// Original from http://www.sunshine2k.de/coding/java/TriangleRasterization/TriangleRasterization.html
void drawTriangleFilled (int x1, int y1, int x2, int y2, int x3, int y3, const uint16_t colour)
{
	#define SWAP(x,y)  swap32i(&(x),&(y))
	
	int minx,maxx,t1xp,t2xp;
	bool changed1 = false;
	bool changed2 = false;
	int signx1,signx2;

	
    // Sort vertices
	if (y1 > y2) { SWAP(y1,y2); SWAP(x1,x2); }
	if (y1 > y3) { SWAP(y1,y3); SWAP(x1,x3); }
	if (y2 > y3) { SWAP(y2,y3); SWAP(x2,x3); }

	int t1x = x1;
	int t2x = x1;
	int y = y1;   // Starting points

	int dx1 = (int)(x2 - x1); if (dx1<0) { dx1=-dx1; signx1=-1; } else signx1=1;
	int dy1 = (int)(y2 - y1);
 
	int dx2 = (int)(x3 - x1); if (dx2<0) { dx2=-dx2; signx2=-1; } else signx2=1;
	int dy2 = (int)(y3 - y1);
	
	if (dy1 > dx1) {   // swap values
        SWAP(dx1,dy1);
		changed1 = true;
	}
	if (dy2 > dx2) {   // swap values
        SWAP(dy2,dx2);
		changed2 = true;
	}
	
	int e1;
	int e2 = (int)(dx2>>1);
	
    // Flat top, just process the second half
    if (y1 == y2) goto next;
    e1 = (int)(dx1>>1);
	
	for (int i = 0; i < dx1;) {
		t1xp=0; t2xp=0;
		if (t1x<t2x) { minx=t1x; maxx=t2x; }
		else		{ minx=t2x; maxx=t1x; }
        // process first line until y value is about to change
		while(i<dx1) {
			i++;			
			e1 += dy1;
	   	   	while (e1 >= dx1) {
				e1 -= dx1;
   	   	   	   if (changed1) t1xp=signx1;//t1x += signx1;
				else          goto next1;
			}
			if (changed1) break;
			else t1x += signx1;
		}
	// Move line
	next1:
        // process second line until y value is about to change
		while (1) {
			e2 += dy2;		
			while (e2 >= dx2) {
				e2 -= dx2;
				if (changed2) t2xp=signx2;//t2x += signx2;
				else          goto next2;
			}
			if (changed2)     break;
			else              t2x += signx2;
		}
	next2:
		if (minx>t1x) minx=t1x;
		if (minx>t2x) minx=t2x;
		if (maxx<t1x) maxx=t1x;
		if (maxx<t2x) maxx=t2x;
	   	drawHLine(y, minx, maxx, colour);    // Draw line from min to max points found on the y
		// Now increase y
		if (!changed1) t1x += signx1;
		t1x+=t1xp;
		if (!changed2) t2x += signx2;
		t2x+=t2xp;
    	y += 1;
		if (y==y2) break;
		
   }
	next:
	// Second half
	dx1 = (int)(x3 - x2); if (dx1<0) { dx1=-dx1; signx1=-1; } else signx1=1;
	dy1 = (int)(y3 - y2);
	t1x=x2;
 
	if (dy1 > dx1){   // swap values
        SWAP(dy1,dx1);
		changed1 = true;
	}else{
		changed1=false;
	}
	
	e1 = (int)(dx1>>1);
	
	for (int i = 0; i<=dx1; i++){
		t1xp=0;
		t2xp=0;
		
		if (t1x<t2x){
			minx=t1x;
			maxx=t2x;
		}else{
			minx=t2x;
			maxx=t1x;
		}
		
		
	    // process first line until y value is about to change
		while (i<dx1) {
    		e1 += dy1;
	   	   	while (e1 >= dx1){
				e1 -= dx1;
   	   	   	   	if (changed1) { t1xp=signx1; break; }//t1x += signx1;
				else          goto next3;
			}
			if (changed1) break;
			else   	   	  t1x += signx1;
			if (i<dx1) i++;
		}
	next3:
        // process second line until y value is about to change
		while (t2x!=x3){
			e2 += dy2;
	   	   	while (e2 >= dx2){
				e2 -= dx2;
				if (changed2) t2xp=signx2;
				else          goto next4;
			}
			if (changed2)     break;
			else              t2x += signx2;
		}	   	   
	next4:

		if (minx>t1x) minx=t1x;
		if (minx>t2x) minx=t2x;
		if (maxx<t1x) maxx=t1x;
		if (maxx<t2x) maxx=t2x;
		
	   	drawHLine(y, minx, maxx, colour);    // Draw line from min to max points found on the y
		// Now increase y
		if (!changed1) t1x += signx1;
		t1x+=t1xp;
		if (!changed2) t2x += signx2;
		t2x+=t2xp;
    	y += 1;
		if (y>y3) return;
	}
}
	
static void fillArcOffsetted (uint16_t cx, uint16_t cy, float radius, float thickness, float start, float end, uint16_t colour)
{
	float xmin = 65535.0, xmax = -32767.0, ymin = 32767.0, ymax = -32767.0;
	float cosStart, sinStart, cosEnd, sinEnd;
	float startAngle =(start / ARCMAXANGLE) * 360.0;	// 252
	float endAngle =(end / ARCMAXANGLE) * 360.0;		// 807

	while (startAngle < 0.0)
		startAngle += 360.0;
		
	while (endAngle < 0.0)
		endAngle += 360.0;
		
	while (startAngle > 360.0)
		startAngle -= 360.0;
		
	while (endAngle > 360.0)
		endAngle -= 360.0;
		
	//if (endAngle == 0) endAngle = 360;

	if (startAngle > endAngle){
		fillArcOffsetted(cx, cy, radius, thickness, ((startAngle) / 360.0) * ARCMAXANGLE, ARCMAXANGLE, colour);
		fillArcOffsetted(cx, cy, radius, thickness, 0,((endAngle) / 360.0) * ARCMAXANGLE, colour);
		
	}else{
		// Calculate bounding box for the arc to be drawn
		cosStart = cosDegrees(startAngle);
		sinStart = sinDegrees(startAngle);
		cosEnd = cosDegrees(endAngle);
		sinEnd = sinDegrees(endAngle);

		float r = radius;
		// Point 1: radius & startAngle
		float t = r * cosStart;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinStart;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		// Point 2: radius & endAngle
		t = r * cosEnd;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinEnd;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		r = radius - thickness;
		// Point 3: radius-thickness & startAngle
		t = r * cosStart;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinStart;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		// Point 4: radius-thickness & endAngle
		t = r * cosEnd;
		if (t < xmin) xmin = t;
		if (t > xmax) xmax = t;
		t = r * sinEnd;
		if (t < ymin) ymin = t;
		if (t > ymax) ymax = t;

		// Corrections if arc crosses X or Y axis
		if ((startAngle < 90) &&(endAngle > 90))
			ymax = radius;
		if ((startAngle < 180) &&(endAngle > 180))
			xmin = -radius;
		if ((startAngle < 270) &&(endAngle > 270))
			ymin = -radius;

		// Slopes for the two sides of the arc
		float sslope = (float)cosStart / (float)sinStart;
		float eslope = (float)cosEnd / (float)sinEnd;
		if (endAngle == 360) eslope = -1000000;

		int ir2 =(radius - thickness) *(radius - thickness);
		int or2 = radius * radius;
		for (int x = xmin; x <= xmax; x++){
			bool y1StartFound = false, y2StartFound = false;
			bool y1EndFound = false, y2EndSearching = false;
			int y1s = 0, y1e = 0, y2s = 0;
			
			for (int y = ymin; y <= ymax; y++){
				int x2 = x * x;
				int y2 = y * y;

				if (
					(x2 + y2 < or2 && x2 + y2 >= ir2) &&(
					(y > 0 && startAngle < 180 && x <= y * sslope) ||
					(y < 0 && startAngle > 180 && x >= y * sslope) ||
					(y < 0 && startAngle <= 180) ||
					(y == 0 && startAngle <= 180 && x < 0) ||
					(y == 0 && startAngle == 0 && x > 0)
					) &&(
					(y > 0 && endAngle < 180 && x >= y * eslope) ||
					(y < 0 && endAngle > 180 && x <= y * eslope) ||
					(y > 0 && endAngle >= 180) ||
					(y == 0 && endAngle >= 180 && x < 0) ||
					(y == 0 && startAngle == 0 && x > 0)))
				{
					if (!y1StartFound){	//start of the higher line found
						y1StartFound = true;
						y1s = y;

					}else if (y1EndFound && !y2StartFound){ //start of the lower line found
						y2StartFound = true;
						y2s = y;
						y += y1e - y1s - 1;	// calculate the most probable end of the lower line(in most cases the length of lower line is equal to length of upper line), in the next loop we will validate if the end of line is really there

						if (y > ymax - 1){ // the most probable end of line 2 is beyond ymax so line 2 must be shorter, thus continue with pixel by pixel search
							y = y2s;	// reset y and continue with pixel by pixel search
							y2EndSearching = true;
						}

					}else if (y2StartFound && !y2EndSearching){
						// we validated that the probable end of the lower line has a pixel, continue with pixel by pixel search, in most cases next loop with confirm the end of lower line as it will not find a valid pixel
						y2EndSearching = true;
					}
				}else{
					if (y1StartFound && !y1EndFound){ //higher line end found
						y1EndFound = true;
						y1e = y - 1;
						drawVLine(cx + x, cy + y1s, y - y1s, colour);
						if (y < 0)
							y = abs(y); // skip the empty middle
						else
							break;

					}else if (y2StartFound){
						if (y2EndSearching){
							// we found the end of the lower line after pixel by pixel search
							drawVLine(cx + x, cy + y2s, y - y2s, colour);
							y2EndSearching = false;
							break;
						}else{
							// the expected end of the lower line is not there so the lower line must be shorter
							y = y2s;	// put the y back to the lower line start and go pixel by pixel to find the end
							y2EndSearching = true;
						}
					}
				}
			}
			
			if (y1StartFound && !y1EndFound){
				y1e = ymax;
				drawVLine(cx + x, cy + y1s, y1e - y1s + 1, colour);
				
			}else if (y2StartFound && y2EndSearching){	// we found start of lower line but we are still searching for the end
				drawVLine(cx + x, cy + y2s, ymax - y2s + 1, colour); // which we haven't found in the loop so the last pixel in a column must be the end
			}
		}
	}
}

void drawFillArc (const uint16_t x, const uint16_t y, const float radius, const float thickness, const float start, const float end, const uint16_t colour)
{
	if (start == 0 && end == ARCMAXANGLE)
		fillArcOffsetted(x, y, radius, thickness, 0, ARCMAXANGLE, colour);
	else
		fillArcOffsetted(x, y, radius, thickness, start + (-90.0 / 360.0)*ARCMAXANGLE, end + (-90.0 / 360.0)*ARCMAXANGLE, colour);
}

void drawLineH (int32_t y, int32_t x1, int32_t x2, const uint16_t colour)
{
	drawHLine(y, x1, x2, colour);
}

void drawLineV (int32_t x, int32_t y1, int32_t y2, const uint16_t colour)
{
	drawVLine(x, y1, y2, colour);
}


typedef struct __attribute__((packed)){
	int16_t span_y_index;
	int16_t x;
}nodeX_t;

static void *qsort_verts;

/* sort edge-segments on y, then x axis */
static inline int fill_poly_v2i_y_sort (const void *a_p, const void *b_p)
{
	//const int16_t (*verts)[2] = (const int16_t(*)[2])qsort_verts;
	const v16_t *verts = (v16_t*)qsort_verts;
	
	const v16_t *a = (v16_t*)a_p;
	const v16_t *b = (v16_t*)b_p;
	const v16_t *co_a = &verts[a->x];
	const v16_t *co_b = &verts[b->x];

	if (co_a->y < co_b->y){
		return -1;
	}else if (co_a->y > co_b->y){
		return 1;
	}else if (co_a->x < co_b->x){
		return -1;
	}else if (co_a->x > co_b->x){
		return 1;
	}else{
		/* co_a & co_b are identical, use the line closest to the x-min */
		const v16_t *co = co_a;
		co_a = &verts[a->y];
		co_b = &verts[b->y];
		
		int32_t ord = (((co_b->x - co->x) * (co_a->y - co->y)) - ((co_a->x - co->x) * (co_b->y - co->y)));
		if (ord > 0) return -1;
		if (ord < 0) return 1;
	}

	return 0;
}

static inline void fill_poly_v2i_n (v16_t *verts, const int32_t total, const int32_t xmin, const int32_t ymin, const int32_t xmax, const int32_t ymax, const uint16_t colour)
{
	/* Originally by Darel Rex Finley, 2007.
	 * Optimized by Campbell Barton, 2016 to keep sorted intersections. */

	v16_t span_y[total];
	int32_t span_y_len = 0;

	for (int32_t i_curr = 0, i_prev = total - 1; i_curr < total; i_prev = i_curr++){
		const v16_t *co_prev = &verts[i_prev];
		const v16_t *co_curr = &verts[i_curr];

		if (co_prev->y != co_curr->y){
			/* Any segments entirely above or below the area of interest can be skipped. */
			if ((min(co_prev->y, co_curr->y) > ymax) || (max(co_prev->y, co_curr->y) < ymin))
				continue;

			int16_t *s = (int16_t*)&span_y[span_y_len++];
			if (co_prev->y < co_curr->y){
				s[0] = i_prev;
				s[1] = i_curr;
			}else{
				s[0] = i_curr;
				s[1] = i_prev;
			}
		}
	}

	qsort_verts = (void*)verts;
	qsort(span_y, (size_t)span_y_len, sizeof(*span_y), fill_poly_v2i_y_sort);

	nodeX_t node_x[total];
	int32_t node_x_len = 0;
	int32_t span_y_index = 0;
	
	if (span_y_len != 0 && verts[span_y[0].x].y < ymin){
		while ((span_y_index < span_y_len) && (verts[span_y[span_y_index].x].y < ymin)){
			if (verts[span_y[span_y_index].y].y >= ymin)
				node_x[node_x_len++].span_y_index = span_y_index;
			span_y_index += 1;
		}
	}

	/* Loop through the rows of the image. */
	for (int32_t pixel_y = ymin; pixel_y <= ymax; pixel_y++){
		bool is_sorted = true;
		bool do_remove = false;

		for (int32_t i = 0, x_ix_prev = -32000/*INT_MIN*/; i < node_x_len; i++){
			nodeX_t *n = &node_x[i];
			const int16_t *s = (int16_t*)&span_y[n->span_y_index];
			const v16_t *co_prev = &verts[s[0]];
			const v16_t *co_curr = &verts[s[1]];


			const float x = (co_prev->x - co_curr->x);
			const float y = (co_prev->y - co_curr->y);
			const float y_px = (pixel_y - co_curr->y);
			const int32_t x_ix = (int32_t)roundf((float)co_curr->x + ((y_px / y) * x));
			n->x = x_ix;

			if (is_sorted && (x_ix_prev > x_ix))
				is_sorted = false;

			if (do_remove == false && co_curr->y == pixel_y)
				do_remove = true;

			x_ix_prev = x_ix;
		}

		/* Sort the nodes, via a simple "Bubble" sort. */
		if (is_sorted == false){
			int32_t i = 0;
			const int32_t current_end = node_x_len-1;
			
			while (i < current_end){
				if (node_x[i].x > node_x[i + 1].x){
					swap32((int32_t*)&node_x[i], (int32_t*)&node_x[i + 1]);
				
					if (i != 0) i -= 1;
				}else{
					i += 1;
				}
			}
		}

		/* Fill the pixels between node pairs. */
		for (int32_t i = 0; i < node_x_len; i += 2){
			int16_t x_src = node_x[i].x;
			int16_t x_dst = node_x[i + 1].x;
			if (x_src >= xmax) break;

			if (x_dst > xmin){
				if (x_src < xmin) x_src = xmin;
				if (x_dst > xmax) x_dst = xmax;

				/* for single call per x-span */
				if (x_src < x_dst)
					drawHLine(pixel_y - ymin, x_src - xmin, x_dst - xmin, colour);
			}
		}

		/* Clear finalized nodes in one pass, only when needed
		 * (avoids excessive array-resizing). */
		if (do_remove == true){
			int32_t i_dst = 0;
			for (int32_t i_src = 0; i_src < node_x_len; i_src += 1){
				const int16_t *s = (int16_t*)&span_y[node_x[i_src].span_y_index];
				const v16_t *co = &verts[s[1]];
				
				if (co->y != pixel_y){
					if (i_dst != i_src){
						/* x is initialized for the next pixel_y (no need to adjust here) */
						node_x[i_dst].span_y_index = node_x[i_src].span_y_index;
					}
					i_dst += 1;
				}
			}
			node_x_len = i_dst;
		}

		/* scan for new x-nodes */
		while ((span_y_index < span_y_len) && (verts[span_y[span_y_index].x].y == pixel_y)){
			/* note, node_x these are just added at the end,
			 * not ideal but sorting once will resolve. */

			/* x is initialized for the next pixel_y */
			nodeX_t *n = &node_x[node_x_len++];
			n->span_y_index = span_y_index;
			span_y_index += 1;
		}
	}
}

void drawPolygon (v16_t *verts, const int32_t total, const uint16_t colour)
{
	fill_poly_v2i_n(verts, total, 0, 0, VWIDTH-1, VHEIGHT-1, colour);
}

void drawPolyline (const float x1, const float y1, const float x2, const float y2, const uint16_t colour)
{
 	drawLine(x1, y1, x2, y2, colour);
}

void drawPolylineSolid (const float x1, const float y1, const float x2, const float y2, const float thickness, const uint16_t colour)
{
	//if (!clipLinef(x1, y1, x2, y2, &x1, &y1, &x2, &y2))
	//	return;
		
	// cross product to calculate thickness
	float x = (y1 * 1.0f) - (1.0f * y2);
	float y = (1.0f * x2) - (x1 * 1.0f);
	const float length = sqrtf((x * x) + (y * y));
	const float mul = (1.0f / length) * thickness;

	// set thickness;
	x *= mul ; //thickness * (1.0f / length);
	y *= mul ; //thickness * (1.0f / length);
	float xh = (x / 2.0f);	// center path by precomputing and using half thickness per side
	float yh = (y / 2.0f);

	drawTriangleFilled(x1+xh, y1+yh, x1-xh, y1-yh, x2+xh, y2+yh, colour);
	drawTriangleFilled(x1-xh, y1-yh, x2+xh, y2+yh, x2-xh, y2-yh, colour);
}

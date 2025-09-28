
#ifndef _PRIMITIVES_H_
#define _PRIMITIVES_H_




#ifndef TWO_PI
#define TWO_PI					(6.283185307179586476925286766559)
#endif

#ifndef DEG_TO_RAD
#define DEG_TO_RAD				(0.017453292519943295769236907684886)
#endif

#ifndef RAD_TO_DEG
#define RAD_TO_DEG				(57.295779513082320876798154814105)
#endif


#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif


#define CALC_PITCH_1(w)		(((w)>>3)+(((w)&0x07)!=0))	// 1bit packed, calculate number of storage bytes per row given width (of glyph)
#define CALC_PITCH_16(w)	((w)*sizeof(uint16_t))		// 16bit, 8 bits per byte



#ifndef DEG2RAD
#define DEG2RAD(deg)		((deg)*DEG_TO_RAD)
#endif

#ifndef RAD2DEG
#define RAD2DEG(rad)		((rad)*RAD_TO_DEG)
#endif

#define ARCMAXANGLE			(360.0)


#endif


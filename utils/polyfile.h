


#ifndef _POLYFILE_H_
#define _POLYFILE_H_



#define PACK_ACROSS			(8)				// blocks across per packed tile
#define PACK_DOWN			PACK_ACROSS		// blocks down per packed tile
#define PACK_MASK			(0x07)			// PACK_ -1




// .poly file  header. vectors follow this as per .total vector2_t
typedef struct __attribute__((packed)){
	uint16_t type;
	uint8_t total;
}poly_field_t;


typedef struct __attribute__((packed)){
	int16_t lat;
	int16_t lon;
}poly_vector16_t;


typedef struct __attribute__((packed)){
	uint32_t offset;
	uint32_t length;
}poly_pack_file_t;


typedef struct{// __attribute__((packed)){
	poly_pack_file_t file[PACK_DOWN][PACK_ACROSS];			// X across by Y down
}poly_pack_header_t;



#endif




#ifndef serial1_console_h
#define serial1_console_h



typedef struct {
	char *cmd;
	void (*func)(const char *cmdStr);
	char *helpStr;
}cmdstr_t;

typedef struct{
	double longitude;
	double latitude;
	float altitude;		// hMSL
}__attribute__((packed))pos_rec_t;
typedef struct  FILE fileio_t;


#include "../src/record.h"
#include "../src/cmd.h"


#endif


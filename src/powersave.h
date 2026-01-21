

#ifndef _POWERSAVE_H_
#define _POWERSAVE_H_




#define POWERSAVE_PERIOD		30000	// Millseconds
#define POWERSAVE_FREQ			136		// MPU MHZ


void powersave_init ();
int powersaveDisable ();
int powersaveEnable ();
int powersaveEnableForce ();		// do not check time period
int powersaveIsEnabled ();

#endif

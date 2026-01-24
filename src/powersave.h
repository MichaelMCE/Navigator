

#ifndef _POWERSAVE_H_
#define _POWERSAVE_H_




#define POWERSAVE_PERIOD		30		// Seconds
#define POWERSAVE_FREQ			136		// MPU Mhz


void powersave_init ();
int powersaveDisable ();
int powersaveEnable ();
int powersaveEnableForce ();		// do not check time period
int powersaveIsEnabled ();

#endif

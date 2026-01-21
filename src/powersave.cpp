

#include "commonGlue.h"




static int powersaveEnabled = 0;
static uint64_t powersaveFullMS = 0;





void powersave_init ()
{
	powersaveFullMS = millis();
}

int powersaveEnableForce ()
{
	if (powersaveEnabled)
		return powersaveEnabled;

	powersaveEnabled = 1;
	mpu_setClockFreq(POWERSAVE_FREQ);
	printf(CS("MPU freq set to 136Mhz"));

	return powersaveEnabled;
}

int powersaveEnable ()
{
	if (powersaveEnabled)
		return powersaveEnabled;

	uint64_t t0 = millis();
	if (t0 - powersaveFullMS > POWERSAVE_PERIOD){
		powersaveEnabled = 1;
		mpu_setClockFreq(POWERSAVE_FREQ);
		printf(CS("MPU freq set to 136Mhz"));
	}

	return powersaveEnabled;
}

int powersaveDisable ()
{
	if (powersaveEnabled){
		powersaveEnabled = 0;
		powersaveFullMS = millis();
		mpu_setClockFreq(MPU_CLOCK_FREQ);

		printf(CS("MPU freq set to %iMhz"), (int)MPU_CLOCK_FREQ);
	}
	
	return powersaveEnabled;
}

int powersaveIsEnabled ()
{
	return powersaveEnabled;
}


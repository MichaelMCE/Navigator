

#include "commonGlue.h"




extern application_t inst;






void powersave_init ()
{
	inst.rstats.powersave.time = inst.rstats.oneSecondCounter;
}

int powersaveEnableForce ()
{
	if (inst.rstats.powersave.enabled)
		return inst.rstats.powersave.enabled;

	inst.rstats.powersave.enabled = 1;
	mpu_setClockFreq(POWERSAVE_FREQ);
	printf(CS("MPU freq set to %iMhz"), POWERSAVE_FREQ);

	return inst.rstats.powersave.enabled;
}

int powersaveEnable ()
{
	if (inst.rstats.powersave.enabled)
		return inst.rstats.powersave.enabled;

	if (inst.rstats.oneSecondCounter - inst.rstats.powersave.time > POWERSAVE_PERIOD){
		inst.rstats.powersave.time = inst.rstats.oneSecondCounter;
		inst.rstats.powersave.enabled = 1;
		mpu_setClockFreq(POWERSAVE_FREQ);
		
		//printf(CS("MPU freq set to %iMhz"), POWERSAVE_FREQ);
	}
	return inst.rstats.powersave.enabled;
}

int powersaveDisable ()
{
	if (inst.rstats.powersave.enabled){
		inst.rstats.powersave.enabled = 0;
		inst.rstats.powersave.time = inst.rstats.oneSecondCounter;
		mpu_setClockFreq(MPU_CLOCK_FREQ);

		//printf(CS("MPU freq set to %iMhz"), MPU_CLOCK_FREQ);
	}
	return inst.rstats.powersave.enabled;
}

int powersaveIsEnabled ()
{
	return inst.rstats.powersave.enabled;
}


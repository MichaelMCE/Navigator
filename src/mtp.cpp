

#include "commonGlue.h"



#if ENABLE_MTP


#include <MTP_Teensy.h>


void mtp_init ()
{
	MTP.begin();
	MTP.addFilesystem(SD, "Navigator");
}

void mtp_task ()
{
	MTP.loop();
}

#endif


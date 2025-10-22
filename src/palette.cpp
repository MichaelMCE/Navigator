

#include <Arduino.h>
#include "config.h"
//#include "vfont/vfont.h"
#include "palette.h"



extern uint16_t colourTable[PALETTE_TOTAL];



uint16_t paletteGet (const uint8_t idx)
{
	return colourTable[idx];
}

static inline void paletteSet (const uint8_t idx, const uint16_t colour)
{
	colourTable[idx] = colour;
}

FLASHMEM void palette_init ()
{
	paletteSet(COLOUR_PAL_GRAY,				COLOUR_GRAY);
	paletteSet(COLOUR_PAL_GREEN_TINT,		COLOUR_GREEN_TINT); 
	paletteSet(COLOUR_PAL_BLUE_SEA_TINT,	COLOUR_BLUE_SEA_TINT);
	paletteSet(COLOUR_PAL_HOVER,			COLOUR_HOVER);
	paletteSet(COLOUR_PAL_MAROON,			0x8000);
	paletteSet(COLOUR_PAL_GOLD,				0xFEA0);
	paletteSet(COLOUR_PAL_WHITE,			COLOUR_WHITE);
	paletteSet(COLOUR_PAL_CREAM,			COLOUR_24TO16(0xEEE7D0));
	paletteSet(COLOUR_PAL_BLACK,			COLOUR_BLACK);
	
	paletteSet(COLOUR_PAL_GREEN,			COLOUR_GREEN);
	paletteSet(COLOUR_PAL_GREENHUE,			COLOUR_24TO16(0x78E6a2));
	paletteSet(COLOUR_PAL_BRIGHTGREEN,		COLOUR_24TO16(0x28C672));
	paletteSet(COLOUR_PAL_LIGHTGREEN,		COLOUR_24TO16(0x00FF1E));		/* softer green. used for highlighting */
	paletteSet(COLOUR_PAL_DARKGREEN,		COLOUR_24TO16(0x00AA00));
	paletteSet(COLOUR_PAL_DARKERGREEN,		COLOUR_24TO16(0x005500));

	paletteSet(COLOUR_PAL_BLUE,				COLOUR_BLUE);
	paletteSet(COLOUR_PAL_DARKBLUE,			COLOUR_24TO16(0x0000BB));
	paletteSet(COLOUR_PAL_SOFTBLUE,			COLOUR_24TO16(0x7296D3));
	paletteSet(COLOUR_PAL_BLUE_SEA,			COLOUR_24TO16(0x508DC5));		/* blue, but not too dark nor too bright. eg; Glass Lite:Volume */
		
	paletteSet(COLOUR_PAL_CYAN,				COLOUR_CYAN);
	paletteSet(COLOUR_PAL_AQUA,				COLOUR_24TO16(0x00B7EB));
	
	paletteSet(COLOUR_PAL_RED,				COLOUR_RED);
	paletteSet(COLOUR_PAL_REDISH,			COLOUR_24TO16(0xFF0045));
	paletteSet(COLOUR_PAL_MAGENTA,			COLOUR_MAGENTA);
	paletteSet(COLOUR_PAL_PURPLE_GLOW,		COLOUR_24TO16(0xFF10CF));
	//paletteSet(COLOUR_PAL_REDFUZZ,			COLOUR_24TO16(0xCC1035));
	paletteSet(COLOUR_PAL_REDFUZZ,			COLOUR_24TO16(0xEC3055));
		
	paletteSet(COLOUR_PAL_YELLOW,			COLOUR_YELLOW);		
	paletteSet(COLOUR_PAL_HOMER,			COLOUR_24TO16(0xFFBF33));

	//paletteSet(COLOUR_PAL_GREY,				COLOUR_24TO16(0x777777));
	paletteSet(COLOUR_PAL_GREY,				COLOUR_24TO16(0x555555));
	paletteSet(COLOUR_PAL_LIGHTERGREY,		COLOUR_24TO16(0xB6B6B6));
	paletteSet(COLOUR_PAL_LIGHTGREY,		COLOUR_24TO16(0x969696));
	paletteSet(COLOUR_PAL_DARKGREY,			COLOUR_24TO16(0x404040));

	paletteSet(COLOUR_PAL_CHERRYBLOSSOM,	COLOUR_24TO16(0xffb7c5));
		
	paletteSet(COLOUR_PAL_TASKBARFR,		COLOUR_24TO16(0x141414));
	paletteSet(COLOUR_PAL_TASKBARBK,		COLOUR_24TO16(0xD4CAC8));

	paletteSet(COLOUR_PAL_ORANGE,			COLOUR_24TO16(0xFF7F11));

	paletteSet(COLOUR_PAL_LIGHTBROWN,		COLOUR_24TO16(0xFFA014));
	paletteSet(COLOUR_PAL_DARKBROWN,		COLOUR_24TO16(0xC73E10));
	paletteSet(COLOUR_PAL_DARKERBROWN,		COLOUR_24TO16(0xA71E0B));
	paletteSet(COLOUR_PAL_BROWN,			COLOUR_24TO16(0x804000));

	paletteSet(COLOUR_PAL_BACKGROUND,		COLOUR_24TO16(0xC6D0A6));
	//paletteSet(COLOUR_PAL_WATER,            COLOUR_24TO16(0x405DB5));
	
	paletteSet(COLOUR_PAL_WATER,            COLOUR_24TO16(0x99CCCC));
	paletteSet(COLOUR_PAL_HEATH,            COLOUR_24TO16(0xd6d99f));
	

	paletteSet(COLOUR_PAL_PL08,				COLOUR_24TO16(0xB09050));
	paletteSet(COLOUR_PAL_PL09,				COLOUR_24TO16(0xB09050));
	paletteSet(COLOUR_PAL_PL0B,				COLOUR_24TO16(0xFF6010));
	paletteSet(COLOUR_PAL_PL0D,				COLOUR_24TO16(0xA5E5CC));
	paletteSet(COLOUR_PAL_PL0E,				COLOUR_24TO16(0x666666));
	paletteSet(COLOUR_PAL_PL16,				COLOUR_24TO16(0xC8B068));
	paletteSet(COLOUR_PAL_PL1A,				COLOUR_24TO16(0xD4CAC8));
	paletteSet(COLOUR_PAL_PG_01,			COLOUR_24TO16(RGB(210, 192, 192)));
	paletteSet(COLOUR_PAL_PG_02,			COLOUR_24TO16(RGB(192, 192, 192)));
	paletteSet(COLOUR_PAL_PG_03,			COLOUR_24TO16(RGB(164, 176, 148)));
	//paletteSet(COLOUR_PAL_PG_04,			COLOUR_24TO16(RGB(128/2, 128/2, 128/2)));
	paletteSet(COLOUR_PAL_PG_04,			COLOUR_24TO16(0xF0C054));
	paletteSet(COLOUR_PAL_PG_05,			COLOUR_24TO16(RGB(180, 180, 180)));
	paletteSet(COLOUR_PAL_PG_06,			COLOUR_24TO16(RGB(204, 153,   0)));
	paletteSet(COLOUR_PAL_PG_07,			COLOUR_24TO16(RGB(248, 184, 128)));
	paletteSet(COLOUR_PAL_PG_08,			COLOUR_24TO16(RGB(248, 184, 128)));
	paletteSet(COLOUR_PAL_PG_09,			COLOUR_24TO16(RGB(248, 184, 128)));
	paletteSet(COLOUR_PAL_PG_0A,			COLOUR_24TO16(RGB(248, 184, 128)));
	paletteSet(COLOUR_PAL_PG_0B,			COLOUR_24TO16(RGB(248,  84,  70)));
	paletteSet(COLOUR_PAL_PG_0C,			COLOUR_24TO16(RGB(128, 128, 128)));
	paletteSet(COLOUR_PAL_PG_0D,			COLOUR_24TO16(RGB(255, 210, 150)));
	paletteSet(COLOUR_PAL_PG_0E,			COLOUR_24TO16(RGB(224, 224, 224)));
	paletteSet(COLOUR_PAL_PG_13,			COLOUR_24TO16(RGB(204, 153,   0)));
	paletteSet(COLOUR_PAL_PG_14,			COLOUR_24TO16(RGB(183, 233, 153)));
	paletteSet(COLOUR_PAL_PG_15,			COLOUR_24TO16(RGB(183, 233, 153)));
	paletteSet(COLOUR_PAL_PG_16,			COLOUR_24TO16(RGB(183, 233, 153)));
	paletteSet(COLOUR_PAL_PG_17,			COLOUR_24TO16(RGB(144, 192,   0)));
	paletteSet(COLOUR_PAL_PINK,				COLOUR_PINK);
	paletteSet(COLOUR_PAL_PG_4F,			COLOUR_24TO16(RGB(211, 245, 165)));
	paletteSet(COLOUR_PAL_PG_51,			COLOUR_24TO16(RGB(183, 233, 153)));
	
	//paletteSet(COLOUR_PAL_PLYEDGE,			COLOUR_24TO16(0x555555));
	paletteSet(COLOUR_PAL_PLYEDGE,			COLOUR_24TO16(0x777777));

	paletteSet(COLOUR_PAL_LDS,				COLOUR_24TO16(0x007DA5));
	paletteSet(COLOUR_PAL_BACKGROUND,		COLOUR_24TO16(0xF1EEE9));
	paletteSet(COLOUR_PAL_HEATH,            COLOUR_24TO16(0xd6d99f));
	
	paletteSet(COLOUR_PAL_Pitch,            COLOUR_24TO16(0x89d2ae));
	paletteSet(COLOUR_PAL_Meadow,           COLOUR_24TO16(0xcfeca8));
	
	paletteSet(COLOUR_PAL_Park ,            COLOUR_24TO16(0xcdf6ca));
	paletteSet(COLOUR_PAL_Residential,      COLOUR_24TO16(0xdddddd));
	paletteSet(COLOUR_PAL_School,           COLOUR_24TO16(0xf0f0d8));
	paletteSet(COLOUR_PAL_Industrail,       COLOUR_24TO16(0xdfd1d6));
	paletteSet(COLOUR_PAL_Building,         COLOUR_24TO16(0xc1b0af));
	paletteSet(COLOUR_PAL_Parking,          COLOUR_24TO16(0xf6eeb6));
	paletteSet(COLOUR_PAL_SportsCentre,     COLOUR_24TO16(0xf0f0d8));
	paletteSet(COLOUR_PAL_Construction,     COLOUR_24TO16(0xdfd1d6));
	paletteSet(COLOUR_PAL_Church,           COLOUR_24TO16(0x007DA5));
	paletteSet(COLOUR_PAL_Administration,   COLOUR_24TO16(0xefc8c8));
	paletteSet(COLOUR_PAL_Playground,       COLOUR_24TO16(0xccfff1));
	paletteSet(COLOUR_PAL_MixedForest,      COLOUR_24TO16(0xb3e2a8));

	paletteSet(COLOUR_PAL_Cemetery,         COLOUR_24TO16(0xaacbaf));
	paletteSet(COLOUR_PAL_GolfCourse,       COLOUR_24TO16(0xb5e3b5));
	paletteSet(COLOUR_PAL_MilArea,          COLOUR_24TO16(0xffa8a8));

	paletteSet(COLOUR_PAL_BrushScrub,       COLOUR_24TO16(0xcdf6ca));
	paletteSet(COLOUR_PAL_Sand,             COLOUR_24TO16(0xfedf88));
	paletteSet(COLOUR_PAL_MarshWetland,     COLOUR_24TO16(0xf1eee9));
	paletteSet(COLOUR_PAL_Dam,              COLOUR_24TO16(0xbababa));
	paletteSet(COLOUR_PAL_RetailArea,       COLOUR_24TO16(0xf0d9d9));
	paletteSet(COLOUR_PAL_Hospital,         COLOUR_24TO16(0xf0f0d8));
	paletteSet(COLOUR_PAL_Water,            COLOUR_24TO16(0x99cccc));
	paletteSet(COLOUR_PAL_DeciduousForest,  COLOUR_24TO16(0xb3e2a8));
	paletteSet(COLOUR_PAL_Stadium,          COLOUR_24TO16(0x33cc99));
	paletteSet(COLOUR_PAL_AirfieldApron,    COLOUR_24TO16(0xe9d1ff));
	paletteSet(COLOUR_PAL_AerodromeAirfield,COLOUR_24TO16(0xeae8e2));
	paletteSet(COLOUR_PAL_Supermarket,      COLOUR_24TO16(0xf6cdd2));
	paletteSet(COLOUR_PAL_Quarry,           COLOUR_24TO16(0xc5c3c3));
	paletteSet(COLOUR_PAL_Sea,              COLOUR_24TO16(0x99cccc));
	paletteSet(COLOUR_PAL_PowerStation,     COLOUR_24TO16(0xEC3055));
	paletteSet(COLOUR_PAL_PedestrianArea,   COLOUR_24TO16(0xededed));
	paletteSet(COLOUR_PAL_PlayingField,     COLOUR_24TO16(0x74dcba));
	paletteSet(COLOUR_PAL_RailwayPlatform,  COLOUR_24TO16(0xcc99ff));
	paletteSet(COLOUR_PAL_Pier,             COLOUR_24TO16(0xf2efe9));
	paletteSet(COLOUR_PAL_Barracks,         COLOUR_24TO16(0xff8f8f));
	paletteSet(COLOUR_PAL_Zoo,              COLOUR_24TO16(0xa4f3a1));
	
	paletteSet(COLOUR_PAL_Highway,          COLOUR_24TO16(0xeb989a));
	paletteSet(COLOUR_PAL_Freeway,          COLOUR_24TO16(0x809bc0));
	paletteSet(COLOUR_PAL_Road,             COLOUR_24TO16(0xfefeb3));
	paletteSet(COLOUR_PAL_ArterialRoad,     COLOUR_24TO16(0xfdd6a4));
	paletteSet(COLOUR_PAL_PrincipledHighway,COLOUR_24TO16(0xeb989a));
	paletteSet(COLOUR_PAL_ResidentialStreet,COLOUR_24TO16(0xfefefe));
	paletteSet(COLOUR_PAL_CollectorRoad,    COLOUR_24TO16(0xfefeb3));
	paletteSet(COLOUR_PAL_ServiceRoad,      COLOUR_24TO16(0xfefefe));
	paletteSet(COLOUR_PAL_LivingStreet ,    COLOUR_24TO16(0xbababa));
	paletteSet(COLOUR_PAL_Roundabout,       COLOUR_24TO16(0xfefefe));
	paletteSet(COLOUR_PAL_Path,             COLOUR_24TO16(0x30b520));
	paletteSet(COLOUR_PAL_CountryRoad,      COLOUR_24TO16(0xfefeb3));
	paletteSet(COLOUR_PAL_FootPath,         COLOUR_24TO16(0xbababa));
	paletteSet(COLOUR_PAL_ServiceRoadRestricted, COLOUR_24TO16(0xf6d4d4));
	paletteSet(COLOUR_PAL_Steps,            COLOUR_24TO16(0xff656a));
	paletteSet(COLOUR_PAL_Cycleway,         COLOUR_24TO16(0x0080ff));
	paletteSet(COLOUR_PAL_AirportRunway,    COLOUR_24TO16(0xbbbbcc));
	paletteSet(COLOUR_PAL_Mooring,          COLOUR_24TO16(0xf2efe9));
	
}


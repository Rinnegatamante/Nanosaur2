/****************************/
/*   	PAUSED.C			*/
/* (c)2002 Pangea Software  */
/* By Brian Greenstone      */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static Boolean NavigatePausedMenu(void);
static void MovePausedMenuItem(ObjNode *theNode);
static void InitPausedObjects(void);



/****************************/
/*    CONSTANTS             */
/****************************/

#define	NUM_PAUSED_MENU_ITEMS	3

#define	PAUSED_FONT_SCALE	30.0f
#define	PAUSED_LINE_SPACING	(PAUSED_FONT_SCALE * 1.1f)

#define	PAUSED_TEXT_ANAGLYPH_Z	5.0f

/*********************/
/*    VARIABLES      */
/*********************/




/********************** DO PAUSED **************************/

void DoPaused(void)
{
Boolean	oldMute = gMuteMusicFlag;
short	i;

	InitPausedObjects();


				/*************/
				/* MAIN LOOP */
				/*************/

	CalcFramesPerSecond();
	DoSDLMaintenance();

	while(true)
	{
		gCurrentMenuItem = -1;									// assume nothing selected

		MoveCursor(gMenuCursorObj);

		for (i = 0; i < NUM_PAUSED_MENU_ITEMS; i++)
			MovePausedMenuItem(gMenuItems[i]);

		gOldMenuItem = gCurrentMenuItem;


			/* SEE IF MAKE SELECTION */

		if (NavigatePausedMenu())
			break;

			/* DRAW STUFF */

		CalcFramesPerSecond();
		DoSDLMaintenance();
		DoPlayerTerrainUpdate();										// need to call this to keep supertiles active

		gGameViewInfoPtr->frameCount &= 0xfffffffe;				// keep frame count event # so that particles won't flicker during pause

		OGL_DrawScene(DrawObjects);

	}


	PlayEffect_Parms(EFFECT_MENUSELECT, FULL_CHANNEL_VOLUME/3, FULL_CHANNEL_VOLUME/5, NORMAL_CHANNEL_RATE);


		/***********/
		/* CLEANUP */
		/***********/

	DeleteObject(gMenuCursorObj);
	for (i = 0; i < 3; i++)
		DeleteObject(gMenuItems[i]);


	if (!oldMute)									// see if restart music
		ToggleMusic();


}



/********************** INIT PAUSED OBJECTS **************************/

static void InitPausedObjects(void)
{
float	w, y;
ObjNode	*newObj;
int		px,py,pw,ph;

static const LocStrID names[NUM_PAUSED_MENU_ITEMS] =
{
	STR_RESUME,
	STR_END,
	STR_QUIT,
};

	gCurrentMenuItem = -1;

	if (!gMuteMusicFlag)							// see if pause music
		ToggleMusic();

			/*****************/
			/* BUILD OBJECTS */
			/*****************/

			/* CURSOR */

	gNewObjectDefinition.group 		= SPRITE_GROUP_FONT;
	gNewObjectDefinition.type 		= FONT_SObjType_ArrowCursor;
	gNewObjectDefinition.coord.x 	= 0;
	gNewObjectDefinition.coord.y 	= 0;
	gNewObjectDefinition.coord.z 	= 0;
	gNewObjectDefinition.flags 		= STATUS_BIT_ONLYSHOWTHISPLAYER;
	gNewObjectDefinition.slot 		= SPRITE_SLOT+5000;			// make sure this is the last sprite drawn
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.rot 		= 0;
	gNewObjectDefinition.scale 	    = CURSOR_SCALE;
	gMenuCursorObj = MakeSpriteObject(&gNewObjectDefinition, false);

	gMenuCursorObj->ColorFilter.a = .9f;

	gMenuCursorObj->AnaglyphZ = PAUSED_TEXT_ANAGLYPH_Z + .5f;


		/*********************/
		/* BUILD PAUSED MENU */
		/*********************/

//	OGL_GetCurrentViewport(&px, &py, &pw, &ph, 0);			// calc aspect ratio for player #0's pane
//	aspectRatio = (float)ph/(float)pw;

	y = 187.5;

	for (int i = 0; i < NUM_PAUSED_MENU_ITEMS; i++)
	{
		const char* name = Localize(names[i]);

		gNewObjectDefinition.coord.x 	= 640/2;
		gNewObjectDefinition.coord.y 	= y;
		gNewObjectDefinition.scale 	    = PAUSED_FONT_SCALE;
		gNewObjectDefinition.slot 		= SPRITE_SLOT + 4000;

		gMenuItems[i] = newObj 	= MakeFontStringObject(name, &gNewObjectDefinition, true);

		w = GetStringWidth(name, gNewObjectDefinition.scale);
		gMenuItemMinX[i] = gNewObjectDefinition.coord.x - (w/2);
		gMenuItemMaxX[i] = gMenuItemMinX[i] + w;

		newObj->AnaglyphZ = PAUSED_TEXT_ANAGLYPH_Z;
		newObj->Kind = i;

		y += PAUSED_LINE_SPACING;
	}

}



/*************** MOVE PAUSED MENU ITEM ***********************/

static void MovePausedMenuItem(ObjNode *theNode)
{
int		i = theNode->Kind;
float	x,y;


			/* IS THE CURSOR OVER THIS ITEM? */

	x = gMenuCursorObj->Coord.x;						// get tip of cursor coords
	y = gMenuCursorObj->Coord.y;


	if ((x >= gMenuItemMinX[i]) && (x < gMenuItemMaxX[i]))
	{
//		if ((y >= theNode->Coord.y) && (y <= (theNode->Coord.y + PAUSED_FONT_SCALE)))
		float d = fabs(y - theNode->Coord.y);
		if (d < (PAUSED_FONT_SCALE/2))

		{
			theNode->ColorFilter.r =
			theNode->ColorFilter.g =
			theNode->ColorFilter.b = 1.0;
			gCurrentMenuItem = theNode->Kind;

			if (gCurrentMenuItem != gOldMenuItem)			// change?
			{
				PlayEffect_Parms(EFFECT_CHANGESELECT, FULL_CHANNEL_VOLUME/5, FULL_CHANNEL_VOLUME/3, NORMAL_CHANNEL_RATE);
			}
			return;
		}
	}
			/* CURSOR NOT ON THIS */

	theNode->ColorFilter.r = .7;
	theNode->ColorFilter.g =
	theNode->ColorFilter.b = .6;
}



/*********************** NAVIGATE PAUSED MENU **************************/

static Boolean NavigatePausedMenu(void)
{
Boolean	continueGame = false;




			/***************************/
			/* SEE IF MAKE A SELECTION */
			/***************************/

	if (IsClickDown(SDL_BUTTON_LEFT))
	{
		PlayEffect_Parms(EFFECT_CHANGESELECT,FULL_CHANNEL_VOLUME/3,FULL_CHANNEL_VOLUME/2,NORMAL_CHANNEL_RATE);

		switch(gCurrentMenuItem)
		{
			case	0:								// RESUME
					continueGame = true;
					break;

			case	1:								// EXIT
					gGameOver = true;
					continueGame = true;
					break;

			case	2:								// QUIT
					CleanQuit();
					break;

		}
	}


			/*****************************/
			/* SEE IF CANCEL A SELECTION */
			/*****************************/

	else
	if (IsNeedDown(kNeed_UIBack, ANY_PLAYER)
		|| IsNeedDown(kNeed_UIPause, ANY_PLAYER))
	{
		continueGame = true;
	}


	return(continueGame);
}




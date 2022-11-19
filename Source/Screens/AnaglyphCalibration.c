/*******************************/
/* ANAGLYPH CALIBRATION SCREEN */
/* (c)2022 Iliyas Jorio        */
/*******************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"


/****************************/
/*    PROTOTYPES            */
/****************************/

// Make sure that the default values are covered, otherwise
// the Cycler may display an error message!
// (DEFAULT_ANAGLYPH_R, DEFAULT_ANAGLYPH_G, DEFAULT_ANAGLYPH_B)
#define ANAGLYPH_CALIBRATION_CYCLER_ARRAY \
{ \
	{STR_MOUSE_SENSITIVITY_1,  0x00}, \
	{STR_MOUSE_SENSITIVITY_2,  0x1C}, \
	{STR_MOUSE_SENSITIVITY_3,  0x38}, \
	{STR_MOUSE_SENSITIVITY_4,  0x55}, \
	{STR_MOUSE_SENSITIVITY_5,  0x71}, \
	{STR_MOUSE_SENSITIVITY_6,  0x84}, \
	{STR_MOUSE_SENSITIVITY_7,  0xA0}, \
	{STR_MOUSE_SENSITIVITY_8,  0xBC}, \
	{STR_MOUSE_SENSITIVITY_9,  0xD8}, \
	{STR_MOUSE_SENSITIVITY_10, 0xFF}, \
}

static void OnChangeAnaglyphSetting(void);
static int GetAnaglyphDisplayFlags(void);
static int GetAnaglyphDisplayFlags_ColorOnly(void);
static void DisposeAnaglyphCalibrationScreen(void);
static void MoveAnaglyphScreenHeadObject(ObjNode* theNode);
static void ResetAnaglyphSettings(void);
static int ShouldShowResetAnaglyphSettings(void);


/****************************/
/*    VARIABLES             */
/****************************/

static ObjNode* gAnaglyphScreenHead = NULL;

static const MenuItem gInGameAnaglyphMenu[] =
{
	{.id='cali'},
	{kMILabel, STR_NO_ANAGLYPH_CALIBRATION_IN_GAME, .customHeight=1.0f},
	{kMISpacer, .customHeight=3},
	{kMIPick, STR_BACK_SYMBOL,		.next='BACK' },

	//-------------------------------------------------------------------------
	// END SENTINEL

	{ .id=0 }
};

static const MenuItem gAnaglyphMenu[] =
{
	{.id='cali'},
	{
		kMICycler1, STR_3D_GLASSES_MODE,
		.callback = OnChangeAnaglyphSetting,
		.cycler =
		{
			.valuePtr = &gGamePrefs.stereoGlassesMode,
			.choices =
			{
				{STR_3D_GLASSES_DISABLED, STEREO_GLASSES_MODE_OFF},
				{STR_3D_GLASSES_ANAGLYPH_COLOR, STEREO_GLASSES_MODE_ANAGLYPH_COLOR},
				{STR_3D_GLASSES_ANAGLYPH_MONO, STEREO_GLASSES_MODE_ANAGLYPH_MONO},
//				{STR_3D_GLASSES_SHUTTER, STEREO_GLASSES_MODE_SHUTTER},
			},
		},
	},
	{
		kMICycler1,
		.text = STR_3D_GLASSES_R,
		.callback = SetUpAnaglyphCalibrationScreen,
		.getLayoutFlags = GetAnaglyphDisplayFlags,
		.cycler=
		{
			.valuePtr=&gGamePrefs.anaglyphCalibrationRed,
			.choices=ANAGLYPH_CALIBRATION_CYCLER_ARRAY,
		},
	},
	{
		kMICycler1,
		.text = STR_3D_GLASSES_G,
		.callback = SetUpAnaglyphCalibrationScreen,
		.getLayoutFlags = GetAnaglyphDisplayFlags_ColorOnly,
		.cycler=
		{
			.valuePtr=&gGamePrefs.anaglyphCalibrationGreen,
			.choices=ANAGLYPH_CALIBRATION_CYCLER_ARRAY,
		},
	},
	{
		kMICycler1,
		.text = STR_3D_GLASSES_B,
		.callback = SetUpAnaglyphCalibrationScreen,
		.getLayoutFlags = GetAnaglyphDisplayFlags,
		.cycler=
		{
			.valuePtr=&gGamePrefs.anaglyphCalibrationBlue,
			.choices=ANAGLYPH_CALIBRATION_CYCLER_ARRAY,
		},
	},
	{
		kMICycler1,
		.text = STR_3D_GLASSES_CHANNEL_BALANCING,
		.callback = SetUpAnaglyphCalibrationScreen,
		.getLayoutFlags = GetAnaglyphDisplayFlags_ColorOnly,
		.cycler =
		{
			.valuePtr = &gGamePrefs.doAnaglyphChannelBalancing,
			.choices =
			{
					{STR_OFF, 0},
					{STR_ON, 1},
			},
		},
	},
	{
		kMIPick,
		STR_RESTORE_DEFAULT_CONFIG,
		.getLayoutFlags=ShouldShowResetAnaglyphSettings,
		.callback=ResetAnaglyphSettings,
		.customHeight=.7f
	},
	{kMIPick, STR_BACK_SYMBOL,		.next='BACK' },
	{kMISpacer, .customHeight=8},


	//-------------------------------------------------------------------------
	// END SENTINEL

	{ .id=0 }
};


/******** SET UP ANAGLYPH CALIBRATION SCREEN *********/

void SetUpAnaglyphCalibrationScreen(void)
{
			/* REGISTER MENU */

	if (gPlayNow)
	{
		RegisterMenu(gInGameAnaglyphMenu);		// can't show actual menu in-game
		return;
	}
	else
	{
		RegisterMenu(gAnaglyphMenu);
	}

			/* NUKE AND RELOAD TEXTURES SO THE CURRENT ANAGLYPH FILTER APPLIES TO THEM */

	DisposeAnaglyphCalibrationScreen();

	DisposeGlobalAssets();		// reload the font - won't apply to current menu because it keeps a reference to the
	LoadGlobalAssets();			// old material, but at least the text will look correct when we exit the menu.

	BuildMainMenuObjects();		// rebuild background image

	LoadSpriteAtlas(SPRITE_GROUP_FONT3, "subtitlefont", kAtlasLoadFont);

			/* CREATE HEAD SENTINEL - ALL ANAGLYPH CALIB OBJECTS WILL BE CHAINED TO IT */

	NewObjectDefinitionType headSentinelDef =
	{
		.group = CUSTOM_GENRE,
		.scale = 1,
		.slot = SPRITE_SLOT,
		.flags = STATUS_BIT_HIDDEN | STATUS_BIT_MOVEINPAUSE,
		.moveCall = MoveAnaglyphScreenHeadObject,
	};
	ObjNode* anaglyphScreenHead = MakeNewObject(&headSentinelDef);
	gAnaglyphScreenHead = anaglyphScreenHead;

			/* MAKE HELP BLURB */

	char blurb[1024];

	if (IsStereoAnaglyphColor())
	{
		snprintf(blurb, sizeof(blurb),
				"%s\n \n"
				"1. %s\n \n"
				"2. %s\n \n"
				"3. %s",
				Localize(STR_ANAGLYPH_HELP_WHILEWEARING),
				Localize(STR_ANAGLYPH_HELP_ADJUSTRB),
				Localize(STR_ANAGLYPH_HELP_ADJUSTG),
				Localize(STR_ANAGLYPH_HELP_CHANNELBALANCING));

	}
	else if (IsStereoAnaglyphMono())
	{
		snprintf(blurb, sizeof(blurb), "%s\n \n%s",
				Localize(STR_ANAGLYPH_HELP_WHILEWEARING),
				Localize(STR_ANAGLYPH_HELP_ADJUSTRB));
	}
	else
	{
		snprintf(blurb, sizeof(blurb), "\n");
	}

	NewObjectDefinitionType blurbDef =
	{
		.group = SPRITE_GROUP_FONT3,
		.scale = 0.18f,
		.slot = SPRITE_SLOT+1,
		.coord = {10, 470, 0},
	};
	ObjNode* blurbNode = TextMesh_New(blurb, kTextMeshAlignLeft | kTextMeshAlignBottom, &blurbDef);
	AppendNodeToChain(anaglyphScreenHead, blurbNode);

			/* MAKE TEST PATTERN */

	LoadSpriteGroupFromFiles(SPRITE_GROUP_LEVELSPECIFIC, 1, (const char*[]) {":images:calibration1.png"}, 0);
	NewObjectDefinitionType def =
	{
		.group		= SPRITE_GROUP_LEVELSPECIFIC,
		.type		= 0,		// 0th image in sprite group
		.coord		= {500,380,0},
		.slot		= SPRITE_SLOT+2,
		.scale		= 250,
	};

	ObjNode* sampleImage = MakeSpriteObject(&def, true);
	sampleImage->AnaglyphZ = 4.0f;
	AppendNodeToChain(anaglyphScreenHead, sampleImage);
}


/****************************/
/*    CALLBACKS             */
/****************************/

static void OnChangeAnaglyphSetting(void)
{
	gAnaglyphPass = 0;
	for (int i = 0; i < 4; i++)
	{
		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		SDL_GL_SwapWindow(gSDLWindow);
	}

	SetUpAnaglyphCalibrationScreen();
	LayoutCurrentMenuAgain();
}

static int GetAnaglyphDisplayFlags(void)
{
	if (IsStereoAnaglyph())
		return 0;
	else
		return kMILayoutFlagHidden;
}

static int GetAnaglyphDisplayFlags_ColorOnly(void)
{
	if (IsStereoAnaglyphColor())
		return 0;
	else
		return kMILayoutFlagHidden;
}

static void DisposeAnaglyphCalibrationScreen(void)
{
	DisposeSpriteAtlas(SPRITE_GROUP_FONT3);
	DisposeSpriteGroup(SPRITE_GROUP_LEVELSPECIFIC);

	if (gAnaglyphScreenHead)
	{
		DeleteObject(gAnaglyphScreenHead);
		gAnaglyphScreenHead = NULL;
	}
}

static void MoveAnaglyphScreenHeadObject(ObjNode* theNode)
{
	(void) theNode;

	if (GetCurrentMenu() != 'cali')
	{
		DisposeAnaglyphCalibrationScreen();
	}
}

static void ResetAnaglyphSettings(void)
{
	gGamePrefs.doAnaglyphChannelBalancing = true;
	gGamePrefs.anaglyphCalibrationRed = DEFAULT_ANAGLYPH_R;
	gGamePrefs.anaglyphCalibrationGreen = DEFAULT_ANAGLYPH_G;
	gGamePrefs.anaglyphCalibrationBlue = DEFAULT_ANAGLYPH_B;
	SetUpAnaglyphCalibrationScreen();
	LayoutCurrentMenuAgain();
}

static int ShouldShowResetAnaglyphSettings(void)
{
	return IsStereo() ? 0 : kMILayoutFlagHidden;
}
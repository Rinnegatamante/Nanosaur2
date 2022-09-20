//
// file.h
//

#include "input.h"

		/***********************/
		/* RESOURCE STURCTURES */
		/***********************/

			/* Hedr */

typedef struct
{
	short	version;			// 0xaa.bb
	short	numAnims;			// gNumAnims
	short	numJoints;			// gNumJoints
	short	num3DMFLimbs;		// gNumLimb3DMFLimbs
}SkeletonFile_Header_Type;

			/* Bone resource */
			//
			// matches BoneDefinitionType except missing
			// point and normals arrays which are stored in other resources.
			// Also missing other stuff since arent saved anyway.

typedef struct
{
	long 				parentBone;			 		// index to previous bone
	unsigned char		name[32];					// text string name for bone
	OGLPoint3D			coord;						// absolute coord (not relative to parent!)
	u_short				numPointsAttachedToBone;	// # vertices/points that this bone has
	u_short				numNormalsAttachedToBone;	// # vertex normals this bone has
	u_long				reserved[8];				// reserved for future use
}File_BoneDefinitionType;


			/* TERRAIN ITEM ENTRY TYPE */
			//
			// when we read this in, some additional data is calculated and store in the TerrainItemEntryType stuct
			//

typedef struct
{
	u_long							x,y;
	u_short							type;
	Byte							parm[4];
	u_short							flags;
}File_TerrainItemEntryType;


			/* Joit */

typedef struct
{
	OGLVector3D		maxRot;						// max rot values of joint
	OGLVector3D		minRot;						// min rot values of joint
	long 			parentBone; 		// index to previous link joint definition
	unsigned char	name[32];						// text string name for joint
	long			limbIndex;					// index into limb list
}Joit_Rez_Type;




			/* AnHd */

typedef struct
{
	Str32	animName;
	short	numAnimEvents;
}SkeletonFile_AnimHeader_Type;



		/* PREFERENCES */

#define	MAX_HTTP_NOTES	1000

#define	CURRENT_PREFS_VERS	0x0300

typedef struct
{
	Boolean	showScreenModeDialog;
	short	depth;
	int		screenWidth;
	int		screenHeight;
	double	hz;
	Boolean	lowRenderQuality;
	Byte	language;
//	DateTimeRec	lastVersCheckDate;
	Byte	didThisNote[MAX_HTTP_NOTES];
	Boolean	hasConfiguredISpControls;

	HIDControlSettingsType	controlSettings;

	Byte	splitScreenMode;

	Byte	stereoGlassesMode;
	Boolean	anaglyphColor;
	short	anaglyphCalibrationRed;
	short	anaglyphCalibrationGreen;
	short	anaglyphCalibrationBlue;
	Boolean doAnaglyphChannelBalancing;

	Boolean	showTargetingCrosshairs;

	u_long	version;

	Boolean kiddieMode;
	Boolean dontUseHID;

	Byte	antialiasingLevel;

	int		reserved[8];
}PrefsType;



		/* SAVE PLAYER */

typedef struct
{
	Byte		numAgesCompleted;		// encode # ages in lower 4 bits, and stage in upper 4 bits
	Str255		playerName;
}SavePlayerType;



		/* EXTERNS */

extern	PrefsType			gGamePrefs;



//=================================================

SkeletonDefType *LoadSkeletonFile(short skeletonType, OGLSetupOutputType *setupInfo);
extern	OSErr LoadPrefs(PrefsType *prefBlock);
void SavePrefs(void);

void LoadPlayfield(FSSpec *specPtr, OGLSetupOutputType *setupInfo);
OSErr DrawPictureIntoGWorld(FSSpec *myFSSpec, GWorldPtr *theGWorld, short depth);
void GetDemoTimer(void);
void SaveDemoTimer(void);
void SetDefaultDirectory(void);

Boolean SaveGame(void);
Boolean LoadSavedGame(void);


void DecompressJPEGToGWorld(ImageDescriptionHandle imageDesc, Ptr compressedData, GWorldPtr gworld, Rect *srcRect);

void LoadLevelArt(OGLSetupOutputType *setupInfo);


Ptr LoadDataFile(const char* path, long* outLength);
char* LoadTextFile(const char* path, long* outLength);

char* CSVIterator(char** csvCursor, bool* eolOut);

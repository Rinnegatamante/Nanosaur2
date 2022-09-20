/****************************/
/*     TERRAIN.C           	*/
/* (c)2003 Pangea Software  */
/* By Brian Greenstone      */
/****************************/

/***************/
/* EXTERNALS   */
/***************/


#include "3DMath.h"
#include	"infobar.h"
#include <AGL/aglmacro.h>

extern	OGLMatrix4x4			gViewToFrustumMatrix,gWorldToViewMatrix;
extern	NewObjectDefinitionType	gNewObjectDefinition;
extern	long					gNumSplines,gNumFences, gNumWaterPatches;
extern	float					**gMapYCoords, **gMapYCoordsOriginal,gScratchF;
extern	Byte					**gMapSplitMode, gDebugMode, gNumPlayers, gCurrentSplitScreenPane, gAnaglyphPass;
extern	OGLSetupOutputType		*gGameViewInfoPtr;
extern	SuperTileItemIndexType	**gSuperTileItemIndexGrid;
extern	int						gScratch;
extern	FenceDefType			*gFenceList;
extern	AGLContext		gAGLContext;
extern	OGLBoundingBox			gObjectGroupBBoxList[MAX_BG3D_GROUPS][MAX_OBJECTS_IN_GROUP];
extern	float					gFramesPerSecond, gFramesPerSecondFrac;
extern	WaterDefType	**gWaterListHandle;
extern	WaterDefType	*gWaterList;
extern	Boolean					gForceVertexArrayUpdate[], gUsingVertexArrayRange;
extern	GLuint					gVertexArrayRangeObjects[];
extern	Byte					gActiveSplitScreenMode;

/****************************/
/*  PROTOTYPES             */
/****************************/

static short GetFreeSuperTileMemory(void);
static inline void ReleaseSuperTileObject(short superTileNum);
static void CalcNewItemDeleteWindow(Byte playerNum);
static u_short	BuildTerrainSuperTile(long	startCol, long startRow);
static void ReleaseAllSuperTiles(void);


/****************************/
/*    CONSTANTS             */
/****************************/


/**********************/
/*     VARIABLES      */
/**********************/

float			gTerrainPolygonSize,gTerrainPolygonSizeFrac;
u_long			gTerrainPolygonSizeInt;
float			gTerrainSuperTileUnitSize, gTerrainSuperTileUnitSizeFrac;
float			gMapToUnitValue, gMapToUnitValueFrac;
int				gSuperTileActiveRange = 4;

short			gNumSuperTilesDrawn;
static	Byte	gHiccupTimer;

Boolean			gDisableHiccupTimer = false;


SuperTileStatus	**gSuperTileStatusGrid = nil;				// supertile status grid



long			gTerrainTileWidth,gTerrainTileDepth;			// width & depth of terrain in tiles
long			gTerrainUnitWidth,gTerrainUnitDepth;			// width & depth of terrain in world units (see gTerrainPolygonSize)

long			gNumUniqueSuperTiles;
short		 	**gSuperTileTextureGrid = nil;			// 2d array

float			**gVertexShading = nil;					// vertex shading grid

MOMaterialObject	*gSuperTileTextureObjects[MAX_SUPERTILE_TEXTURES];

//u_short			**gAttributeGrid = nil;

long			gNumSuperTilesDeep,gNumSuperTilesWide;	  		// dimensions of terrain in terms of supertiles
static long		gCurrentSuperTileRow[MAX_PLAYERS],gCurrentSuperTileCol[MAX_PLAYERS];
static long		gPreviousSuperTileCol[MAX_PLAYERS],gPreviousSuperTileRow[MAX_PLAYERS];

short			gNumFreeSupertiles = 0;
SuperTileMemoryType	gSuperTileMemoryList[MAX_SUPERTILES];



			/* TILE SPLITTING TABLES */

					/* /  */
static	Byte	gTileTriangles1_A[SUPERTILE_SIZE][SUPERTILE_SIZE][3];
static	Byte	gTileTriangles2_A[SUPERTILE_SIZE][SUPERTILE_SIZE][3];

					/* \  */

static	Byte	gTileTriangles1_B[SUPERTILE_SIZE][SUPERTILE_SIZE][3];
static	Byte	gTileTriangles2_B[SUPERTILE_SIZE][SUPERTILE_SIZE][3];

OGLVertex		gWorkGrid[SUPERTILE_SIZE+1][SUPERTILE_SIZE+1];

OGLVector3D		gRecentTerrainNormal;							// from _Planar


		/* MASTER ARRAYS FOR ALL SUPERTILE DATA FOR CURRENT LEVEL */

static MOVertexArrayData		*gSuperTileMeshData = nil;
static MOTriangleIndecies		*gSuperTileTriangles = nil;

static OGLPoint3D				*gSuperTileCoords = nil;
static OGLTextureCoord			*gSuperTileUVs = nil;
static OGLVector3D				*gSuperTileNormals = nil;
static OGLColorRGBA		*gSuperTileColors = nil;


static	GLuint	gTerrainOpenGLFence = 0;
static	Boolean	gTerrainOpenGLFenceIsActive = false;



/****************** INIT TERRAIN MANAGER ************************/
//
// Only called at boot!
//

void InitTerrainManager(void)
{
int	x,y;

	SetTerrainScale(DEFAULT_TERRAIN_SCALE);									// set scale to some default for now

		/* BUILT TRIANGLE SPLITTING TABLES */

	for (y = 0; y < SUPERTILE_SIZE; y++)
	{
		for (x = 0; x < SUPERTILE_SIZE; x++)
		{
			gTileTriangles1_A[y][x][0] = (SUPERTILE_SIZE+1) * y + x + 1;
			gTileTriangles1_A[y][x][1] = (SUPERTILE_SIZE+1) * y + x;
			gTileTriangles1_A[y][x][2] = (SUPERTILE_SIZE+1) * (y+1) + x;

			gTileTriangles2_A[y][x][0] = (SUPERTILE_SIZE+1) * (y+1) + x + 1;
			gTileTriangles2_A[y][x][1] = (SUPERTILE_SIZE+1) * y + x + 1;
			gTileTriangles2_A[y][x][2] = (SUPERTILE_SIZE+1) * (y+1) + x;

			gTileTriangles1_B[y][x][0] = (SUPERTILE_SIZE+1) * (y+1) + x;
			gTileTriangles1_B[y][x][1] = (SUPERTILE_SIZE+1) * (y+1) + x + 1;
			gTileTriangles1_B[y][x][2] = (SUPERTILE_SIZE+1) * y + x;

			gTileTriangles2_B[y][x][0] = (SUPERTILE_SIZE+1) * (y+1) + x + 1;
			gTileTriangles2_B[y][x][1] = (SUPERTILE_SIZE+1) * y + x + 1;
			gTileTriangles2_B[y][x][2] = (SUPERTILE_SIZE+1) * y + x;
		}
	}
}


/***************** SET TERRAIN SCALE ***********************/

void SetTerrainScale(int polygonSize)
{
	gTerrainPolygonSize = 	polygonSize;										// size in world units of terrain polygon
	gTerrainPolygonSizeInt = polygonSize;

	gTerrainPolygonSizeFrac = 1.0f / gTerrainPolygonSize;


	gTerrainSuperTileUnitSize = SUPERTILE_SIZE * gTerrainPolygonSize;			// world unit size of a supertile
	gTerrainSuperTileUnitSizeFrac = 1.0f / gTerrainSuperTileUnitSize;

	gMapToUnitValue = 	gTerrainPolygonSize / OREOMAP_TILE_SIZE;				//value to xlate Oreo map pixel coords to 3-space unit coords
	gMapToUnitValueFrac = 1.0f / gMapToUnitValue;

	if (gGamePrefs.lowRenderQuality)
		gSuperTileActiveRange = 7;
	else
		gSuperTileActiveRange = MAX_SUPERTILE_ACTIVE_RANGE;
}


/****************** INIT CURRENT SCROLL SETTINGS *****************/

void InitCurrentScrollSettings(void)
{
long	x,y;
long	dummy1,dummy2;
ObjNode	*obj;
long	i;

	for (i = 0; i < gNumPlayers; i++)						// init settings for each player in game
	{
		x = gPlayerInfo[i].coord.x-(gSuperTileActiveRange*gTerrainSuperTileUnitSize);
		y = gPlayerInfo[i].coord.z-(gSuperTileActiveRange*gTerrainSuperTileUnitSize);
		GetSuperTileInfo(x, y, &gCurrentSuperTileCol[i], &gCurrentSuperTileRow[i], &dummy1, &dummy2);

		gPreviousSuperTileCol[i] = -100000;
		gPreviousSuperTileRow[i] = -100000;
	}

		/*************************************************************************/
		/* CREATE DUMMY CUSTOM OBJECT TO CAUSE TERRAIN DRAWING AT THE DESIRED TIME */
		/*************************************************************************/

	gNewObjectDefinition.genre		= CUSTOM_GENRE;
	gNewObjectDefinition.slot 		= TERRAIN_SLOT;
	gNewObjectDefinition.moveCall 	= nil;
	gNewObjectDefinition.flags 		= STATUS_BIT_DONTCULL|STATUS_BIT_NOLIGHTING|STATUS_BIT_NOTEXTUREWRAP;

	obj = MakeNewObject(&gNewObjectDefinition);
	obj->CustomDrawFunction = DrawTerrain;

	obj->VertexArrayMode = VERTEX_ARRAY_RANGE_TYPE_TERRAIN;

}


/******************** INIT SUPERTILE GRID *************************/

void InitSuperTileGrid(void)
{
int		r,c;

	Alloc_2d_array(SuperTileStatus, gSuperTileStatusGrid, gNumSuperTilesDeep, gNumSuperTilesWide);	// alloc 2D grid array


			/* INIT ALL GRID SLOTS TO EMPTY AND UNUSED */

	for (r = 0; r < gNumSuperTilesDeep; r++)
	{
		for (c = 0; c < gNumSuperTilesWide; c++)
		{
			gSuperTileStatusGrid[r][c].supertileIndex 	= 0;
			gSuperTileStatusGrid[r][c].statusFlags 		= 0;
			gSuperTileStatusGrid[r][c].playerHereFlags	= 0;
		}
	}

}


/***************** DISPOSE TERRAIN **********************/
//
// Deletes any existing terrain data
//

void DisposeTerrain(void)
{
int	i;

	DisposeSuperTileMemoryList();

			/* FREE ALL TEXTURE OBJECTS */

	for (i = 0; i < gNumUniqueSuperTiles; i++)
	{
		MO_DisposeObjectReference(gSuperTileTextureObjects[i]);
		gSuperTileTextureObjects[i] = nil;
	}
	gNumUniqueSuperTiles = 0;

	if (gSuperTileItemIndexGrid)
	{
		Free_2d_array(gSuperTileItemIndexGrid);
		gSuperTileItemIndexGrid = nil;
	}

	if (gSuperTileTextureGrid)
	{
		Free_2d_array(gSuperTileTextureGrid);
		gSuperTileTextureGrid = nil;
	}

	if (gSuperTileStatusGrid)
	{
		Free_2d_array(gSuperTileStatusGrid);
		gSuperTileStatusGrid = nil;
	}

	if (gVertexShading)
	{
		Free_2d_array(gVertexShading);
		gVertexShading = nil;
	}

	if (gMasterItemList)
	{
		SafeDisposePtr(gMasterItemList);
		gMasterItemList = nil;
	}

	if (gMapYCoords)
	{
		Free_2d_array(gMapYCoords);
		gMapYCoords = nil;
	}

	if (gMapYCoordsOriginal)
	{
		Free_2d_array(gMapYCoordsOriginal);
		gMapYCoordsOriginal = nil;
	}

	if (gMapSplitMode)
	{
		Free_2d_array(gMapSplitMode);
		gMapSplitMode = nil;
	}

			/* NUKE SPLINE DATA */

	if (gSplineList)
	{
		for (i = 0; i < gNumSplines; i++)
		{
			SafeDisposePtr(gSplineList[i].pointList);		// nuke point list
			SafeDisposePtr(gSplineList[i].itemList);		// nuke item list
		}
		SafeDisposePtr(gSplineList);
		gSplineList = nil;
	}

				/* NUKE WATER PATCH */

	if (gWaterListHandle)
	{
		DisposeHandle((Handle)gWaterListHandle);
		gWaterListHandle = nil;
	}

	gWaterList = nil;
	gNumWaterPatches = 0;
	gNumSuperTilesDeep = gNumSuperTilesWide = 0;

	ReleaseAllSuperTiles();

	DisposeFences();

}


#pragma mark -

/************** CREATE SUPERTILE MEMORY LIST ********************/
//
// Preallocates memory and initializes the data for the maximum number of supertiles that
// we will ever need on this level.
//

void CreateSuperTileMemoryList(OGLSetupOutputType *setupInfo)
{
int	u,v,i,j;
AGLContext 	agl_ctx = setupInfo->drawContext;


			/*************************************************/
			/* ALLOCATE ARRAYS FOR ALL THE DATA WE WILL NEED */
			/*************************************************/

	gNumFreeSupertiles = MAX_SUPERTILES;

			/* ALLOC BASE TRIMESH DATA FOR ALL SUPERTILES */

	gSuperTileMeshData = AllocPtr(sizeof(MOVertexArrayData) * MAX_SUPERTILES);
	if (gSuperTileMeshData == nil)
		DoFatalAlert("CreateSuperTileMemoryList: AllocPtr failed - gSuperTileMeshData");

			/* ALLOC TRIANGLE ARRAYS ALL SUPERTILES */

	gSuperTileTriangles = OGL_AllocVertexArrayMemory(sizeof(MOTriangleIndecies) * NUM_TRIS_IN_SUPERTILE * MAX_SUPERTILES, VERTEX_ARRAY_RANGE_TYPE_TERRAIN);


			/* ALLOC POINTS FOR ALL SUPERTILES */

	gSuperTileCoords = OGL_AllocVertexArrayMemory(sizeof(OGLPoint3D) * (NUM_VERTICES_IN_SUPERTILE * MAX_SUPERTILES), VERTEX_ARRAY_RANGE_TYPE_TERRAIN);


			/* ALLOC VERTEX NORMALS FOR ALL SUPERTILES */

	gSuperTileNormals = OGL_AllocVertexArrayMemory(sizeof(OGLVector3D) * (NUM_VERTICES_IN_SUPERTILE * MAX_SUPERTILES), VERTEX_ARRAY_RANGE_TYPE_TERRAIN);


			/* ALLOC UVS FOR ALL SUPERTILES */

	gSuperTileUVs = OGL_AllocVertexArrayMemory(sizeof(OGLTextureCoord) * NUM_VERTICES_IN_SUPERTILE * MAX_SUPERTILES, VERTEX_ARRAY_RANGE_TYPE_TERRAIN);

			/* ALLOC VERTEX COLORS FOR ALL SUPERTILES */

	gSuperTileColors = OGL_AllocVertexArrayMemory(sizeof(OGLColorRGBA) * NUM_VERTICES_IN_SUPERTILE * MAX_SUPERTILES, VERTEX_ARRAY_RANGE_TYPE_TERRAIN);




			/****************************************/
			/* FOR EACH POSSIBLE SUPERTILE SET INFO */
			/****************************************/


	for (i = 0; i < MAX_SUPERTILES; i++)
	{
		MOVertexArrayData		*meshPtr;
		OGLPoint3D				*coordPtr;
		MOTriangleIndecies		*triPtr;
		OGLTextureCoord			*uvPtr;
		OGLVector3D				*normalPtr;
		OGLColorRGBA			*colorPtr;

		gSuperTileMemoryList[i].mode = SUPERTILE_MODE_FREE;					// it's free for use


				/* POINT TO ARRAYS */

		j = i* NUM_VERTICES_IN_SUPERTILE;									// index * supertile needs

		meshPtr 		= &gSuperTileMeshData[i];
		coordPtr 		= &gSuperTileCoords[j];
		normalPtr 		= &gSuperTileNormals[j];
		uvPtr 			= &gSuperTileUVs[j];
		colorPtr		= &gSuperTileColors[j];
		triPtr 			= &gSuperTileTriangles[i * NUM_TRIS_IN_SUPERTILE];

		gSuperTileMemoryList[i].meshData = meshPtr;



				/* SET MESH STRUCTURE */

		meshPtr->VARtype		= VERTEX_ARRAY_RANGE_TYPE_TERRAIN;
		meshPtr->numMaterials	= -1;							// textures will be manually submitted in drawing function!
		meshPtr->numPoints		= NUM_VERTICES_IN_SUPERTILE;
		meshPtr->numTriangles 	= NUM_TRIS_IN_SUPERTILE;

		meshPtr->points			= coordPtr;
		meshPtr->normals 		= normalPtr;
		meshPtr->uvs[0]			= uvPtr;
//		meshPtr->colorsByte		= nil;
		meshPtr->colorsFloat	= colorPtr;
		meshPtr->triangles 		= triPtr;


				/* SET UV & COLOR VALUES */

		j = 0;
		for (v = 0; v <= SUPERTILE_SIZE; v++)
		{
			for (u = 0; u <= SUPERTILE_SIZE; u++)
			{
				uvPtr[j].u = (float)u / (float)SUPERTILE_SIZE;		// sets uv's 0.0 -> 1.0 for single texture map
				uvPtr[j].v = 1.0f - ((float)v / (float)SUPERTILE_SIZE);

				j++;
			}
		}
	}

		/* ALSO CREATE AN OPENGL "FENCE" SO THAT WE CAN TELL WHEN DRAWING THE TERRAIN IS DONE */

	if (gUsingVertexArrayRange)
	{
		glGenFencesAPPLE(1, &gTerrainOpenGLFence);
		gTerrainOpenGLFenceIsActive = false;
	}
}


/********************* DISPOSE SUPERTILE MEMORY LIST **********************/

void DisposeSuperTileMemoryList(void)
{
AGLContext agl_ctx = gAGLContext;

			/* NUKE ALL MASTER ARRAYS WHICH WILL FREE UP ALL SUPERTILE MEMORY */

	if (gSuperTileMeshData)
		SafeDisposePtr((Ptr)gSuperTileMeshData);
	gSuperTileMeshData = nil;

	if (gSuperTileTriangles)
	{
		OGL_FreeVertexArrayMemory(gSuperTileTriangles, VERTEX_ARRAY_RANGE_TYPE_TERRAIN);
		gSuperTileTriangles = nil;
	}


	if (gSuperTileCoords)
	{
		OGL_FreeVertexArrayMemory(gSuperTileCoords, VERTEX_ARRAY_RANGE_TYPE_TERRAIN);
	}
	gSuperTileCoords = nil;

	if (gSuperTileNormals)
	{
		OGL_FreeVertexArrayMemory(gSuperTileNormals, VERTEX_ARRAY_RANGE_TYPE_TERRAIN);
	}
	gSuperTileNormals = nil;

	if (gSuperTileUVs)
	{
		OGL_FreeVertexArrayMemory(gSuperTileUVs, VERTEX_ARRAY_RANGE_TYPE_TERRAIN);
	}
	gSuperTileUVs = nil;

	if (gSuperTileColors)
	{
		OGL_FreeVertexArrayMemory(gSuperTileColors, VERTEX_ARRAY_RANGE_TYPE_TERRAIN);
	}
	gSuperTileColors = nil;




			/* DISPOSE OF THE OPENGL "FENCE" */

	if (gUsingVertexArrayRange)
	{
		glDeleteFencesAPPLE(1, &gTerrainOpenGLFence);
		gTerrainOpenGLFence = nil;
		gTerrainOpenGLFenceIsActive = false;
	}

}


/***************** GET FREE SUPERTILE MEMORY *******************/
//
// Finds one of the preallocated supertile memory blocks and returns its index
// IT ALSO MARKS THE BLOCK AS USED
//
// OUTPUT: index into gSuperTileMemoryList
//

static short GetFreeSuperTileMemory(void)
{
int	i;

				/* SCAN FOR A FREE BLOCK */

	for (i = 0; i < MAX_SUPERTILES; i++)
	{
		if (gSuperTileMemoryList[i].mode == SUPERTILE_MODE_FREE)
		{
			gSuperTileMemoryList[i].mode = SUPERTILE_MODE_USED;
			gNumFreeSupertiles--;
			return(i);
		}
	}

	DoFatalAlert("No Free Supertiles!");
	return(-1);											// ERROR, NO FREE BLOCKS!!!! SHOULD NEVER GET HERE!
}

#pragma mark -


/******************* BUILD TERRAIN SUPERTILE *******************/
//
// Builds a new supertile which has scrolled on
//
// INPUT: startCol = starting tile column in map
//		  startRow = starting tile row in map
//
// OUTPUT: index to supertile
//

static u_short	BuildTerrainSuperTile(long	startCol, long startRow)
{
long	 			row,col,row2,col2,numPoints,i;
u_short				superTileNum;
float				height,miny,maxy;
MOVertexArrayData	*meshData;
SuperTileMemoryType	*superTilePtr;
OGLColorRGBA		*vertexColorList;
MOTriangleIndecies	*triangleList;
OGLPoint3D			*vertexPointList;
OGLTextureCoord		*uvs;
OGLVector3D			*vertexNormals;


				/*****************************************/
				/* SEE IF WE CAN MODIFY THE TERRAIN DATA */
				/*****************************************/
				//
				// If we're using Vertex Array Range then we need to be careful that we don't modify any data that's
				// in the vertex array memory until the GPU is done with it.
				//

	if (gUsingVertexArrayRange && gTerrainOpenGLFenceIsActive)
	{
		AGLContext agl_ctx = gAGLContext;

		glFinishFenceAPPLE(gTerrainOpenGLFence);			// wait until the GPU is done using the previous frame's data
		gTerrainOpenGLFenceIsActive = false;
	}


	superTileNum = GetFreeSuperTileMemory();						// get memory block for the data
	superTilePtr = &gSuperTileMemoryList[superTileNum];				// get ptr to it


			/* SET COORDINATE DATA */

	superTilePtr->x = (startCol * gTerrainPolygonSize) + (gTerrainSuperTileUnitSize * .5f);		// also remember world coords (center of supertile)
	superTilePtr->z = (startRow * gTerrainPolygonSize) + (gTerrainSuperTileUnitSize * .5f);

	superTilePtr->left = startCol * gTerrainPolygonSize;			// also save left/back coord
	superTilePtr->back = startRow * gTerrainPolygonSize;

	superTilePtr->tileRow = startRow;								// save tile row/col
	superTilePtr->tileCol = startCol;



				/*******************/
				/* GET THE TRIMESH */
				/*******************/

	meshData 				= gSuperTileMemoryList[superTileNum].meshData;		// get ptr to mesh data
	triangleList 			= meshData->triangles;								// get ptr to triangle index list
	vertexPointList 		= meshData->points;									// get ptr to points list
	vertexColorList 		= meshData->colorsFloat;							// get ptr to vertex color
	vertexNormals			= meshData->normals;								// get ptr to vertex normals
	uvs						= meshData->uvs[0];									// get ptr to uvs

	miny = 10000000;													// init bbox counters
	maxy = -miny;


			/**********************/
			/* CREATE VERTEX GRID */
			/**********************/

	for (row2 = 0; row2 <= SUPERTILE_SIZE; row2++)
	{
		row = row2 + startRow;

		for (col2 = 0; col2 <= SUPERTILE_SIZE; col2++)
		{
			col = col2 + startCol;

			if ((row >= gTerrainTileDepth) || (col >= gTerrainTileWidth)) 	// check for edge vertices (off map array)
				height = 0;
			else
				height = gMapYCoords[row][col]; 							// get pixel height here


					/* SET COORD */

			gWorkGrid[row2][col2].point.x = (col*gTerrainPolygonSize);
			gWorkGrid[row2][col2].point.z = (row*gTerrainPolygonSize);
			gWorkGrid[row2][col2].point.y = height;							// save height @ this tile's upper left corner

					/* SET UV */

			gWorkGrid[row2][col2].uv.u = (float)col2 * (.99f / (float)SUPERTILE_SIZE);		// sets uv's 0.0 -> .99 for single texture map
			gWorkGrid[row2][col2].uv.v = .99f - ((float)row2 * (.99f / (float)SUPERTILE_SIZE));


					/* SET COLOR */

			gWorkGrid[row2][col2].color.r =
			gWorkGrid[row2][col2].color.g =
			gWorkGrid[row2][col2].color.b =
			gWorkGrid[row2][col2].color.a = 1.0;

			if (height > maxy)											// keep track of min/max
				maxy = height;
			if (height < miny)
				miny = height;
		}
	}


			/*********************************/
			/* CREATE TERRAIN MESH POLYGONS  */
			/*********************************/

					/* SET VERTEX COORDS */

	numPoints = 0;
	for (row = 0; row < (SUPERTILE_SIZE+1); row++)
	{
		for (col = 0; col < (SUPERTILE_SIZE+1); col++)
		{
			vertexPointList[numPoints] = gWorkGrid[row][col].point;						// copy from work grid
			numPoints++;
		}
	}

				/* UPDATE TRIMESH DATA WITH NEW INFO */

	i = 0;
	for (row2 = 0; row2 < SUPERTILE_SIZE; row2++)
	{
		row = row2 + startRow;

		for (col2 = 0; col2 < SUPERTILE_SIZE; col2++)
		{

			col = col2 + startCol;


					/* SET SPLITTING INFO */

			if (gMapSplitMode[row][col] == SPLIT_BACKWARD)	// set coords & uv's based on splitting
			{
					/* \ */
				triangleList[i].vertexIndices[0] 	= gTileTriangles1_B[row2][col2][0];
				triangleList[i].vertexIndices[1] 	= gTileTriangles1_B[row2][col2][1];
				triangleList[i++].vertexIndices[2] 	= gTileTriangles1_B[row2][col2][2];
				triangleList[i].vertexIndices[0] 	= gTileTriangles2_B[row2][col2][0];
				triangleList[i].vertexIndices[1] 	= gTileTriangles2_B[row2][col2][1];
				triangleList[i++].vertexIndices[2] 	= gTileTriangles2_B[row2][col2][2];
			}
			else
			{
					/* / */

				triangleList[i].vertexIndices[0] 	= gTileTriangles1_A[row2][col2][0];
				triangleList[i].vertexIndices[1] 	= gTileTriangles1_A[row2][col2][1];
				triangleList[i++].vertexIndices[2] 	= gTileTriangles1_A[row2][col2][2];
				triangleList[i].vertexIndices[0] 	= gTileTriangles2_A[row2][col2][0];
				triangleList[i].vertexIndices[1] 	= gTileTriangles2_A[row2][col2][1];
				triangleList[i++].vertexIndices[2] 	= gTileTriangles2_A[row2][col2][2];
			}
		}
	}


			/******************************/
			/* CALCULATE VERTEX NORMALS   */
			/******************************/

	CalculateSupertileVertexNormals(meshData, startRow, startCol);

			/*****************************/
			/* CALCULATE VERTEX COLORS   */
			/*****************************/

	if (vertexColorList)
	{
		float				ambientR,ambientG,ambientB;
		float				fillR0,fillG0,fillB0;
		float				fillR1 = 0,fillG1 = 0,fillB1=0;
		OGLVector3D			fillDir0,fillDir1;
		Byte				numFillLights;

			/* GET LIGHT DATA */

		ambientR = gGameViewInfoPtr->lightList.ambientColor.r;			// get ambient color
		ambientG = gGameViewInfoPtr->lightList.ambientColor.g;
		ambientB = gGameViewInfoPtr->lightList.ambientColor.b;

		fillR0 = gGameViewInfoPtr->lightList.fillColor[0].r;			// get fill color
		fillG0 = gGameViewInfoPtr->lightList.fillColor[0].g;
		fillB0 = gGameViewInfoPtr->lightList.fillColor[0].b;
		fillDir0 = gGameViewInfoPtr->lightList.fillDirection[0];		// get fill direction
		fillDir0.x = -fillDir0.x;
		fillDir0.y = -fillDir0.y;
		fillDir0.z = -fillDir0.z;

		numFillLights = gGameViewInfoPtr->lightList.numFillLights;
		if (numFillLights > 1)
		{
			fillR1 = gGameViewInfoPtr->lightList.fillColor[1].r;
			fillG1 = gGameViewInfoPtr->lightList.fillColor[1].g;
			fillB1 = gGameViewInfoPtr->lightList.fillColor[1].b;
			fillDir1 = gGameViewInfoPtr->lightList.fillDirection[1];
			fillDir1.x = -fillDir1.x;
			fillDir1.y = -fillDir1.y;
			fillDir1.z = -fillDir1.z;
		}


		i = 0;
		for (row = 0; row <= SUPERTILE_SIZE; row++)
		{
			for (col = 0; col <= SUPERTILE_SIZE; col++)
			{
				float	shade = gVertexShading[row+startRow][col+startCol];		// get value from shading grid
				float	r,g,b,dot;


						/* APPLY LIGHTING TO THE VERTEX */

				r = ambientR;												// factor in the ambient
				g = ambientG;
				b = ambientB;

				dot = OGLVector3D_Dot(&vertexNormals[i], &fillDir0);
				if (dot > 0.0f)
				{
					r += fillR0 * dot;
					g += fillG0 * dot;
					b += fillB0 * dot;
				}

				if (numFillLights > 1)
				{
					dot = OGLVector3D_Dot(&vertexNormals[i], &fillDir1);
					if (dot > 0.0f)
					{
						r += fillR1 * dot;
						g += fillG1 * dot;
						b += fillB1 * dot;
					}
				}

				if (r > 1.0f)
					r = 1.0f;
				if (g > 1.0f)
					g = 1.0f;
				if (b > 1.0f)
					b = 1.0f;


						/* SAVE COLOR INTO LIST */

				vertexColorList[i].r = r * shade;		// apply shade
				vertexColorList[i].g = g * shade;
				vertexColorList[i].b = b * shade;
				vertexColorList[i].a = 1.0f;
				i++;
			}
		}
	}

			/*********************/
			/* CALC COORD & BBOX */
			/*********************/
			//
			// This y coord is not used to translate since the terrain has no translation matrix
			// instead, this is used by the culling routine for culling tests
			//

	superTilePtr->y = (miny+maxy)*.5f;					// calc center y coord as average of top & bottom

	superTilePtr->bBox.min.x = gWorkGrid[0][0].point.x;
	superTilePtr->bBox.max.x = gWorkGrid[0][0].point.x + gTerrainSuperTileUnitSize;
	superTilePtr->bBox.min.y = miny;
	superTilePtr->bBox.max.y = maxy;
	superTilePtr->bBox.min.z = gWorkGrid[0][0].point.z;
	superTilePtr->bBox.max.z = gWorkGrid[0][0].point.z + gTerrainSuperTileUnitSize;

	if (gDisableHiccupTimer)
	{
		superTilePtr->hiccupTimer = 0;
	}
	else
	{
		superTilePtr->hiccupTimer = gHiccupTimer++;
		gHiccupTimer &= 0x1;							// spread over 2 frames
	}


			/* WE'VE MODIFIED DATA IN THE VERTEX ARRAY RANGE, SO FORCE AN UPDATE */

	gForceVertexArrayUpdate[VERTEX_ARRAY_RANGE_TYPE_TERRAIN] = true;

	return(superTileNum);
}


/******************** CALCULATE SUPERTILE VERTEX NORMALS **********************/

void CalculateSupertileVertexNormals(MOVertexArrayData	*meshData, long	startRow, long startCol)
{
OGLPoint3D			*vertexPointList;
OGLVector3D			*vertexNormals;
int					i,row,col;
MOTriangleIndecies	*triangleList;
OGLVector3D			faceNormal[NUM_TRIS_IN_SUPERTILE];
OGLVector3D			*n1,*n2;
float				avX,avY,avZ;
OGLVector3D			nA,nB;
long				ro,co;

	vertexPointList 		= meshData->points;									// get ptr to points list
	vertexNormals			= meshData->normals;								// get ptr to vertex normals
	triangleList 			= meshData->triangles;								// get ptr to triangle index list


						/* CALC FACE NORMALS */

	for (i = 0; i < NUM_TRIS_IN_SUPERTILE; i++)
	{
		CalcFaceNormal_NotNormalized(&vertexPointList[triangleList[i].vertexIndices[0]],
									&vertexPointList[triangleList[i].vertexIndices[1]],
									&vertexPointList[triangleList[i].vertexIndices[2]],
									&faceNormal[i]);
	}


			/******************************/
			/* CALCULATE VERTEX NORMALS   */
			/******************************/

	i = 0;
	for (row = 0; row <= SUPERTILE_SIZE; row++)
	{
		for (col = 0; col <= SUPERTILE_SIZE; col++)
		{

			/* SCAN 4 TILES AROUND THIS TILE TO CALC AVERAGE NORMAL FOR THIS VERTEX */
			//
			// We use the face normal already calculated for triangles inside the supertile,
			// but for tiles/tris outside the supertile (on the borders), we need to calculate
			// the face normals there.
			//

			avX = avY = avZ = 0;									// init the normal

			for (ro = -1; ro <= 0; ro++)
			{
				for (co = -1; co <= 0; co++)
				{
					long	cc = col + co;
					long	rr = row + ro;

					if ((cc >= 0) && (cc < SUPERTILE_SIZE) && (rr >= 0) && (rr < SUPERTILE_SIZE)) // see if this vertex is in supertile bounds
					{
						n1 = &faceNormal[rr * (SUPERTILE_SIZE*2) + (cc*2)];				// average 2 triangles...
						n2 = n1+1;
						avX += n1->x + n2->x;											// ...and average with current average
						avY += n1->y + n2->y;
						avZ += n1->z + n2->z;
					}
					else																// tile is out of supertile, so calc face normal & average
					{
						CalcTileNormals_NotNormalized(startRow + rr, startCol + cc, &nA,&nB);		// calculate the 2 face normals for this tile
						avX += nA.x + nB.x;												// average with current average
						avY += nA.y + nB.y;
						avZ += nA.z + nB.z;
					}
				}
			}

			FastNormalizeVector(avX, avY, avZ, &vertexNormals[i]);						// normalize the vertex normal
			i++;
		}
	}


}


/******************* RELEASE SUPERTILE OBJECT *******************/
//
// Deactivates the terrain object
//

static inline void ReleaseSuperTileObject(short superTileNum)
{
		/* MAKE SURE GPU ISNT STILL USING THIS SUPERTILE'S VERTEX DATA */

	if (gUsingVertexArrayRange && gTerrainOpenGLFenceIsActive)
	{
		AGLContext agl_ctx = gAGLContext;
		glFinishFenceAPPLE(gTerrainOpenGLFence);			// wait until the GPU is done using this
		gTerrainOpenGLFenceIsActive = false;
	}


	gSuperTileMemoryList[superTileNum].mode = SUPERTILE_MODE_FREE;		// it's free!
	gNumFreeSupertiles++;
}

/******************** RELEASE ALL SUPERTILES ************************/

static void ReleaseAllSuperTiles(void)
{
long	i;

	for (i = 0; i < MAX_SUPERTILES; i++)
		ReleaseSuperTileObject(i);

	gNumFreeSupertiles = MAX_SUPERTILES;
}

#pragma mark -

/********************* DRAW TERRAIN **************************/
//
// This is the main call to update the screen.  It draws all ObjNode's and the terrain itself
//

void DrawTerrain(ObjNode *theNode, const OGLSetupOutputType *setupInfo)
{
int				r,c;
int				i,unique;
AGLContext agl_ctx = setupInfo->drawContext;
Boolean			superTileVisible;
#pragma unused(theNode)


				/**************/
				/* DRAW STUFF */
				/**************/

		/* SET A NICE STATE FOR TERRAIN DRAWING */

	OGL_PushState();

  	glDisable(GL_NORMALIZE);														// turn off vector normalization since scale == 1
    OGL_DisableBlend();																// no blending for terrain - its always opaque
	glDisable(GL_ALPHA_TEST);	//--------

	gNumSuperTilesDrawn	= 0;


	/******************************************************************/
	/* SCAN THE SUPERTILE GRID AND LOOK FOR USED & VISIBLE SUPERTILES */
	/******************************************************************/

	for (r = 0; r < gNumSuperTilesDeep; r++)
	{
		for (c = 0; c < gNumSuperTilesWide; c++)
		{
			if (gSuperTileStatusGrid[r][c].statusFlags & SUPERTILE_IS_USED_THIS_FRAME)			// see if used
			{
				i = gSuperTileStatusGrid[r][c].supertileIndex;					// extract supertile #

					/* SEE WHICH UNIQUE SUPERTILE TEXTURE TO USE */

				unique = gSuperTileTextureGrid[r][c];
				if (unique == -1)												// if -1 then its a blank
					continue;

						/* SEE IF DELAY HICCUP TIMER */

				if (gSuperTileMemoryList[i].hiccupTimer)
				{
					gSuperTileMemoryList[i].hiccupTimer--;
					continue;
				}


					/* SEE IF IS CULLED */

				superTileVisible = OGL_IsBBoxVisible(&gSuperTileMemoryList[i].bBox, nil);
				if (!superTileVisible)
					continue;



						/***********************************/
						/* DRAW THE MESH IN THIS SUPERTILE */
						/***********************************/

					/* SUBMIT THE TEXTURE */

				MO_DrawMaterial(gSuperTileTextureObjects[unique], setupInfo);


					/* SUBMIT THE GEOMETRY */

				MO_DrawGeometry_VertexArray(gSuperTileMemoryList[i].meshData, setupInfo);
				gNumSuperTilesDrawn++;
			}
		}
	}

	OGL_PopState();
	glEnable(GL_ALPHA_TEST);	//--------



		/*********************************************/
		/* PREPARE SUPERTILE GRID FOR THE NEXT FRAME */
		/*********************************************/

	if (gActiveSplitScreenMode != SPLITSCREEN_MODE_NONE)				// if splitscreen, then dont do this until done with player #2
		if (gCurrentSplitScreenPane < 1)
			goto dont_prep_grid;

	for (r = 0; r < gNumSuperTilesDeep; r++)
	{
		for (c = 0; c < gNumSuperTilesWide; c++)
		{
			/* IF THIS SUPERTILE WAS NOT USED BUT IS DEFINED, THEN FREE IT */

			if (gSuperTileStatusGrid[r][c].statusFlags & SUPERTILE_IS_DEFINED)					// is it defined?
			{
				if (!(gSuperTileStatusGrid[r][c].statusFlags & SUPERTILE_IS_USED_THIS_FRAME))	// was it used?  If not, then release the supertile definition
				{
					ReleaseSuperTileObject(gSuperTileStatusGrid[r][c].supertileIndex);
					gSuperTileStatusGrid[r][c].statusFlags = 0;									// no longer defined
				}
			}

				/* ASSUME SUPERTILES WILL BE UNUSED ON NEXT FRAME */

			if ((gGamePrefs.stereoGlassesMode == STEREO_GLASSES_MODE_OFF) || (gAnaglyphPass > 0))
				gSuperTileStatusGrid[r][c].statusFlags &= ~SUPERTILE_IS_USED_THIS_FRAME;			// clear the isUsed bit

		}
	}

dont_prep_grid:;


		/* INSERT AN OPENGL "FENCE" INTO THE DATA STREAM SO THAT WE CAN DETECT WHEN DRAWING THE TERRAIN IS COMPLETE */

	if (gCurrentSplitScreenPane == (gNumPlayers-1))				// only submit the fence on the last pane to be drawn
	{
		glSetFenceAPPLE(gTerrainOpenGLFence);
		gTerrainOpenGLFenceIsActive = true;						// it's ok to call glTestFenceAPPLE or glFinishFenceAPPLE now that we've set it
	}

}



#pragma mark -

/***************** GET TERRAIN HEIGHT AT COORD ******************/
//
// Given a world x/z coord, return the y coord based on height map
//
// INPUT: x/z = world coords
//
// OUTPUT: y = world y coord
//

float	GetTerrainY(float x, float z)
{
OGLPlaneEquation	planeEq;
short				row,col;
OGLPoint3D			p[4];
float				xi,zi;

	if (!gMapYCoords)												// make sure there's a terrain
		return(ILLEGAL_TERRAIN_Y);

				/* CALC TILE ROW/COL INFO */

	col = x * gTerrainPolygonSizeFrac;								// see which tile row/col we're on
	row = z * gTerrainPolygonSizeFrac;

	if ((col < 0) || (col >= gTerrainTileWidth))					// check bounds
		return(0);
	if ((row < 0) || (row >= gTerrainTileDepth))
		return(0);


	xi = x - (col * gTerrainPolygonSizeInt);								// calc x/z offset into the tile
	zi = z - (row * gTerrainPolygonSizeInt);


			/* BUILD VERTICES FOR THE 4 CORNERS OF THE TILE */

	p[0].x = col * gTerrainPolygonSizeInt;								// far left
	p[0].y = gMapYCoords[row][col];
	p[0].z = row * gTerrainPolygonSizeInt;

	p[1].x = p[0].x + gTerrainPolygonSize;							// far right
	p[1].y = gMapYCoords[row][col+1];
	p[1].z = p[0].z;

	p[2].x = p[1].x;													// near right
	p[2].y = gMapYCoords[row+1][col+1];
	p[2].z = p[1].z + gTerrainPolygonSize;

	p[3].x = col * gTerrainPolygonSizeInt;								// near left
	p[3].y = gMapYCoords[row+1][col];
	p[3].z = p[2].z;


		/************************************/
		/* CALC PLANE EQUATION FOR TRIANGLE */
		/************************************/

	if (gMapSplitMode[row][col] == SPLIT_BACKWARD)			// if \ split
	{
		if (xi < zi)														// which triangle are we on?
			CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[2],&p[3]);		// calc plane equation for left triangle
		else
			CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[1], &p[2]);		// calc plane equation for right triangle
	}
	else																	// otherwise, / split
	{
		xi = gTerrainPolygonSize-xi;										// flip x
		if (xi > zi)
			CalcPlaneEquationOfTriangle(&planeEq, &p[0], &p[1], &p[3]);		// calc plane equation for left triangle
		else
			CalcPlaneEquationOfTriangle(&planeEq, &p[1], &p[2], &p[3]);		// calc plane equation for right triangle
	}

	gRecentTerrainNormal = planeEq.normal;									// remember the normal here

	return (IntersectionOfYAndPlane(x,z,&planeEq));							// calc intersection
}





/***************** GET MIN TERRAIN Y ***********************/
//
// Uses the models's bounding box to find the lowest y for all sides
//

float	GetMinTerrainY(float x, float z, short group, short type, float scale)
{
float			y,minY;
OGLBoundingBox	*bBox;
float			minX,maxX,minZ,maxZ;

			/* POINT TO BOUNDING BOX */

	bBox =  &gObjectGroupBBoxList[group][type];				// get ptr to this model's bounding box

	minX = x + bBox->min.x * scale;
	maxX = x + bBox->max.x * scale;
	minZ = z + bBox->min.z * scale;
	maxZ = z + bBox->max.z * scale;


		/* GET CENTER */

	minY = GetTerrainY(x, z);

		/* CHECK FAR LEFT */

	y = GetTerrainY(minX, minZ);
	if (y < minY)
		minY = y;


		/* CHECK FAR RIGHT */

	y = GetTerrainY(maxX, minZ);
	if (y < minY)
		minY = y;

		/* CHECK FRONT LEFT */

	y = GetTerrainY(minX, maxZ);
	if (y < minY)
		minY = y;

		/* CHECK FRONT RIGHT */

	y = GetTerrainY(maxX, maxZ);
	if (y < minY)
		minY = y;

	return(minY);
}



/***************** GET SUPERTILE INFO ******************/
//
// Given a world x/z coord, return some supertile info
//
// INPUT: x/y = world x/y coords
// OUTPUT: row/col in tile coords and supertile coords
//

void GetSuperTileInfo(long x, long z, long *superCol, long *superRow, long *tileCol, long *tileRow)
{
long	row,col;

	if ((x < 0) || (z < 0))									// see if out of bounds
		return;
	if ((x >= gTerrainUnitWidth) || (z >= gTerrainUnitDepth))
		return;

	col = x * (1.0f/gTerrainSuperTileUnitSize);				// calc supertile relative row/col that the coord lies on
	row = z * (1.0f/gTerrainSuperTileUnitSize);

	*superRow = row;										// return which supertile relative row/col it is
	*superCol = col;
	*tileRow = row*SUPERTILE_SIZE;							// return which tile row/col the super tile starts on
	*tileCol = col*SUPERTILE_SIZE;
}




#pragma mark -

/******************** DO MY TERRAIN UPDATE ********************/

void DoPlayerTerrainUpdate(void)
{
int			row,col,maxRow,maxCol,maskRow,maskCol,deltaRow,deltaCol;
Boolean		fullItemScan,moved;
Byte		mask, playerNum;
float		x,y;

static const Byte gridMask9[9*2][9*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,0,
	0,0,0,0,2,2,1,1,1,1,1,1,2,2,2,0,0,0,
	0,0,2,2,1,1,1,1,1,1,1,1,1,1,2,2,0,0,
	0,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,0,
	0,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
	2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
	0,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
	0,2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,0,
	0,0,2,2,1,1,1,1,1,1,1,1,1,1,2,2,0,0,
	0,0,0,2,2,2,1,1,1,1,1,1,2,2,2,0,0,0,
	0,0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,0
};

static const Byte gridMask8[8*2][8*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0,
	0,0,2,2,2,1,1,1,1,1,1,2,2,2,0,0,
	0,2,2,1,1,1,1,1,1,1,1,1,1,2,2,0,
	0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
	2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,2,1,1,1,1,1,1,1,1,1,1,1,1,2,2,
	0,2,1,1,1,1,1,1,1,1,1,1,1,1,2,0,
	0,2,2,1,1,1,1,1,1,1,1,1,1,2,2,0,
	0,0,2,2,2,1,1,1,1,1,1,2,2,2,0,0,
	0,0,0,0,2,2,2,2,2,2,2,2,0,0,0,0
};


static const Byte gridMask7[7*2][7*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	0,0,0,0,2,2,2,2,2,2,0,0,0,0,
	0,0,2,2,2,1,1,1,1,2,2,2,0,0,
	0,2,2,1,1,1,1,1,1,1,1,2,2,0,
	0,2,1,1,1,1,1,1,1,1,1,1,2,0,
	2,2,1,1,1,1,1,1,1,1,1,1,2,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,2,1,1,1,1,1,1,1,1,1,1,2,2,
	0,2,1,1,1,1,1,1,1,1,1,1,2,0,
	0,2,2,1,1,1,1,1,1,1,1,2,2,0,
	0,0,2,2,2,1,1,1,1,2,2,2,0,0,
	0,0,0,0,2,2,2,2,2,2,0,0,0,0
};

#if 0
	0,0,2,2,2,2,2,2,2,2,2,2,0,0,
	0,2,2,1,1,1,1,1,1,1,1,2,2,0,
	2,2,1,1,1,1,1,1,1,1,1,1,2,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,1,1,2,
	2,2,1,1,1,1,1,1,1,1,1,1,2,2,
	0,2,2,1,1,1,1,1,1,1,1,1,2,0,
	0,0,2,2,2,2,2,2,2,2,2,2,2,0
#endif

static const Byte gridMask6[6*2][6*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	0,0,2,2,2,2,2,2,2,2,0,0,
	0,2,2,1,1,1,1,1,1,2,2,0,
	2,2,1,1,1,1,1,1,1,1,2,2,
	2,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,1,1,2,
	2,2,1,1,1,1,1,1,1,1,2,2,
	0,2,2,1,1,1,1,1,1,2,2,0,
	0,0,2,2,2,2,2,2,2,2,0,0,
};


static const Byte gridMask5[5*2][5*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	0,2,2,2,2,2,2,2,2,0,
	2,2,1,1,1,1,1,1,2,2,
	2,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,1,1,2,
	2,2,1,1,1,1,1,1,2,2,
	0,2,2,2,2,2,2,2,2,0,
};


static const Byte gridMask4[4*2][4*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	2,2,2,2,2,2,2,2,
	2,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,2,
	2,1,1,1,1,1,1,2,
	2,2,2,2,2,2,2,2,
};

static const Byte gridMask3[3*2][3*2] =
{
											// 0 == empty, 1 == draw tile, 2 == item add ring
	2,2,2,2,2,2,
	2,1,1,1,1,2,
	2,1,1,1,1,2,
	2,1,1,1,1,2,
	2,1,1,1,1,2,
	2,2,2,2,2,2,
};

	if (gNumUniqueSuperTiles == 0)			// dont draw if terrain not loaded
		return;



		/* FIRST CLEAR OUT THE PLAYER FLAGS - ASSUME NO PLAYERS ON ANY SUPERTILES */

	for (row = 0; row < gNumSuperTilesDeep; row++)
		for (col = 0; col < gNumSuperTilesWide; col++)
			gSuperTileStatusGrid[row][col].playerHereFlags 	= 0;

	gHiccupTimer = 0;




	for (playerNum = 0; playerNum < gNumPlayers; playerNum++)
	{
				/* CALC PIXEL COORDS OF FAR LEFT SUPER TILE */

		x = gPlayerInfo[playerNum].camera.cameraLocation.x;
		y = gPlayerInfo[playerNum].camera.cameraLocation.z;

		x -= gSuperTileActiveRange * gTerrainSuperTileUnitSize;				// calc pixel coords of far left supertile
		y -= gSuperTileActiveRange * gTerrainSuperTileUnitSize;


					/* CALC ROW/COL SUPERTILE */

		gCurrentSuperTileCol[playerNum] = (x * gTerrainSuperTileUnitSizeFrac) + .5f;		// round to nearest row/col
		gCurrentSuperTileRow[playerNum] = (y * gTerrainSuperTileUnitSizeFrac) + .5f;


					/* SEE IF ROW/COLUMN HAVE CHANGED */

		deltaRow = abs(gCurrentSuperTileRow[playerNum] - gPreviousSuperTileRow[playerNum]);
		deltaCol = abs(gCurrentSuperTileCol[playerNum] - gPreviousSuperTileCol[playerNum]);

		if (deltaRow || deltaCol)
		{
			moved = true;
			if ((deltaRow > 1) || (deltaCol > 1))								// if moved > 1 tile then need to do full item scan
				fullItemScan = true;
			else
				fullItemScan = false;
		}
		else
		{
			moved 			= false;
			fullItemScan 	= false;
		}



			/*****************************************************************/
			/* SCAN THE GRID AND SEE WHICH SUPERTILES NEED TO BE INITIALIZED */
			/*****************************************************************/

		maxRow = gCurrentSuperTileRow[playerNum] + (gSuperTileActiveRange*2);
		maxCol = gCurrentSuperTileCol[playerNum] + (gSuperTileActiveRange*2);

		for (row = gCurrentSuperTileRow[playerNum], maskRow = 0; row < maxRow; row++, maskRow++)
		{
			if (row < 0)													// see if row is out of range
				continue;
			if (row >= gNumSuperTilesDeep)
				break;

			for (col = gCurrentSuperTileCol[playerNum], maskCol = 0; col < maxCol; col++, maskCol++)
			{
				if (col < 0)												// see if col is out of range
					continue;
				if (col >= gNumSuperTilesWide)
					break;

							/* CHECK MASK AND SEE IF WE NEED THIS */

				switch(gSuperTileActiveRange)
				{
					case	3:
							mask = gridMask3[maskRow][maskCol];
							break;

					case	4:
							mask = gridMask4[maskRow][maskCol];
							break;

					case	5:
							mask = gridMask5[maskRow][maskCol];
							break;

					case	6:
							mask = gridMask6[maskRow][maskCol];
							break;

					case	7:
							mask = gridMask7[maskRow][maskCol];
							break;

					case	8:
							mask = gridMask8[maskRow][maskCol];
							break;

					case	9:
							mask = gridMask9[maskRow][maskCol];
							break;

					default:
							return;
				}

				if (mask == 0)
					continue;
				else
				{
					gSuperTileStatusGrid[row][col].playerHereFlags |= (1 << playerNum);						// remember which players are using this supertile

								/*************************/
				                /* ONLY CREATE GEOMETRY  */
								/*************************/

							/* IS THIS SUPERTILE NOT ALREADY DEFINED? */

					if (!(gSuperTileStatusGrid[row][col].statusFlags & SUPERTILE_IS_DEFINED))
					{
						if (gSuperTileTextureGrid[row][col] != -1)										// supertiles with texture ID -1 are blank, so dont build them
						{
							gSuperTileStatusGrid[row][col].supertileIndex = BuildTerrainSuperTile(col * SUPERTILE_SIZE, row * SUPERTILE_SIZE);	// build the supertile
							gSuperTileStatusGrid[row][col].statusFlags = SUPERTILE_IS_DEFINED|SUPERTILE_IS_USED_THIS_FRAME;						// mark as defined & used
						}
					}
					else
	   					gSuperTileStatusGrid[row][col].statusFlags |= SUPERTILE_IS_USED_THIS_FRAME;		// mark this as used
				}



						/* SEE IF ADD ITEMS ON THIS SUPERTILE */

				if (moved)													// dont check for items if we didnt move
				{
					if (fullItemScan || (mask == 2))						// if full scan or mask is 2 then check for items
					{
						AddTerrainItemsOnSuperTile(row,col);
					}
				}
			}
		}

				/* UPDATE STUFF */

		gPreviousSuperTileRow[playerNum] = gCurrentSuperTileRow[playerNum];
		gPreviousSuperTileCol[playerNum] = gCurrentSuperTileCol[playerNum];

		CalcNewItemDeleteWindow(playerNum);											// recalc item delete window
	}

}


/****************** CALC NEW ITEM DELETE WINDOW *****************/

static void CalcNewItemDeleteWindow(Byte playerNum)
{
float	temp;

				/* CALC LEFT SIDE OF WINDOW */

	temp = gCurrentSuperTileCol[playerNum]*gTerrainSuperTileUnitSize;			// convert to unit coords
	gPlayerInfo[playerNum].itemDeleteWindow.left = temp;


				/* CALC RIGHT SIDE OF WINDOW */

	temp += SUPERTILE_DIST_WIDE*gTerrainSuperTileUnitSize;			// calc offset to right side
	gPlayerInfo[playerNum].itemDeleteWindow.right = temp;


				/* CALC FAR SIDE OF WINDOW */

	temp = gCurrentSuperTileRow[playerNum]*gTerrainSuperTileUnitSize;			// convert to unit coords
	gPlayerInfo[playerNum].itemDeleteWindow.top = temp;


				/* CALC NEAR SIDE OF WINDOW */

	temp += SUPERTILE_DIST_DEEP*gTerrainSuperTileUnitSize;			// calc offset to bottom side
	gPlayerInfo[playerNum].itemDeleteWindow.bottom = temp;
}





/*************** CALCULATE SPLIT MODE MATRIX ***********************/

void CalculateSplitModeMatrix(void)
{
int		row,col;
float	y0,y1,y2,y3;

	Alloc_2d_array(Byte, gMapSplitMode, gTerrainTileDepth, gTerrainTileWidth);	// alloc 2D array

	for (row = 0; row < gTerrainTileDepth; row++)
	{
		for (col = 0; col < gTerrainTileWidth; col++)
		{
				/* GET Y COORDS OF 4 VERTICES */

			y0 = gMapYCoords[row][col];
			y1 = gMapYCoords[row][col+1];
			y2 = gMapYCoords[row+1][col+1];
			y3 = gMapYCoords[row+1][col];

					/* QUICK CHECK FOR FLAT POLYS */

			if ((y0 == y1) && (y0 == y2) && (y0 == y3))							// see if all same level
				gMapSplitMode[row][col] = SPLIT_BACKWARD;

				/* CALC FOLD-SPLIT */
			else
			{
				if (fabs(y0-y2) < fabs(y1-y3))
					gMapSplitMode[row][col] = SPLIT_BACKWARD; 	// use \ splits
				else
					gMapSplitMode[row][col] = SPLIT_FORWARD;		// use / splits
			}



		}
	}
}












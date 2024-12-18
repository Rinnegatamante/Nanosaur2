/****************************/
/*   METAOBJECTS.C		    */
/* (c)2003 Pangea Software  */
/*   By Brian Greenstone    */
/****************************/


/****************************/
/*    EXTERNALS             */
/****************************/

#include "game.h"

/****************************/
/*    PROTOTYPES            */
/****************************/

static MetaObjectPtr AllocateEmptyMetaObject(uint32_t type, intptr_t subType);
static void SetMetaObjectToGroup(MOGroupObject *groupObj);
static void SetMetaObjectToGeometry(MetaObjectPtr mo, intptr_t subType, void *data);
static void SetMetaObjectToMaterial(MOMaterialObject *matObj, MOMaterialData *inData);
static void SetMetaObjectToVertexArrayGeometry(MOVertexArrayObject *geoObj, MOVertexArrayData *data);
static void SetMetaObjectToMatrix(MOMatrixObject *matObj, OGLMatrix4x4 *inData);
static void MO_DetachFromLinkedList(MetaObjectPtr obj);
static void MO_DisposeObject_Group(MOGroupObject *group);
static void MO_DeleteObjectInfo_Material(MOMaterialObject *obj);
static void MO_CalcBoundingBox_Recurse(MetaObjectPtr object, OGLBoundingBox *bBox, const OGLMatrix4x4 *m);
static void SetMetaObjectToPicture(MOPictureObject *pictObj, const char* path);
static void MO_DeleteObjectInfo_Picture(MOPictureObject *obj);

static void SetMetaObjectToSprite(MOSpriteObject *spriteObj, MOSpriteSetupData *inData);
static void MO_DisposeObject_Sprite(MOSpriteObject *obj);
static void MO_CalcBoundingSphere_Recurse(MetaObjectPtr object, float *bSphere);


/****************************/
/*    CONSTANTS             */
/****************************/





/*********************/
/*    VARIABLES      */
/*********************/

MetaObjectHeader	*gFirstMetaObject = nil;
MetaObjectHeader 	*gLastMetaObject = nil;
int					gNumMetaObjects = 0;

float				gGlobalTransparency = 1;			// 0 == clear, 1 = opaque
OGLColorRGB			gGlobalColorFilter = {1,1,1};
uint32_t				gGlobalMaterialFlags = 0;

MOMaterialObject	*gMostRecentMaterial;


/***************** INIT META OBJECT HANDLER ******************/

void MO_InitHandler(void)
{
	gFirstMetaObject = nil;				// no meta object nodes yet
	gLastMetaObject = nil;
	gNumMetaObjects = 0;
}


#pragma mark -

/****************** MO: CREATE NEW OBJECT OF TYPE ****************/
//
// INPUT:	type = type of mo to create
//			subType = subtype to create (optional)
//			data = pointer to any data needed to create the mo (optional)
//


MetaObjectPtr	MO_CreateNewObjectOfType(uint32_t type, intptr_t subType, void *data)
{
MetaObjectPtr	mo;

			/* ALLOCATE EMPTY OBJECT */

	mo = AllocateEmptyMetaObject(type,subType);
	if (mo == nil)
		return(nil);


			/* SET OBJECT INFO */

	switch(type)
	{
		case	MO_TYPE_GROUP:
				SetMetaObjectToGroup(mo);
				break;

		case	MO_TYPE_GEOMETRY:
				SetMetaObjectToGeometry(mo, subType, data);
				break;

		case	MO_TYPE_MATERIAL:
				SetMetaObjectToMaterial(mo, data);
				break;

		case	MO_TYPE_MATRIX:
				SetMetaObjectToMatrix(mo, data);
				break;

		case	MO_TYPE_PICTURE:
//				if (gGamePrefs.depth == 16)		// picture depth depends on display depth (no point in doing 32 bit if display is 16)
//					SetMetaObjectToPicture(mo, (OGLSetupOutputType *)subType, data, GL_RGB5_A1);
//				else
					SetMetaObjectToPicture(mo, data);
				break;

		case	MO_TYPE_SPRITE:
				SetMetaObjectToSprite(mo, data);
				break;

		default:
				DoFatalAlert("MO_CreateNewObjectOfType: object type not recognized");
	}

	return(mo);
}


/****************** ALLOCATE EMPTY META OBJECT *****************/
//
// allocates an empty meta object and connects it to the linked list
//

static MetaObjectPtr AllocateEmptyMetaObject(uint32_t type, intptr_t subType)
{
MetaObjectHeader	*mo;
int					size;

			/* DETERMINE SIZE OF DATA TO ALLOC */

	switch(type)
	{
		case	MO_TYPE_GROUP:
				size = sizeof(MOGroupObject);
				break;

		case	MO_TYPE_GEOMETRY:
				switch(subType)
				{
					case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
							size = sizeof(MOVertexArrayObject);
							break;

					default:
							DoFatalAlert("AllocateEmptyMetaObject: object subtype not recognized");
							return(nil);
				}
				break;

		case	MO_TYPE_MATERIAL:
				size = sizeof(MOMaterialObject);
				break;

		case	MO_TYPE_MATRIX:
				size = sizeof(MOMatrixObject);
				break;

		case	MO_TYPE_PICTURE:
				size = sizeof(MOPictureObject);
				break;

		case	MO_TYPE_SPRITE:
				size = sizeof(MOSpriteObject);
				break;

		default:
				DoFatalAlert("AllocateEmptyMetaObject: object type not recognized");
				return(nil);
	}


			/* ALLOC MEMORY FOR META OBJECT */

	mo = AllocPtrClear(size);
	if (mo == nil)
		DoFatalAlert("AllocateEmptyMetaObject: AllocPtr failed!");


			/* INIT STRUCTURE */

	mo->cookie 		= MO_COOKIE;
	mo->type 		= type;
	mo->subType 	= subType;
	mo->data 		= nil;
	mo->nextNode 	= nil;
	mo->refCount	= 1;							// initial reference count is always 1
	mo->parentGroup = nil;


		/***************************/
		/* ADD NODE TO LINKED LIST */
		/***************************/

		/* SEE IF IS ONLY NODE */

	if (gFirstMetaObject == nil)
	{
		if (gLastMetaObject)
			DoFatalAlert("AllocateEmptyMetaObject: gFirstMetaObject & gLastMetaObject should be nil");

		mo->prevNode = nil;
		gFirstMetaObject = gLastMetaObject = mo;
		gNumMetaObjects = 1;
	}

		/* ADD TO END OF LINKED LIST */

	else
	{
		mo->prevNode = gLastMetaObject;		// point new prev to last
		gLastMetaObject->nextNode = mo;		// point old last to new
		gLastMetaObject = mo;				// set new last
		gNumMetaObjects++;
	}

	return(mo);
}

/******************* SET META OBJECT TO GROUP ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//

static void SetMetaObjectToGroup(MOGroupObject *groupObj)
{

			/* INIT THE DATA */

	groupObj->objectData.numObjectsInGroup = 0;
}


/***************** SET META OBJECT TO GEOMETRY ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//

static void SetMetaObjectToGeometry(MetaObjectPtr mo, intptr_t subType, void *data)
{
	switch(subType)
	{
		case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
				SetMetaObjectToVertexArrayGeometry(mo, data);
				break;

		default:
				DoFatalAlert("SetMetaObjectToGeometry: unknown subType");

	}
}


/*************** SET META OBJECT TO VERTEX ARRAY GEOMETRY ***************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.  It also boosts the ref count of
// any referenced items.
//

static void SetMetaObjectToVertexArrayGeometry(MOVertexArrayObject *geoObj, MOVertexArrayData *data)
{
int	i;

			/* INIT THE DATA */

	geoObj->objectData = *data;									// copy from input data


		/* INCREASE MATERIAL REFERENCE COUNTS */

	for (i = 0; i < data->numMaterials; i++)
	{
		if (data->materials[i] != nil)				// make sure this material ref is valid
			MO_GetNewReference(data->materials[i]);
	}

}


/******************* SET META OBJECT TO MATERIAL ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.
//

static void SetMetaObjectToMaterial(MOMaterialObject *matObj, MOMaterialData *inData)
{

		/* COPY INPUT DATA */

	matObj->objectData = *inData;

#if 0
		{
			MO_DrawMaterial(matObj);		// safety prime ----------
			glBegin(GL_TRIANGLES);
			glTexCoord2f(0,0); glVertex3f(0,0,0);
			glTexCoord2f(1,0); glVertex3f(0,100,0);
			glTexCoord2f(0,1); glVertex3f(0,0,1000);
			glEnd();
		}
#endif

}


/******************* SET META OBJECT TO MATRIX ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.
//

static void SetMetaObjectToMatrix(MOMatrixObject *matObj, OGLMatrix4x4 *inData)
{

		/* COPY INPUT DATA */

	matObj->matrix = *inData;
}


/******************* SET META OBJECT TO PICTURE ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.
//

static void SetMetaObjectToPicture(MOPictureObject *pictObj, const char* path)
{
	int			width,height;
	MOPictureData	*picData = &pictObj->objectData;
	MOMaterialData	matData;

	/* LOAD PICTURE FILE */

	GLuint textureName = OGL_TextureMap_LoadImageFile(path, &width, &height, NULL);
	OGL_CheckError();

	/***************************/
	/* CREATE A TEXTURE OBJECT */
	/***************************/

	matData.flags			= BG3D_MATERIALFLAG_TEXTURED|BG3D_MATERIALFLAG_CLAMP_U|BG3D_MATERIALFLAG_CLAMP_V;
	matData.diffuseColor	= (OGLColorRGBA) {1,1,1,1};
	matData.numMipmaps		= 1;
	matData.width			= width;
	matData.height			= height;
	matData.textureName[0]	= textureName;
	picData->material		= MO_CreateNewObjectOfType(MO_TYPE_MATERIAL, 0, &matData);
	OGL_CheckError();
}


/******************* SET META OBJECT TO SPRITE ********************/
//
// INPUT:	mo = meta object which has already been allocated and added to linked list.
//
// This takes the given input data and copies it.
//

static void SetMetaObjectToSprite(MOSpriteObject *spriteObj, MOSpriteSetupData *inData)
{
MOSpriteData	*spriteData = &spriteObj->objectData;


		/* CREATE MATERIAL OBJECT FROM FSSPEC */

	GAME_ASSERT(!inData->loadFile);

			/* GET MATERIAL FROM SPRITE LIST */
	{
		short	group,type;

		group = inData->group;
		type = inData->type;

		if (inData->type >= gNumSpritesInGroupList[group])								// make sure type is valid
			DoFatalAlert("SetMetaObjectToSprite: illegal type");

		spriteData->material = gSpriteGroupList[group][type].materialObject;
		MO_GetNewReference(spriteData->material);										// this is a new reference, so inc ref count

		spriteData->width 		= gSpriteGroupList[group][type].width;					// get width and height of texture
		spriteData->height 		= gSpriteGroupList[group][type].height;
		spriteData->aspectRatio = gSpriteGroupList[group][type].aspectRatio;			// get aspect ratio
	}


			/*******************************/
			/* SET SOME SPRITE OBJECT DATA */
			/*******************************/

	spriteData->drawCentered = inData->drawCentered;

	spriteData->scaleBasis = spriteData->width / SPRITE_SCALE_BASIS_DENOMINATOR;		// calculate a scale basis to keep things scaled relative to texture size


	spriteData->coord.x		= -1.0;								// assume upper left corner
	spriteData->coord.y		= 1.0;
	spriteData->coord.z		= 0;								// assume in front
	spriteData->scaleX		= 1.0;								// scale is normal
	spriteData->scaleY		= 1.0;
	spriteData->rot			= 0;								// rot
}



#pragma mark -


/********************** MO: APPEND TO GROUP ****************/
//
// Attach new object to end of group
//

void MO_AppendToGroup(MOGroupObject *group, MetaObjectPtr newObject)
{
int	i;

			/* VERIFY COOKIE */

	if ((group->objectHeader.cookie != MO_COOKIE) || (((MetaObjectHeader *)newObject)->cookie != MO_COOKIE))
		DoFatalAlert("MO_AppendToGroup: cookie is invalid!");


	i = group->objectData.numObjectsInGroup++;		// get index into group list
	if (i >= MO_MAX_ITEMS_IN_GROUP)
		DoFatalAlert("MO_AppendToGroup: too many objects in group!");

	MO_GetNewReference(newObject);					// get new reference to object
	group->objectData.groupContents[i] = newObject;	// save object reference in group
}

/**************** MO: ATTACH TO GROUP START **************/
//
// Attach new object to START of group
//

void MO_AttachToGroupStart(MOGroupObject *group, MetaObjectPtr newObject)
{
int	i,j;

			/* VERIFY COOKIE */

	if ((group->objectHeader.cookie != MO_COOKIE) || (((MetaObjectHeader *)newObject)->cookie != MO_COOKIE))
		DoFatalAlert("MO_AttachToGroupStart: cookie is invalid!");


	i = group->objectData.numObjectsInGroup++;		// get index into group list
	if (i >= MO_MAX_ITEMS_IN_GROUP)
		DoFatalAlert("MO_AttachToGroupStart: too many objects in group!");

	MO_GetNewReference(newObject);					// get new reference to object


			/* SHIFT ALL EXISTING OBJECTS UP */

	for (j = i; j > 0; j--)
		group->objectData.groupContents[j] = group->objectData.groupContents[j-1];

	group->objectData.groupContents[0] = newObject;	// save object ref into group
}


#pragma mark -


/******************** MO: DRAW OBJECT ***********************/
//
// This recursive function will draw any objects submitted and parses groups.
//

void MO_DrawObject(const MetaObjectPtr object)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayObject	*vObj;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("MO_DrawObject: cookie is invalid!");


			/* HANDLE TYPE */

	switch(objHead->type)
	{
		case	MO_TYPE_GEOMETRY:
				switch(objHead->subType)
				{
					case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
							vObj = object;
							MO_DrawGeometry_VertexArray(&vObj->objectData);
							break;

					default:
						DoFatalAlert("MO_DrawObject: unknown sub-type!");
				}
				break;

		case	MO_TYPE_MATERIAL:
				MO_DrawMaterial(object);
				break;

		case	MO_TYPE_GROUP:
				MO_DrawGroup(object);
				break;

		case	MO_TYPE_MATRIX:
				MO_DrawMatrix(object);
				break;

		case	MO_TYPE_PICTURE:
				MO_DrawPicture(object);
				break;

		case	MO_TYPE_SPRITE:
				MO_DrawSprite(object);
				break;

		default:
			DoFatalAlert("MO_DrawObject: unknown type!");
	}
}


/******************** MO_DRAW GROUP *************************/

void MO_DrawGroup(const MOGroupObject *object)
{
int	numChildren,i;

				/* VERIFY OBJECT TYPE */

	if (object->objectHeader.type != MO_TYPE_GROUP)
		DoFatalAlert("MO_DrawGroup: this isnt a group!");


			/*************************************/
			/* PUSH CURRENT STATE ON STATE STACK */
			/*************************************/


	OGL_PushState();


				/***************/
				/* PARSE GROUP */
				/***************/

	numChildren = object->objectData.numObjectsInGroup;			// get # objects in group

	for (i = 0; i < numChildren; i++)
	{
		MO_DrawObject(object->objectData.groupContents[i]);
	}


			/******************************/
			/* POP OLD STATE OFF OF STACK */
			/******************************/

	OGL_PopState();
}


/******************** MO: DRAW GEOMETRY - VERTEX ARRAY *************************/

void MO_DrawGeometry_VertexArray(const MOVertexArrayData *data)
{
Boolean		useTexture = false, multiTexture = false, texGen = false;
uint32_t 		materialFlags;
short		i;
Boolean		needNormals;

			/**********************/
			/* SETUP VERTEX ARRAY */
			/**********************/

	glEnableClientState(GL_VERTEX_ARRAY);				// enable vertex arrays
	glVertexPointer(3, GL_FLOAT, 0, data->points);		// point to points array



			/***********************/
			/* SETUP VERTEX COLORS */
			/***********************/

	if (data->colorsFloat)									// do we have float colors?
	{
		glColorPointer(4, GL_FLOAT, 0, data->colorsFloat);
		glEnableClientState(GL_COLOR_ARRAY);				// enable color arrays
	}
	else
		glDisableClientState(GL_COLOR_ARRAY);				// no color data, so disable


	if (OGL_CheckError())
		DoFatalAlert("MO_DrawGeometry_VertexArray: color!");



			/****************************/
			/* SEE IF ACTIVATE MATERIAL */
			/****************************/
			//
			// For now, I'm just looking at material #0.
			//


	if (data->numMaterials < 0)							// if (-), then assume texture has been manually set
		goto use_current;

	if (data->numMaterials > 0)									// are there any materials?
	{
				/*************************************************/
				/* SEE IF DO PLAIN MULTI-TEXTURING FROM GEOMETRY */
				/*************************************************/
				//
				// If the geometry has 2+ textures assigned to is then this is what we'll use
				// for the multi-texturing.  Otherwise, we fall to the ENVMAP auto-generated
				// multi-texturing below.
				//

		if (data->numMaterials > 1)
		{
			useTexture = multiTexture = true;

			for (i = 0 ;i < data->numMaterials; i++)
			{
				OGL_ActiveTextureUnit(GL_TEXTURE0+i);								// activate texture layer #i
				OGL_EnableTexture2D();

				glTexCoordPointer(2, GL_FLOAT, 0,data->uvs[i]);						// enable uv arrays
				glEnableClientState(GL_TEXTURE_COORD_ARRAY);


							/* SET COMBINE MODE FOR TEXTURE LAYER #2 */

				if (i > 0)
				{
					uint16_t	multiTextureCombine = data->materials[0]->objectData.multiTextureCombine;
					switch(multiTextureCombine)										// set combining info
					{
						case	MULTI_TEXTURE_COMBINE_MODULATE:
								glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
								break;

						case	MULTI_TEXTURE_COMBINE_ADD:
								glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
								glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
								glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
								break;

						case	MULTI_TEXTURE_COMBINE_ADDALPHA:
								glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
								glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
								glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
								break;
					}
				}

							/* SUBMIT MATERIAL FOR THIS TEXTURE UNIT */

				MO_DrawMaterial(data->materials[i]);						// submit material #n

				if (i == 0)
				{
					gMostRecentMaterial = nil;							// need duplicate submits to be okay
				}
			}

			goto go_here;
		}


				/* MAYBE ONLY 1 MATERIAL IN GEOMETRY */

		MO_DrawMaterial(data->materials[0]);			// submit material #0 (also applies for multitexture layer 0)


			/* IF TEXTURED, THEN ALSO ACTIVATE UV ARRAY */

use_current:

		materialFlags = gMostRecentMaterial->objectData.flags;	// get material flags
		if (materialFlags & BG3D_MATERIALFLAG_TEXTURED)
		{
			if (data->uvs[0])
			{
						/**********************************/
						/* SEE IF DO ENVMAP MULTI-TEXTURE */
						/**********************************/
						//
						// We want to use reflection mapping or some other Environment mapping
						// and this uses multi-texturing.
						//

				if (materialFlags & BG3D_MATERIALFLAG_MULTITEXTURE)
				{
					uint16_t	multiTextureMode 	= gMostRecentMaterial->objectData.multiTextureMode;
					uint16_t	multiTextureCombine = gMostRecentMaterial->objectData.multiTextureCombine;
					uint16_t	envMapNum 			= gMostRecentMaterial->objectData.envMapNum;

					if (envMapNum >= gNumSpritesInGroupList[SPRITE_GROUP_SPHEREMAPS])
						DoFatalAlert("MO_DrawGeometry_VertexArray: illegal envMapNum");

					multiTexture = true;


					switch(multiTextureMode)
					{
								/* REFLECTION SPHERE */

						case	MULTI_TEXTURE_MODE_REFLECTIONSPHERE:
								for (i = 0 ;i < 2; i++)
								{
									OGL_ActiveTextureUnit(GL_TEXTURE0+i);								// activate texture layer #i
									OGL_EnableTexture2D();

									if (i == 0)
									{
										glTexCoordPointer(2, GL_FLOAT, 0,data->uvs[0]);					// enable uv arrays
										glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
										glEnableClientState(GL_TEXTURE_COORD_ARRAY);
									}
									else
									{

										MO_DrawMaterial(gSpriteGroupList[SPRITE_GROUP_SPHEREMAPS][envMapNum].materialObject);		// activate reflection map texture

//										glEnable(GL_COLOR_MATERIAL);		///----  fixes a glitch in OpenGL?

										switch(multiTextureCombine)														// set combining info
										{
											case	MULTI_TEXTURE_COMBINE_MODULATE:
													glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
													break;

											case	MULTI_TEXTURE_COMBINE_ADD:
													glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
													glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
													glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_MODULATE);
													break;

											case	MULTI_TEXTURE_COMBINE_ADDALPHA:										// note, when we do this gGlobalTransparency will have no effect
													glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE);
													glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_ADD);
													glTexEnvi(GL_TEXTURE_ENV, GL_COMBINE_ALPHA, GL_ADD);
													break;
										}
#ifndef __vita__	
										glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);							// activate reflection mapping
										glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_SPHERE_MAP);
										glEnable(GL_TEXTURE_GEN_S);
										glEnable(GL_TEXTURE_GEN_T);
#endif
										texGen = true;
									}
								}
								break;

					}
				}
							/* JUST 1 TEXTURE LAYER */
				else
				{
					glTexCoordPointer(2, GL_FLOAT, 0,data->uvs[0]);
					glEnableClientState(GL_TEXTURE_COORD_ARRAY);	// enable uv arrays
				}

				useTexture = true;

				if (OGL_CheckError())
					DoFatalAlert("MO_DrawGeometry_VertexArray: uv!");
			}
		}
	}
	else
		OGL_DisableTexture2D();							// no materials, thus no texture, thus turn this off


		/* WE DONT HAVE ENOUGH INFO TO DO TEXTURES, SO DISABLE */

	if (!useTexture)
	{
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		if (OGL_CheckError())
			DoFatalAlert("MO_DrawGeometry_VertexArray: glDisableClientState(GL_TEXTURE_COORD_ARRAY)!");
	}

go_here:

			/************************/
			/* SETUP VERTEX NORMALS */
			/************************/
			//
			// We do this last because we need to know some things
			// before we can determine if normals are actually needed
			//

	if (data->normals == nil)							// see if we even have normals to pass
		needNormals = false;
	else
	{
		if (!gMyState_Lighting)							// if no lighting, then probably don't need to pass normals
			needNormals = texGen;						// pass normals if doing texGen for sphere maps, etc.
		else											// there's lighting, so pass the normals
			needNormals = true;
	}

	if (needNormals)
	{
		glNormalPointer(GL_FLOAT, 0, data->normals);
		glEnableClientState(GL_NORMAL_ARRAY);			// enable normal arrays

#if 0
			/* SHOW VERTEX NORMALS */
		{
			int	i;

			OGL_PushState();
			OGL_DisableTexture2D();
			SetColor4f(1,1,0,1);
			for (i = 0; i < data->numPoints; i++)
			{
				glBegin(GL_LINES);

				glVertex3fv((GLfloat *)&data->points[i]);
				glVertex3f(data->points[i].x + data->normals[i].x * 20.0f,
							data->points[i].y + data->normals[i].y * 20.0f,
							data->points[i].z + data->normals[i].z * 20.0f);

				glEnd();
			}
			OGL_PopState();
		}
#endif

	}
	else
		glDisableClientState(GL_NORMAL_ARRAY);			// disable normal arrays

	OGL_CheckError();


			/***********/
			/* DRAW IT */
			/***********/

	if (data->numTriangles)
	{
		GAME_ASSERT(data->triangles);
		glDrawElements(GL_TRIANGLES,data->numTriangles*3,GL_UNSIGNED_INT,&data->triangles[0]);
		OGL_CheckError();
	}

	gPolysThisFrame += data->numTriangles;					// inc poly counter


			/* CLEANUP */

	if (multiTexture)
	{
		OGL_ActiveTextureUnit(GL_TEXTURE1);			// turn off textureing for multi-texture layer 2 since it isnt needed anymore
		OGL_DisableTexture2D();
#ifndef __vita__
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
#endif
		OGL_ActiveTextureUnit(GL_TEXTURE0);			// make sure #0 is active when we leave
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
#ifndef __vita__		
		glDisable(GL_TEXTURE_GEN_S);
		glDisable(GL_TEXTURE_GEN_T);
#endif
	}

}





/************************ DRAW MATERIAL **************************/

void MO_DrawMaterial(MOMaterialObject *matObj)
{
MOMaterialData		*matData;
OGLColorRGBA		*diffuseColor,diffColor2;
Boolean				alreadySet;
uint32_t				matFlags;


	matData = &matObj->objectData;									// point to material data

	if (matObj->objectHeader.cookie != MO_COOKIE)					// verify cookie
		DoFatalAlert("MO_DrawMaterial: bad cookie!");


				/****************************/
				/* SEE IF TEXTURED MATERIAL */
				/****************************/

	matFlags = matData->flags | gGlobalMaterialFlags;				// check flags of material & global

	if (matFlags & BG3D_MATERIALFLAG_TEXTURED)
	{
//		if (matData->setupInfo != gGameViewInfoPtr)						// make sure texture is loaded for this draw context
//			DoFatalAlert("MO_DrawMaterial: texture is not assigned to this draw context");


				/* ACTIVATE MATERIAL */

		alreadySet = (matObj == gMostRecentMaterial);
		if (alreadySet)												// see if even need to bother resetting this
		{
			OGL_EnableTexture2D();									// just make sure textures are on
		}
		else
		{
			OGL_Texture_SetOpenGLTexture(matData->textureName[0]);	// set this texture active
		}

				/*****************************/
				/* SET TEXTURE WRAPPING MODE */
				/*****************************/

						/* U */

		if (matFlags & BG3D_MATERIALFLAG_CLAMP_U)										// we want to clamp the U
		{
			if (!(matData->flags & BG3D_MATERIALFLAG_CLAMP_U_TRUE))						// see if clamping needs to be enabled
			{
			    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);	// nope, so set clamping
			    matData->flags |= BG3D_MATERIALFLAG_CLAMP_U_TRUE;						// and remember that we set it
			}
		}
		else																			// we DONT want to clamp U
		{
			if (matData->flags & BG3D_MATERIALFLAG_CLAMP_U_TRUE)						// see clamping is still enabled
			{
			    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			    matData->flags &= ~BG3D_MATERIALFLAG_CLAMP_U_TRUE;						// and remember that we cleared it
			}
		}

						/* V */

		if (matFlags & BG3D_MATERIALFLAG_CLAMP_V)										// we want to clamp the V
		{
			if (!(matData->flags & BG3D_MATERIALFLAG_CLAMP_V_TRUE))						// see if clamping needs to be enabled
			{
			    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);	// nope, so set clamping
			    matData->flags |= BG3D_MATERIALFLAG_CLAMP_V_TRUE;						// and remember that we set it
			}
		}
		else																			// we DONT want to clamp V
		{
			if (matData->flags & BG3D_MATERIALFLAG_CLAMP_V_TRUE)						// see clamping is still enabled
			{
			    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			    matData->flags &= ~BG3D_MATERIALFLAG_CLAMP_V_TRUE;						// and remember that we cleared it
			}
		}

	}
	else
		OGL_DisableTexture2D();						// not textured, so disable textures


			/********************/
			/* COLORED MATERIAL */
			/********************/

	diffuseColor = &matData->diffuseColor;			// point to diffuse color

	if (gGlobalTransparency != 1.0f)				// see if need to factor in global transparency
	{
		diffColor2.r = diffuseColor->r;
		diffColor2.g = diffuseColor->g;
		diffColor2.b = diffuseColor->b;
		diffColor2.a = diffuseColor->a * gGlobalTransparency;
	}
	else
		diffColor2 = *diffuseColor;					// copy to local so we can apply filter to it without munging original


			/* APPLY COLOR FILTER */

	diffColor2.r *= gGlobalColorFilter.r;
	diffColor2.g *= gGlobalColorFilter.g;
	diffColor2.b *= gGlobalColorFilter.b;

	OGL_SetColor4fv(&diffColor2);						// set current diffuse color


		/* SEE IF NEED TO ENABLE BLENDING */


	if ((diffColor2.a != 1.0f) || (matFlags & BG3D_MATERIALFLAG_ALWAYSBLEND))		// if has alpha, then we need blending on
		OGL_EnableBlend();
	else
		OGL_DisableBlend();


			/* SAVE THIS STUFF */

	gMostRecentMaterial = matObj;
}


/************************ DRAW MATRIX **************************/

void MO_DrawMatrix(const MOMatrixObject *matObj)
{
const OGLMatrix4x4		*m;

	m = &matObj->matrix;							// point to matrix

				/* MULTIPLY CURRENT MATRIX BY THIS */

	glMultMatrixf((GLfloat *)m);
}

/************************ MO: DRAW PICTURE **************************/

void MO_DrawPicture(const MOPictureObject *picObj)
{
const MOPictureData	*picData = &picObj->objectData;

	OGL_PushState();

			/* SET STATE */

	SetInfobarSpriteState(-5, 1);

			/* CENTER VERTICALLY */

	float width = 640.0f;
	float height = 480.0f;

	float x = -width/2;
	float y = -height/2;
	float z = 0;

	float currentAR = (1.0f/gCurrentPaneAspectRatio);
	float targetAR = 640.0f/480.0f;

	float scale = currentAR / targetAR;
	scale = GAME_CLAMP(scale, 1, 3);

	float yOffset = (scale-1) * 0.333f;		// apply small offset to keep nano within frame

	glTranslatef(-x, -y + yOffset*height, 0);
	glScalef(scale, scale, 1);

			/* ACTIVATE THE MATERIAL */

	MO_DrawMaterial(picData->material);		// submit material #0

			/* DRAW QUAD */

	glBegin(GL_QUADS);
	glTexCoord2f(0,1);	glVertex3f(x, y + height,z);
	glTexCoord2f(1,1);	glVertex3f(x + width, y + height,z);
	glTexCoord2f(1,0);	glVertex3f(x + width, y, z);
	glTexCoord2f(0,0);	glVertex3f(x, y, z);
	glEnd();

	gPolysThisFrame += 2;										// 2 more triangles

			/* RESTORE STATE */

	OGL_PopState();
}

/************************ MO: DRAW SPRITE **************************/
//
// Assume that the matrices are already set to identity
//
// Also, assume that the projection matrix is already the identity matrix.
//

void MO_DrawSprite(const MOSpriteObject *spriteObj)
{
const MOSpriteData	*spriteData = &spriteObj->objectData;
float			scaleX,scaleY,x,y;
MOMaterialObject	*mo;
float				aspect, xoff, yoff;
OGLPoint2D			p[4];

	x = spriteData->coord.x;
	y = spriteData->coord.y;

	scaleX = spriteData->scaleX;
	scaleY = spriteData->scaleY;

		/* ACTIVATE THE MATERIAL */

	MO_DrawMaterial(mo = spriteData->material);

	aspect = (float)mo->objectData.height / (float)mo->objectData.width;

		/* SET OFFSETS FOR CENTERED SPRITE */

	if (spriteData->drawCentered)
	{
		xoff = scaleX*.5f;
		yoff = (scaleY*aspect)*.5f;

		p[0].x = -xoff;		p[0].y = -yoff;				// set coords of quad
		p[1].x = xoff;		p[1].y = -yoff;
		p[2].x = xoff;		p[2].y = yoff;
		p[3].x = -xoff;		p[3].y = yoff;
	}


		/* SET OFFSETS FOR NON-CENTERED SPRITE */

	else
	{
		xoff = scaleX;
		yoff = scaleY * aspect;

		p[0].x = 0;			p[0].y = 0;				// set coords of quad
		p[1].x = xoff;		p[1].y = 0;
		p[2].x = xoff;		p[2].y = yoff;
		p[3].x = 0;			p[3].y = yoff;
	}

			/* APPLY ROTATION IF ANY */

	if (spriteData->rot != 0.0f)
	{
		OGLMatrix3x3		m;
		OGLMatrix3x3_SetRotate(&m, spriteData->rot);
		OGLPoint2D_TransformArray(p, &m, p, 4);
	}


			/* DRAW IT */

	glBegin(GL_QUADS);

	glTexCoord2f(0,0);	glVertex2f(p[0].x + x, p[0].y + y);
	glTexCoord2f(1,0);	glVertex2f(p[1].x + x, p[1].y + y);
	glTexCoord2f(1,1);	glVertex2f(p[2].x + x, p[2].y + y);
	glTexCoord2f(0,1);	glVertex2f(p[3].x + x, p[3].y + y);

	glEnd();



	gPolysThisFrame += 2;						// 2 more tris
}



#pragma mark -

/******************** MO GET NEW REFERENCE *********************/

MetaObjectPtr MO_GetNewReference(MetaObjectPtr mo)
{
MetaObjectHeader *h = mo;

	h->refCount++;

	return(mo);
}




/********************** MO DISPOSE OBJECT REFERENCE ******************************/
//
// NOTE:  	Groups and other objects are NOT sub-recursed.  When a group is de-referenced, only that group object is affected.
//

void MO_DisposeObjectReference(MetaObjectPtr obj)
{
MetaObjectHeader	*header = obj;
MOVertexArrayObject	*vObj;

	if (obj == nil)
		DoFatalAlert("MO_DisposeObjectReference: obj == nil");

	if (header->cookie != MO_COOKIE)					// verify cookie
		DoFatalAlert("MO_DisposeObjectReference: bad cookie!");


			/**************************************/
			/* DEC REFERENCE COUNT OF THIS OBJECT */
			/**************************************/

	header->refCount--;									// dec ref count

	if (header->refCount < 0)							// see if over did it
		DoFatalAlert("MO_DisposeObjectReference: refcount < 0!");

	if (header->refCount == 0)							// see if we can DELETE this node
	{
			/* NO MORE REFERENCES, SO DELETE DATA */

		switch(header->type)
		{
			case	MO_TYPE_GROUP:
					MO_DisposeObject_Group(obj);
					break;

			case	MO_TYPE_GEOMETRY:
					switch(header->subType)
					{
						case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
								vObj = obj;
								MO_DeleteObjectInfo_Geometry_VertexArray(&vObj->objectData);
								break;

						default:
								DoFatalAlert("MO_DisposeObject: unknown sub-type");
					}
					break;

			case	MO_TYPE_MATERIAL:
					MO_DeleteObjectInfo_Material(obj);
					break;

			case	MO_TYPE_PICTURE:
					MO_DeleteObjectInfo_Picture(obj);
					break;

			case	MO_TYPE_SPRITE:
					MO_DisposeObject_Sprite(obj);
					break;

		}

			/* DELETE THE OBJECT NODE */

		MO_DetachFromLinkedList(obj);					// detach from linked list

		header->cookie = 0xdeadbeef;					// devalidate cookie
		SafeDisposePtr(obj);								// free memory used by object
		return;
	}
}



/***************** DETACH FROM LINKED LIST *********************/

static void MO_DetachFromLinkedList(MetaObjectPtr obj)
{
MetaObjectHeader	*header = obj;
MetaObjectHeader	*prev,*next;


			/* VERIFY COOKIE */

	if (header->cookie != MO_COOKIE)
		DoFatalAlert("MO_DetachFromLinkedList: cookie is invalid!");


	prev = header->prevNode;
	next = header->nextNode;

			/* SEE IF WAS 1ST NODE IN LIST */

	if (prev == nil)
	{
		gFirstMetaObject = next;
		if (gFirstMetaObject)
			gFirstMetaObject->prevNode = nil;
	}

			/* SEE IF WAS LAST NODE IN LIST */

	if (next == nil)
	{
		gLastMetaObject = prev;
		if (gLastMetaObject)
			gLastMetaObject->nextNode = nil;
	}

			/* SOMEWHERE IN THE MIDDLE */

	else
	if (prev != nil)
	{
		prev->nextNode = next;
		next->prevNode = prev;
	}

	gNumMetaObjects--;

	if (gNumMetaObjects < 0)
		DoFatalAlert("MO_DetachFromLinkedList: counter mismatch");

	if (gNumMetaObjects == 0)
	{
		if (prev || next)							// if all gone, then prev & next should be nil
			DoFatalAlert("MO_DetachFromLinkedList: prev/next should be nil!");

		if (gFirstMetaObject != nil)
			DoFatalAlert("MO_DetachFromLinkedList: gFirstMetaObject should be nil!");

		if (gLastMetaObject != nil)
			DoFatalAlert("MO_DetachFromLinkedList: gLastMetaObject should be nil!");
	}
}


/**************** DISPOSE OBJECT:  GROUP ************************/
//
// Decrement the references of all objects in the group.
//

static void MO_DisposeObject_Group(MOGroupObject *group)
{
int	i,n;

	n = group->objectData.numObjectsInGroup;				// get # objects in group

	for (i = 0; i < n; i++)
	{
		MO_DisposeObjectReference(group->objectData.groupContents[i]);	// dispose of this object's ref
	}
}


/****************** DISPOSE OBJECT: SPRITE *******************/
//
// Decrement reference tothe material used in this sprite
//

static void MO_DisposeObject_Sprite(MOSpriteObject *obj)
{
MOSpriteData *data = &obj->objectData;

	MO_DisposeObjectReference(data->material);
}



/****************** DELETE OBJECT INFO: GEOMETRY : VERTEX ARRAY *******************/
//
// Assumes the contents (the materials) have already been dereferenced!
//

void MO_DeleteObjectInfo_Geometry_VertexArray(MOVertexArrayData *data)
{
int		i,n;
short	varType = data->VARtype;
Boolean	usingVAR = (varType != -1);					// were these arrays stored in VAR memory?

			/* DEREFERENCE ANY MATERIALS */

	n = data->numMaterials;
	for (i = 0; i < n; i++)
		MO_DisposeObjectReference(data->materials[i]);	// dispose of this object's ref


		/* DISPOSE OF VARIOUS ARRAYS */

	if (data->points)
	{
		if (usingVAR)
			OGL_FreeVertexArrayMemory(data->points, varType);
		else
			SafeDisposePtr(data->points);
		data->points = nil;
	}

	if (data->normals)
	{
		if (usingVAR)
			OGL_FreeVertexArrayMemory(data->normals, varType);
		else
			SafeDisposePtr(data->normals);
		data->normals = nil;
	}

	if (data->uvs[0])
	{
		if (usingVAR)
			OGL_FreeVertexArrayMemory(data->uvs[0], varType);
		else
			SafeDisposePtr(data->uvs[0]);
		data->uvs[0] = nil;

		if (data->numMaterials == 2)					// see if also nuke secondary uv list
		{
			if (data->uvs[1])
			{
				if (usingVAR)
					OGL_FreeVertexArrayMemory(data->uvs[1], varType);
				else
					SafeDisposePtr(data->uvs[1]);

				data->uvs[1] = nil;
			}
		}
	}

	if (data->colorsFloat)
	{
		if (usingVAR)
			OGL_FreeVertexArrayMemory(data->colorsFloat, varType);
		else
			SafeDisposePtr(data->colorsFloat);
		data->colorsFloat = nil;
	}

	if (data->triangles)
	{
		if (usingVAR)
			OGL_FreeVertexArrayMemory(data->triangles, varType);
		else
			SafeDisposePtr(data->triangles);
		data->triangles = nil;
	}
}



/****************** DELETE OBJECT INFO: MATERIAL *******************/

static void MO_DeleteObjectInfo_Material(MOMaterialObject *obj)
{
MOMaterialData		*data = &obj->objectData;

		/* DISPOSE OF TEXTURE NAMES */

	if (data->numMipmaps > 0)
		glDeleteTextures(data->numMipmaps, &data->textureName[0]);
}


/****************** DELETE OBJECT INFO: PICTURE *******************/

static void MO_DeleteObjectInfo_Picture(MOPictureObject *obj)
{
MOPictureData		*data = &obj->objectData;

				/* DEREFERENCE THE MATERIALS */

	MO_DisposeObjectReference(data->material);
	data->material = nil;
}


#pragma mark -

/******************** MO_DUPLICATE VERTEX ARRAY DATA *********************/
//
// Duplicates all of the data associated with a VertexArray definition.
// varType determines how we want to handle the new arrays.  If varType == -1 then we just
// allocate them in regular RAM.  Otherwise, we're using Vertex Array Range memory.
//

void MO_DuplicateVertexArrayData(MOVertexArrayData *inData, MOVertexArrayData *outData, short varType)
{
int			i,n,s;
Boolean		usingVAR = (varType != -1);

			/***********************************/
			/* GET NEW REFERENCES TO MATERIALS */
			/***********************************/

	outData->VARtype = varType;											// set new data's VAR type

	outData->numMaterials = n = inData->numMaterials;

	for (i = 0; i < n; i++)
	{
		MO_GetNewReference(inData->materials[i]);
		outData->materials[i] = inData->materials[i];
	}

			/************************/
			/* DUPLICATE THE ARRAYS */
			/************************/

			/* POINTS */

	n = outData->numPoints = inData->numPoints;
	s = inData->numPoints * sizeof(OGLPoint3D);

	if (inData->points)
	{
		if (usingVAR)
			outData->points = OGL_AllocVertexArrayMemory(s, varType);
		else
			outData->points = AllocPtr(s);

		BlockMove(inData->points, outData->points, s);
	}
	else
		outData->points = nil;


			/* NORMALS */

	s = n * sizeof(OGLVector3D);

	if (inData->normals)
	{
		if (usingVAR)
			outData->normals = OGL_AllocVertexArrayMemory(s, varType);
		else
			outData->normals = AllocPtr(s);

		BlockMove(inData->normals, outData->normals, s);
	}
	else
		outData->normals = nil;


			/* UVS */

	s = n * sizeof(OGLTextureCoord);

	if (inData->uvs[0])
	{
		if (usingVAR)
			outData->uvs[0] = OGL_AllocVertexArrayMemory(s, varType);
		else
			outData->uvs[0] = AllocPtr(s);

		BlockMove(inData->uvs[0], outData->uvs[0], s);
	}
	else
		outData->uvs[0] = nil;



			/* COLORS FLOAT */

	s = n * sizeof(OGLColorRGBA);

	if (inData->colorsFloat)
	{
		if (usingVAR)
			outData->colorsFloat = OGL_AllocVertexArrayMemory(s, varType);
		else
			outData->colorsFloat = AllocPtr(s);

		BlockMove(inData->colorsFloat, outData->colorsFloat, s);
	}
	else
		outData->colorsFloat = nil;


			/* TRIANGLES */

	n = outData->numTriangles = inData->numTriangles;
	s = n * sizeof(GLint) * 3;

	if (inData->triangles)
	{
		if (usingVAR)
			outData->triangles = OGL_AllocVertexArrayMemory(s, varType);
		else
			outData->triangles = AllocPtr(s);

		BlockMove(inData->triangles, outData->triangles, s);
	}
	else
		outData->triangles = nil;
}


#pragma mark -


/******************** MO: CALC BOUNDING BOX ***********************/
//
// INPUT:
//			m = transform matrix to apply to verts or nil.
//

void MO_CalcBoundingBox(MetaObjectPtr object, OGLBoundingBox *bBox, OGLMatrix4x4 *m)
{
		/* INIT BBOX TO BOGUS VALUES */

	bBox->min.x =
	bBox->min.y =
	bBox->min.z = 100000000;

	bBox->max.x =
	bBox->max.y =
	bBox->max.z = -bBox->min.x;

	bBox->isEmpty = false;

			/* RECURSIVELY CALC BBOX */

	MO_CalcBoundingBox_Recurse(object, bBox, m);
}


/******************** MO: CALC BOUNDING BOX: RECURSE ***********************/

static void MO_CalcBoundingBox_Recurse(MetaObjectPtr object, OGLBoundingBox *bBox, const OGLMatrix4x4 *m)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayObject	*vObj;
MOGroupObject		*groupObject;
MOVertexArrayData	*geoData;
int					numChildren,i,numPoints;
float				x,y,z;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("MO_CalcBoundingBox_Recurse: cookie is invalid!");


	switch(objHead->type)
	{
			/* CALC BBOX OF GEOMETRY */

		case	MO_TYPE_GEOMETRY:
				switch(objHead->subType)
				{
					case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
							vObj = object;
							geoData = &vObj->objectData;
							numPoints = geoData->numPoints;

								/* TRANSFORM POINTS */

							if (m)
							{
								static OGLPoint3D tpoints[1000];

								if (numPoints > 1000)					// make sure not overflowing buffer
								{
									DoFatalAlert("MO_CalcBoundingBox_Recurse: buffer overflow!");
									return;
								}

								OGLPoint3D_TransformArray(geoData->points, m, tpoints, numPoints);
								for (i = 0; i < numPoints; i++)
								{
									x = tpoints[i].x;
									y = tpoints[i].y;
									z = tpoints[i].z;

									if (x < bBox->min.x)
										bBox->min.x = x;
									if (x > bBox->max.x)
										bBox->max.x = x;

									if (y < bBox->min.y)
										bBox->min.y = y;
									if (y > bBox->max.y)
										bBox->max.y = y;

									if (z < bBox->min.z)
										bBox->min.z = z;
									if (z > bBox->max.z)
										bBox->max.z = z;
								}

							}

								/* NO TRANSFORM, USE RAW DATA */

							else
							{
								for (i = 0; i < numPoints; i++)
								{
									x = geoData->points[i].x;
									y = geoData->points[i].y;
									z = geoData->points[i].z;

									if (x < bBox->min.x)
										bBox->min.x = x;
									if (x > bBox->max.x)
										bBox->max.x = x;

									if (y < bBox->min.y)
										bBox->min.y = y;
									if (y > bBox->max.y)
										bBox->max.y = y;

									if (z < bBox->min.z)
										bBox->min.z = z;
									if (z > bBox->max.z)
										bBox->max.z = z;
								}
							}
							break;

					default:
						DoFatalAlert("MO_CalcBoundingBox_Recurse: unknown sub-type!");
				}
				break;


			/* RECURSE THRU GROUP */

		case	MO_TYPE_GROUP:
				groupObject = object;
				numChildren = groupObject->objectData.numObjectsInGroup;
				for (i = 0; i < numChildren; i++)
					MO_CalcBoundingBox_Recurse(groupObject->objectData.groupContents[i], bBox, m);
				break;


		case	MO_TYPE_MATRIX:
				DoFatalAlert("MO_CalcBoundingBox_Recurse: why is there a matrix here?");
				break;
	}
}


/******************** MO: CALC BOUNDING SPHERE ***********************/

void MO_CalcBoundingSphere(MetaObjectPtr object, float *bSphere)
{
	*bSphere = 0;

			/* RECURSIVELY CALC SPHERE */

	MO_CalcBoundingSphere_Recurse(object, bSphere);
}


/******************** MO: CALC BOUNDING SPHERE: RECURSE ***********************/

static void MO_CalcBoundingSphere_Recurse(MetaObjectPtr object, float *bSphere)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayObject	*vObj;
MOGroupObject		*groupObject;
MOVertexArrayData	*geoData;
int					numChildren,i,numPoints;
float				d;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("MO_CalcBoundingSphere_Recurse: cookie is invalid!");


	switch(objHead->type)
	{
			/* CALC BBOX OF GEOMETRY */

		case	MO_TYPE_GEOMETRY:
				switch(objHead->subType)
				{
					case	MO_GEOMETRY_SUBTYPE_VERTEXARRAY:
							vObj = object;
							geoData = &vObj->objectData;
							numPoints = geoData->numPoints;

							for (i = 0; i < numPoints; i++)
							{
								d = CalcVectorLength((OGLVector3D *)&geoData->points[i]);		// calc this radius
								if (d > *bSphere)								// is this the best?
									*bSphere = d;
							}
							break;

					default:
						DoFatalAlert("MO_CalcBoundingSphere_Recurse: unknown sub-type!");
				}
				break;


			/* RECURSE THRU GROUP */

		case	MO_TYPE_GROUP:
				groupObject = object;
				numChildren = groupObject->objectData.numObjectsInGroup;
				for (i = 0; i < numChildren; i++)
					MO_CalcBoundingSphere_Recurse(groupObject->objectData.groupContents[i], bSphere);
				break;


		case	MO_TYPE_MATRIX:
				DoFatalAlert("MO_CalcBoundingSphere_Recurse: why is there a matrix here?");
				break;
	}
}



#pragma mark -

/******************* MO: OBJECT OFFSET UVS ************************/

void MO_Object_OffsetUVs(MetaObjectPtr object, float du, float dv)
{
MetaObjectHeader	*objHead = object;
MOGroupData			*data;
int					i;
MOGroupObject		*group;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("MO_Group_OffsetUVs: cookie is invalid!");


			/* HANDLE IT */

	switch(objHead->type)
	{
		case	MO_TYPE_GEOMETRY:
				MO_VertexArray_OffsetUVs(object, du, dv);
				break;

		case	MO_TYPE_GROUP:
				group = object;
				data = &group->objectData;

							/* PARSE OBJECTS IN GROUP */

				for (i = 0; i < data->numObjectsInGroup; i++)
				{
					switch(data->groupContents[i]->type)
					{
						case	MO_TYPE_GEOMETRY:
								MO_VertexArray_OffsetUVs(data->groupContents[i], du, dv);
								break;

						case	MO_TYPE_GROUP:
								MO_Object_OffsetUVs(data->groupContents[i], du, dv);		// recurse this sub-group
								break;

					}
				}
				break;


		default:
			DoFatalAlert("MO_Group_OffsetUVs: object type is not supported.");
	}

}


/******************* MO: VERTEX ARRAY, OFFSET UVS ************************/

void MO_VertexArray_OffsetUVs(MetaObjectPtr object, float du, float dv)
{
MetaObjectHeader	*objHead = object;
MOVertexArrayData	*data;
int					numPoints;
OGLTextureCoord		*uvPtr;
MOVertexArrayObject	*vObj;

			/* VERIFY COOKIE */

	if (objHead->cookie != MO_COOKIE)
		DoFatalAlert("MO_VertexArray_OffsetUVs: cookie is invalid!");


		/* MAKE SURE IT IS A VERTEX ARRAY */

	if ((objHead->type != MO_TYPE_GEOMETRY) || (objHead->subType != MO_GEOMETRY_SUBTYPE_VERTEXARRAY))
		DoFatalAlert("MO_VertexArray_OffsetUVs: object is not a Vertex Array!");

	vObj = object;
	data = &vObj->objectData;						// point to data


	uvPtr = data->uvs[0];								// point to uv list
	if (uvPtr == nil)
		return;

	numPoints = data->numPoints;					// get # points

			/* OFFSET THE UV'S */

	for (int i = 0; i < numPoints; i++)
	{
		uvPtr[i].u += du;
		uvPtr[i].v += dv;
	}

			/* THIS UPDATE WILL CAUSE US TO UPDATE THE VAR IF IT'S USED */

	OGL_SetVertexArrayRangeDirty(data->VARtype);
}


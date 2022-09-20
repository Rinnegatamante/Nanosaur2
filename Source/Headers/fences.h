//
// fences.h
//

#ifndef FENCE_H
#define FENCE_H

#define MAX_FENCES			90
#define	MAX_NUBS_IN_FENCE	80


typedef struct
{
	float	top,bottom,left,right;
}RectF;

typedef struct
{
	u_short			type;				// type of fence
	short			numNubs;			// # nubs in fence
	OGLPoint3D		*nubList;			// pointer to nub list
	OGLBoundingBox	bBox;				// bounding box of fence area
	OGLVector2D		*sectionVectors;	// for each section/span, this is the vector from nub(n) to nub(n+1)
	OGLVector2D		*sectionNormals;	// for each section/span, this is the perpendicular normal vector
}FenceDefType;



		/* EXTERNS */

extern	long				gNumFences;
extern	FenceDefType		*gFenceList;
extern	MOVertexArrayData	gFenceTriMeshData[MAX_FENCES][2];


//============================================

void PrimeFences(void);
void UpdateFences(void);
Boolean DoFenceCollision(ObjNode *theNode);
void DisposeFences(void);
Boolean SeeIfLineSegmentHitsFence(const OGLPoint3D *endPoint1, const OGLPoint3D *endPoint2, OGLPoint3D *intersect, Boolean *overTop, float *fenceTopY);


#endif



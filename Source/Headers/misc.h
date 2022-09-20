//
// misc.h
//

#ifndef __MISC
#define __MISC

#define SERIAL_LENGTH      12


extern	void ShowSystemErr(long err);
extern void	DoAlert(Str255);
extern void	DoFatalAlert(Str255);
extern unsigned char	*NumToHex(unsigned short);
extern unsigned char	*NumToHex2(unsigned long, short);
extern unsigned char	*NumToDec(unsigned long);
extern	void CleanQuit(void);
extern	void SetMyRandomSeed(unsigned long seed);
extern	unsigned long MyRandomLong(void);
extern	void FloatToString(float num, Str255 string);
extern	Handle	AllocHandle(long size);
extern	void *AllocPtr(long size);
void *AllocPtrClear(long size);
extern	void PStringToC(char *pString, char *cString);
float StringToFloat(Str255 textStr);
extern	void DrawCString(char *string);
extern	void InitMyRandomSeed(void);
extern	void VerifySystem(void);
extern	float RandomFloat(void);
u_short	RandomRange(unsigned short min, unsigned short max);
extern	void RegulateSpeed(short fps);
extern	void CopyPStr(ConstStr255Param	inSourceStr, StringPtr	outDestStr);
extern	void ShowSystemErr_NonFatal(long err);
void DoAlertNum(int n);
void CalcFramesPerSecond(void);
Boolean IsPowerOf2(int num);
float RandomFloat2(void);

void CopyPString(Str255 from, Str255 to);

void SafeDisposePtr(void *ptr);
void MyFlushEvents(void);

StringPtr PStrCat(StringPtr dst, ConstStr255Param   src);
StringPtr PStrCopy(StringPtr dst, ConstStr255Param   src);



Boolean WeAreFrontProcess(void);
OSErr LoadOurPrefs(void);
void SaveOurPrefs(void);

short SwizzleShort(short *shortPtr);
long SwizzleLong(long *longPtr);
float SwizzleFloat(float *floatPtr);
u_long SwizzleULong(u_long *longPtr);
u_short SwizzleUShort(u_short *shortPtr);


/***********************************/


extern	float		gFramesPerSecondFrac;
extern	Boolean		gGameIsRegistered, gAltivec, gLittleSnitch, gLowRAM;
extern	Str255  	gSerialFileName;
extern  u_long		gSerialWasVerifiedMode;

#endif



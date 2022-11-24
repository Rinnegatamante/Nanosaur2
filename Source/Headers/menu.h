#pragma once

#include "localization.h"

#define MAX_MENU_CYCLER_CHOICES		20

typedef enum
{
	kMISENTINEL,
	kMIPick,
	kMILabel,
	kMISpacer,
	kMICycler1,
	kMICycler2,
	kMIFloatRange,
	kMIKeyBinding,
	kMIPadBinding,
	kMIMouseBinding,
	kMI_COUNT
} MenuItemType;

typedef struct
{
	Byte*			valuePtr;

	bool			isDynamicallyGenerated;

	union
	{
		struct
		{
			LocStrID	text;
			uint8_t		value;
		} choices[MAX_MENU_CYCLER_CHOICES];

		struct
		{
			int				(*generateNumChoices)(void);
			const char*		(*generateChoiceString)(Byte value);
		} generator;
	};
} MenuCyclerData;

typedef struct
{
	float*			valuePtr;
	const float*	equilibriumPtr;
	float			incrementFrac;
	float			xSpread;
} MenuFloatRangeData;

typedef struct MenuItem
{
	MenuItemType			type;
	LocStrID				text;
	const char*				rawText;
	int32_t					id;			// value stored in gMenuOutcome when exiting menu
	int32_t					next;		// next menu, or one of 'EXIT', 'BACK' or 0 (no-op)

	void					(*callback)(void);
	int						(*getLayoutFlags)(void);

	union
	{
		int 				inputNeed;
		MenuCyclerData		cycler;
		MenuFloatRangeData	floatRange;
	};

	float					customHeight;
} MenuItem;

enum	// Returned by getLayoutFlags
{
	kMILayoutFlagDisabled = 1 << 0,
	kMILayoutFlagHidden = 1 << 1,
};

typedef struct MenuStyle
{
	float			darkenPaneOpacity;
	float			fadeInSpeed;		// Menu will ignore input during 1.0/fadeInSpeed seconds after booting
	float			fadeOutSpeed;
	float			standardScale;
	float			rowHeight;
	float			uniformXExtent;
	short			textSlot;
	float			yOffset;
	int				fontAtlas;

	OGLColorRGBA	labelColor;
	OGLColorRGBA	idleColor;
	OGLColorRGBA	highlightColor;
	OGLColorRGBA	arrowColor;
	struct
	{
		bool			asyncFadeOut : 1;
		bool			fadeOutSceneOnExit : 1;
		bool			startButtonExits : 1;
		bool			isInteractive : 1;
		bool			canBackOutOfRootMenu : 1;
	};

	void			(*exitCall)(int);
} MenuStyle;

extern const MenuStyle kDefaultMenuStyle;

void RegisterMenu(const MenuItem* menus);

ObjNode* MakeMenu(const MenuItem* menu, const MenuStyle* style);

void LayoutCurrentMenuAgain(void);
int GetCurrentMenu(void);
int GetCurrentMenuItemID(void);
bool IsMenuMouseControlled(void);
ObjNode* GetCurrentMenuItemObject(void);
float GetMenuIdleTime(void);
void KillMenu(int returnCode);
bool IsMenuTreeEndSentinel(const MenuItem* menuItem);

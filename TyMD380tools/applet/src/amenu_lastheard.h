// File:    md380tools/applet/src/amenu_codeplug.h
// Purpose:
//  Helper functions to show parts of the codeplug in the 'application menu'
//         (app_menu.c), for example:
//  - a fast-scrolling ZONE list,
//  - a fast-scrolling CONTACTS list,
//  - and maybe more in future .
// Details in the implementation - see amenu_codeplug.c .

//extern int ZoneList_nEntries; // number of entries in the zone list.
//extern int ZoneList_iCurrent; // index of the currently selected (active) zone.

//extern uint8_t channel_num; // <- belongs to THE ORIGINAL FIRMWARE (1..16, 0 forces "reload")


extern int am_cbk_LastheardList(app_menu_t *pMenu, menu_item_t *pItem, int event, int param );


extern void LHList_AddEntry(uint32_t src, uint32_t dst);


#ifdef __cplusplus
extern "C" {
#endif


#define LH_FLAG_ACTIVE     1
#define LH_FLAG_PRIV       2
#define LH_FLAG_STATUS     4


	typedef struct
	{
		uint8_t lstIndex;
		uint8_t flags;
		uint32_t src;
		uint32_t dst;
		char timestamp[0x10];
		user_t dbEntry;


	} lastheard_user;


#ifdef __cplusplus
}
#endif
// File:    md380tools/applet/src/amenu_codeplug.h
// Purpose:
//  Helper functions to show parts of the codeplug in the 'application menu'
//         (app_menu.c), for example:
//  - a fast-scrolling ZONE list,
//  - a fast-scrolling CONTACTS list,
//  - and maybe more in future .
// Details in the implementation - see amenu_codeplug.c .

extern uint8_t channel_num; // <- belongs to THE ORIGINAL FIRMWARE (1..16, 0 forces "reload")


extern int am_cbk_ZoneList(app_menu_t *pMenu, menu_item_t *pItem, int event, int param );
extern int overwriteChannel(uint16_t channelIndex);


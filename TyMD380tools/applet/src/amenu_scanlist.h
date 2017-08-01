
// Details in the implementation - see amenu_scanlist.c .

#define CODEPLUG_SPIFLASH_ADDR_SCANLIST 0x00018860   
#define CODEPLUG_SIZEOF_SCANLIST_ENTRY 104
#define CODEPLUG_MAX_SCANLIST   250
#define CODEPLUG_MAX_SCANLIST_CHANNELS 30

typedef struct
{
	wchar_t name[16];
	uint16_t primary;
	uint16_t secondary;
	uint16_t txchan;
	uint8_t unk1; //0xFF
	uint8_t holdtime;
	uint8_t sampletime;
	uint8_t unk2; //0xFF
	uint16_t channels[CODEPLUG_MAX_SCANLIST_CHANNELS];
} scanlist_t;



extern int am_cbk_ScanList(app_menu_t *pMenu, menu_item_t *pItem, int event, int param);
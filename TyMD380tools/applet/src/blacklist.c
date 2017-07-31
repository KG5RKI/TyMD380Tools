
#include <stdint.h>
#include "syslog.h"

//extern const int BLACKLIST_LENGTH;

uint32_t idBlackListNum = 0;
uint32_t idBlackList[0x10];

void blockID(uint32_t id) 
{
	if (idBlackListNum < 0x10 - 1) {
		idBlackList[idBlackListNum++] = id;
		//syslog_printf("\rBlacklist: %d %d\r", idBlackList[idBlackListNum - 1], idBlackListNum);
	}
}

int isBlackListed(uint32_t id)
{
	for (int i = 0; i < idBlackListNum; i++) {
		if (idBlackList[i] == id) {
			//syslog_printf("\r%d %d - %d\r", i, idBlackList[i], id);
			return 1;
		}
	}
	return 0;
}

#include <stdint.h>

#ifndef BLACKLIST_H
#define BLACKLIST_H

#ifdef __cplusplus
extern "C" {
#endif

	//const int BLACKLIST_LENGTH = 0x10;

	extern uint32_t idBlackList[0x10];
	extern uint32_t idBlackListNum;
	extern int isBlackListed(uint32_t id);
	extern void blockID(uint32_t id);

#ifdef __cplusplus
}
#endif

#endif /* CODEPLUG_H */


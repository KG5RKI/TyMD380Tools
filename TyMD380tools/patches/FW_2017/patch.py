#!/usr/bin/env python2
# -*- coding: utf-8 -*-

# Vocoder Patch for MD380 Firmware
# Applies to version D013.020

from Patcher import Patcher

# Match all public calls.
monitormode = False
# Match all private calls.
monitormodeprivate = True

if __name__ == '__main__':
    print("Creating patches from unwrapped.img.")
    patcher = Patcher("unwrapped.img")
	
	#test gps
    #patcher.nopout((0x800C278 + 0))
    #patcher.nopout((0x800C278 + 2))
	

    # bypass vocoder copy protection on D013.020

    #patcher.nopout((0x08011444))
    #patcher.nopout((0x08011444) + 0x2)
	
	#test manual dial group callable
    #patcher.sethword(0x08023170, 0x2204)
    #patcher.sethword(0x08012912, 0x2804)
    #patcher.sethword(0x080EB1B0, 0x00FF)
    patcher.nopout((0x08028154))
    patcher.nopout((0x08028154 + 0x2))
    patcher.nopout((0x08028154 + 0x4))
    patcher.nopout((0x08028154 + 0x6))
    patcher.nopout((0x08028154 + 0x8))
    patcher.nopout((0x08028154 + 0xA))
	
    # freeing ~200k for code patches
    patcher.ffrange(0x0809aee8, 0x080cf754)

    # This mirrors the RESET vector to 0x080C020, for use in booting.
    patcher.setword(0x0800C020,
                    patcher.getword(0x0800C004),
                    0x00000000)

    # This makes RESET point to our stub below.
    patcher.setword(0x0800C004,
                    0x0809af00 + 1)

    # This stub calls the target RESET vector,
    # if it's not FFFFFFFF.
    patcher.sethword(0x0809af00, 0x4840)
    patcher.sethword(0x0809af02, 0x2100)
    patcher.sethword(0x0809af04, 0x3901)
    patcher.sethword(0x0809af06, 0x4508)
    patcher.sethword(0x0809af08, 0xd100)
    patcher.sethword(0x0809af0a, 0x483c)
    patcher.sethword(0x0809af0c, 0x4700)

    # Stores the RESET handler for our stub.
    patcher.setword(0x0809affc, patcher.getword(0x0800C020), 0xFFFFFFFF)
	
    patcher.export("patched.img")

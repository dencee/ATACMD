ATACMD
======

Low-level ATA/SATA hard drive diagnostic tool.

ATACMD.exe - CLI diagnostic tool. Comamnds are listed in wtAtacmdCommands.
// The purpose of this program is to act as a diagnostic tool for ATA disk
// drives. This program differs from other HDD diagnostic tools in that it has
// the ability to issue commands at the command block register level. This
// means the program allows the user to send command codes defined by the ATA
// standard (read, writes, etc.) as well as those that are designated as
// reserved/unused/vendor by the ATA standard. Of course those undefined
// command codes have undefined behavior, so if you decide to send those
// commands, you do so at your own risk, as stated above.

ATAErase.exe - Overwrite all data on each attached hard drive.
Same kind of erase done in HDDErase: https://en.wikipedia.org/wiki/HDDerase

ATATest.exe - Issue all commands, even potential vendor commands
// This program attempts to issue every single ATA command to the drive and log
// the input and output command block registers. Some command codes that are
// either ATA reserved or vendor-allocated will sometimes produce bizzare
// results, hangs, etc. when issued. Since these commands are undocumented by
// the ATA standard, their behavior is unknown.  Therefore, I recommend to only
// use this program on backup/test drives, i.e. drives that don't have data you
// care about!

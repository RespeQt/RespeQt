AspeQt, Atari Serial Peripheral Emulator for Qt

Summary
=======
AspeQt emulates Atari SIO peripherals when connected to an Atari 8-bit computer with an SIO2PC cable.
In that respect it's similar to programs like APE and Atari810. The main difference is that it's free
(unlike APE) and it's cross-platform (unlike Atari810 and APE).

Some features
=============
* Qt based GUI with drag and drop support.
* Cross-platform (currently Windows and x86-Linux)
* 15 disk drive emulation (drives 9-15 are only supported by SpartaDOS X)
* Text-only printer emulation with saving and printing of the output
* Cassette image playback
* Folders can be mounted as simulated Dos20s disks. (read-only, now with SDX compatibility, and bootable)
* Atari executables can be booted directly, optionally with high speed.
* Contents of image files can be viewed / changed
* AspeQt Client module (ASPECL.COM). Runs on the Atari and is used to get/set Date/Time on the Atari plus a variety of other remote tasks.
* Upto 6xSIO speed and more if the serial port adaptor supports it (FTDI chip based cables are recommanded).
* Localization support
* Multi-session support

License (see license.txt file for more details)
===============================================
Original code up to version 0.6.0 Copyright 2009 by Fatih Aygün. 
Updates since v0.6.0 Copyright 2012 by Ray Ataergin.

You can freely copy, use, modify and distribute it under the GPL 2.0
license. Please see license.txt for details.

Qt libraries: Copyright 2009 Nokia Corporation and/or its subsidiary(-ies).
Used in this package under LGPL 2.0 license.

Silk Icons: Copyright by Mark James (famfamfam.com).
Used in this package under Creative Commons Attribution 3.0 license.

Additional Icons by Oxygen Team. Used in this package under Creative Commons Attribution-ShareAlike 3.0 license.

AtariSIO Linux kernel module and high speed code used in the EXE loader.
Copyright Matthias Reichl <hias@horus.com>. Used in this package under GPL 2.0 license.

Atascii Fonts by Mark Simonson (http://members.bitstream.net/~marksim/atarimac).
Used in this package under Freeware License.


Contact
=======
Ray Ataergin, mail to ray@atari8warez.com. Please include the word "aspeqt" in the subject field.
Fatih Aygun, mail to cyco130@yahoo.com, Please include the word "aspeqt" in the subject field.

For SIO2PC/10502PC hardware purchase/support visit www.atari8warez.com

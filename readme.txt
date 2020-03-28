RespeQt, Atari Serial Peripheral Emulator for Qt

Summary
=======
RespeQt emulates Atari SIO peripherals when connected to an Atari 8-bit computer with an SIO2PC cable.
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
* RespeQt Client module RCL.COM. Runs on the Atari and is used to get/set Date/Time on the Atari plus a variety of other remote tasks.
* ApeTime client support.
* Upto 6xSIO speed and more if the serial port adaptor supports it (FTDI chip based cables are recommanded).
* Localization support (Currently for English, German, Polish, Russian, Slovak, Spanish and Turkish). ***this may be broken***
* Multi-session support
* PRO and ATX images support in read/write mode with accurate (I hope so) protection emulation.
* Spy mode to display data exchanged with Atari.
* Happy 810 Rev.5 and Happy 1050 Rev.7 emulation to read/write real floppy disks from/to images.
* Chip 810 and Super Archiver 1050 emulation to read/write real floppy disks from/to images.
* Automatic Translator activation when OS-B is detected in the filename in drive D1:
* Favorite tool disk accessible in one click in drive D1:

License (see license.txt file for more details)
===============================================
RespeQt fork Copyright 2015,2016 by Joseph Zatarski, Copyright 2015 DrVenkman, and Copyright 2016 TheMontezuma, Copyright 2016, 2017 by Jochen Schäfer (josch1710), and Copyright 2017 by blind.
RespeQt enhancements Copyright 2018 by ebiguy.
RespeQt is based on AspeQt 1.0.0-preview7
Original AspeQt code up to version 0.6.0 Copyright 2009 by Fatih Aygün. 
Updates to AspeQt since v0.6.0 to 1.0.0-preview7 Copyright 2012 by Ray Ataergin.

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

1020 emu font by UNIXcoffee928. Used with permission under GPL 2.0 license.

RespeQt icon by djmat56. Used with permission.

Support for the spanish language by AsCrNet.

RespeQt Client program (rcl.com) Copyright FlashJazzCat. Used under the GPL v2.

DOS files distributed with RespeQt are copyright of their respective owners. Joseph Zatarski and RespeQt distributes those files with the understanding that they are either abandonware or public domain, and are widely available for download through the internet. If you are the copyright holder of one or more of these files, and believe that distribution of these files constitutes a breach of your rights please contact ebiguy on AtariAge. We respect the rights of copyright holders and won't distribute copyrighted work without the rights holder's consent.

Special Thanks
==============
Kyle22 for finding 2 instances of AspeQt, 1 reference to Ray Ataergin, 1 reference to Ray's website which needed to be changed, for finding that the about and user documentation dialogs were out of date (way out of date), contact information in readme.txt was out of date, and for pointing out that I forgot to update the documentation for r1.

FlashJazzCat for writing a replacement for AspeCL.

Contact
=======
Please include RespeQt in the subject field when emailing

Maintainer - ebiguy on AtariAge

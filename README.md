ps3mca-ps1-driver
===

this is a libusb driver to communicate with the PlayStation 3 Memory Card Adaptor CECHZM1 (SCPH-98042) for PS1 cards (SCPH-1020, SCPH-1170 and SCPH-119X), maybe the PocketStation (SCPH-4000) and maybe other cards or device (like the MEMORY DISK DRIVE).


**Requirements**

* libusb (on MacOSX perhaps via [Homebrew](http://mxcl.github.io/homebrew/): `brew install libusb`)

**Compiling**

gcc main.c `pkg-config --libs --cflags libusb-1.0`


**Usage**

"a.out r" for reading
"a.out w" for writing

**Supported file**

All pure (raw) image of memory card:
.psm,
.ps,
.ddf,
.mcr,
.mcd,
.mc...

Not supported:
Connectix Virtual Game Station format (.MEM): "VgsM", 64 bytes
PlayStation Magazine format (.PSX): "PSV", 256 bytes
Virtual Memory Card PS1 (.VM1): VM1 is a PS1 memory card in "PS3 format", used in PS3 internal HDD only.

**Author**

Written by [Paolo Caroni](kenren89@gmail.com) and released under the terms of the GNU GPL, version 3, or later.

**License**

This program is free software: you can redistribute it and/or modify it under the terms  of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.
If not, see <http://www.gnu.org/licenses/>.


**Tested**
Reading 90% with some errors in the odd frame.
Writing 90% with some errors in the odd frame.


**Other similar but unrelated projects**

ps3-memorycard-adapter is a python tool that read and write PS1 and PS2 card.
Licence: GPLv2
<https://github.com/vpelletier/ps3-memorycard-adapter>


ps3mca-tool is a C program to read and write PS2 memory card.
Licence: GPLv3+
<https://github.com/jimmikaelkael/ps3mca-tool>
--WARNING--
this project is removed on github because (probably) violate software patent.
see:
<https://github.com/github/dmca/blob/master/2011/2011-06-21-sony.markdown>
If you live on U.S.A. DON'T download it.
In that confederation of states it is illegal (DMCA).
If you live in any other states:
<http://www.mirrorcreator.com/files/XVFKCQ6R/jimmikaelkael-ps3mca-tool-12a198f.zip_links>



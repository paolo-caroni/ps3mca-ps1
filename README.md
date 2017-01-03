# ps3mca-ps1-driver
===

this is a libusb driver to communicate with the PlayStation 3 Memory Card Adaptor CECHZM1 (SCPH-98042) for PS1 cards (SCPH-1020, SCPH-1170 and SCPH-119X), maybe the PocketStation (SCPH-4000) and maybe other cards or device (like the MEMORY DISK DRIVE).


**Requirements**

* libusb;
* PlayStation 3 Memory Card Adaptor CECHZM1 (SCPH-98042) or similar;
* PS1 memory card, PocketStation and maybe other cards or device (like the MEMORY DISK DRIVE).

**Compiling**

gcc main.c `pkg-config --libs --cflags libusb-1.0`


**Usage**

"a.out v" for verify what type of card is (PS1 or PS2).<br>
"a.out g" for verify if is a original card. Some known bug (see doc/FAQ).<br>
"a.out p" for verify if is a original PocketStation card.<br>
"a.out r" for reading.<br>
"a.out w" for writing (WARNING need a write.mcd file), unstable, use at your own risk.<br>

**Supported file**

All pure (raw) image of memory card:<br>
*.psm, *.ps, *.ddf, *.mcr, *.mcd, *.mc...<br>

Not supported:<br>
Connectix Virtual Game Station format (.MEM): "VgsM", 64 bytes.<br>
PlayStation Magazine format (.PSX): "PSV", 256 bytes.<br>
Virtual Memory Card PS1 (.VM1): VM1 is a PS1 memory card in "PS3 format", used in PS3 internal HDD only.<br>

**Author**

Written by [Paolo Caroni](kenren89@gmail.com) and released under the terms of the GNU GPL, version 3, or later.

**License**

This program is free software: you can redistribute it and/or modify it under the terms  of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.<br>
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with this program.<br>
If not, see <http://www.gnu.org/licenses/>.


**Tested**

Reading 90% with some errors in the odd frame.<br>
Writing 90% with some errors in the odd frame, unreadable on PS<br>


**Documentation**

PS1 Memory Card
<http://problemkaputt.de/psx-spx.htm#memorycardreadwritecommands><br>


**Other similar but unrelated projects**

ps3-memorycard-adapter is a python tool that read and write PS1 and PS2 card.<br>
Licence: GPLv2<br>
<https://github.com/vpelletier/ps3-memorycard-adapter><br>


ps3mca-tool is a C program to read and write PS2 memory card.<br>
Licence: GPLv3+<br>
<https://github.com/jimmikaelkael/ps3mca-tool><br>
--WARNING--<br>
this project is removed on github because (probably) violate software patent.<br>
see:<br>
<https://github.com/github/dmca/blob/master/2011/2011-06-21-sony.markdown><br>
If you live on U.S.A. DON'T download it.<br>
In that confederation of states it is illegal (DMCA).<br>
If you live in any other states:<br>
<http://www.mirrorcreator.com/files/XVFKCQ6R/jimmikaelkael-ps3mca-tool-12a198f.zip_links><br>





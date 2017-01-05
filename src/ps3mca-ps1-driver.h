/*
 * this is a libusb program to communicate with the PlayStation 3 Memory Card Adaptor CECHZM1 (SCPH-98042) for PS1 cards (SCPH-1020,
 * SCPH-1170 and SCPH-119X), maybe the PocketStation (SCPH-4000) and maybe other cards or device (like the MEMORY DISK DRIVE).
 *
 * Copyright (C) 2017 Paolo Caroni <kenren89@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 */


/* -------------------------------------------------PS3mca definitions and protocol-------------------------------------------------*/
/* Define the USB device (CECHZM1)*/
uint16_t USB_VENDOR =				0x054c;	/* Sony Corp.*/
uint16_t USB_PRODUCT =				0x02ea;	/* PlayStation 3 Memory Card Adaptor*/
unsigned int USB_TIMEOUT = 			5000;	/* USB timeout (milliseconds)*/

uint8_t BULK_WRITE_ENDPOINT = 			0x02;	/* bEndpointAddress     0x02  EP 2 OUT (Bulk)*/
uint8_t BULK_READ_ENDPOINT = 			0x81;	/* bEndpointAddress     0x81  EP 1 IN  (Bulk)*/
uint8_t bulk_buffer[64];				/* wMaxPacketSize     0x0040  1x 64 bytes*/

uint8_t INTERRUPT_READ_ENDPOINT = 		0x83;	/* bEndpointAddress     0x83  EP 3 IN  (Interrupt)*/
int INTERRUPT_LENGTH = 				1;	/* wMaxPacketSize     0x0001  1x 1 bytes*/

/* PS3mca commands*/
uint8_t PS3MCA_CMD_FIRST = 			0xaa;   /* First command for ps3mca protocol*/
uint8_t PS3MCA_CMD_TYPE_LONG = 			0x42;   /* PS1 type of command*/
uint8_t PS3MCA_CMD_VERIFY_CARD_TYPE = 		0x40;   /* Verify what type of card (PS1 or PS2)*/

/* PS3mca autentication response code*/
uint8_t RESPONSE_CODE =				0x55;   /* Expected response to PS3MCA_CMD_FIRST*/
uint8_t RESPONSE_STATUS_SUCCES = 		0x5a;   /* Expected response to PS3MCA_CMD_TYPE_LONG*/
uint8_t RESPONSE_WRONG =			0xaf;   /* Response to PS3MCA_CMD_TYPE_LONG if autentication is failed*/
uint8_t RESPONSE_PS1_CARD =			0x01;   /* This is a PS1 card*/
uint8_t RESPONSE_PS2_CARD =			0x02;   /* This is a PS2 card, unused with this driver*/

/* ----------------------------------------------End of PS3mca definitions and protocol---------------------------------------------*/





/* ---------------------------------------------PS1 Memory Card definitions----------------------------------------------------------*/
/* Information for PS1 memory card (SCPH-1020, SCPH-1170 and SCPH-119X)*/
int PS1CARD_TOTAL_SIZE = 131072;			/* 1024x128=131.072 bytes=131,1 kB=128 KiB=1 Megabit*/
int PS1CARD_FRAME_SIZE = 128;				/* frame is the equivalent of a disk sector=128 bytes*/
uint16_t PS1CARD_MIN_FRAME = 0x0000;			/* 0000h (0) min value frame*/
uint16_t PS1CARD_MAX_FRAME = 0x03ff;			/* 03ffh (1023) max value of frame (1024 total frame number but 0 is the first)*/
uint16_t frame;						/* Frame value, 0000h to 03ffh*/
uint8_t msb;						/* First two significant digits of the frame value*/
uint8_t lsb;						/* Last two significant digits of the frame value*/
uint8_t checksum;					/* Checksum variable*/
uint8_t ps1_ram_buffer[256];				/* 256 bytes of RAM on chip*/
/*
int PS1CARD_BLOCK_SIZE = 8192;				/* single block 1024x8=8192 bytes*/
/*
int PS1CARD_MAX_BLOCK = 16;				/* max number of block (however 1 is lost for formatting MC)*/

/* -------------------------------------------End of PS1 Memory Card definitions------------------------------------------------------*/






/* --------------------------------------------------PS1 Memory Card commands list----------------------------------------------------*/
/* Command list*/
uint8_t PS1CARD_CMD_MEMORY_CARD_ACCESS = 	0x81;	/* Memory Card Access, principal command for any action with any memory card*/

/* Classic PS1 commands list*/
uint8_t PS1CARD_CMD_READ = 			0x52;	/* Send Read Command (ASCII "R")*/
uint8_t PS1CARD_CMD_GET_ID = 			0x53;	/* Send Get ID Command (ASCII "S")*/
uint8_t PS1CARD_CMD_WRITE = 			0x57;	/* Send Write Command (ASCII "W")*/

/* PocketStation commands list*/
uint8_t POCKETSTATION_CMD_GET_ID = 		0x58;	/* Send Get ID PocketStation Command (ASCII "X")*/
uint8_t POCKETSTATION_CMD_59 = 			0x59;	/* Prepare File Execution with Dir_index, and Parameter*/
uint8_t POCKETSTATION_CMD_5A = 			0x5a;	/* Get Dir_index, ComFlags, F_SN, Date, and Time*/
uint8_t POCKETSTATION_CMD_TRANSFER_PKST_PSX = 	0x5b;	/* Execute Function and transfer data from Pocketstation to PSX*/
uint8_t POCKETSTATION_CMD_TRANSFER_PSX_PKST = 	0x5c;	/* Execute Function and transfer data from PSX to Pocketstation*/
uint8_t POCKETSTATION_CMD_DOWNLOAD_NOTIF = 	0x5d;	/* Execute Custom Download Notification Function*/
uint8_t POCKETSTATION_CMD_COMFLAGS_1_3_2 = 	0x5e;	/* Get-and-Send ComFlags.bit1,3,2 (send new, get old)*/
uint8_t POCKETSTATION_CMD_COMFLAGS_0 = 		0x5f;	/* Get-and-Send ComFlags.bit0 (send new, get old)*/
uint8_t POCKETSTATION_CMD_FUNC03 = 		0x50;	/* Change a FUNC 03h related value*/

/* PocketStation variable list*/
uint8_t curr_dir_index.bit0_7;
uint8_t curr_dir_index.bit8_15;
uint8_t comflags.bit0;
uint8_t comflags.bit1;
uint8_t comflags.bit2;
uint8_t comflags.bit3;
uint8_t f_sn.bit0_7;
uint8_t f_sn.bit8_15;
uint8_t f_sn.bit16_23;
uint8_t f_sn.bit24_31;

/* Expected reply from memory card (SCPH-1020) or PocketStation (SCPH-4000) or maybe other cards devices*/
uint8_t PS1CARD_REPLY_MC_ID_1 = 		0x5a;	/* Memory Card ID1*/
uint8_t PS1CARD_REPLY_MC_ID_2 = 		0x5d;	/* Memory Card ID2*/

uint8_t PS1CARD_REPLY_COMMAND_ACKNOWLEDGE_1 = 	0x5c;	/* Command Acknowledge 1*/
uint8_t PS1CARD_REPLY_COMMAND_ACKNOWLEDGE_2 = 	0x5d;	/* Command Acknowledge 2*/

uint8_t PS1CARD_REPLY_MEB_GOOD = 		0x47;	/* Memory End Byte Good (ASCII "G")*/
uint8_t PS1CARD_REPLY_MEB_BAD_CHECKSUM = 	0x4e;	/* Memory End Byte BadChecksum (ASCII "N")*/
uint8_t PS1CARD_REPLY_MEB_BAD_FRAME = 		0xff;	/* Memory End Byte BadFrame*/
uint8_t POCKETSTATION_REPLY_REJECT_EXECUTED = 	0xfd;	/* Reject write to Directory Entries of currently executed file*/
uint8_t POCKETSTATION_REPLY_REJECT_PROTECTED = 	0xfe;	/* Reject write to write-protected Broken Frame region*/

uint8_t PS1CARD_REPLY_NUMBER_FRAME_1 = 		0x04;	/* First two significant digits of the number of frame*/
uint8_t PS1CARD_REPLY_NUMBER_FRAME_2 = 		0x00;	/* Last two significant digits of the number of frame (0400h=1024)*/
uint8_t PS1CARD_REPLY_FRAME_SIZE_1 = 		0x00;	/* First two significant digits of the frame size*/
uint8_t PS1CARD_REPLY_FRAME_SIZE_2 = 		0x80;	/* Last two significant digits of the frame size (0080h=128)*/


/* -----------------------------------------------End of PS1 Memory Card commands list------------------------------------------------*/




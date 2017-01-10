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

#include <stdio.h>
#include <libusb-1.0/libusb.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "ps3mca-ps1-driver.h"

void processMessage(const uint8_t*);

int res = 0;				/* Return codes from libusb functions */
int ret = 0;				/* Return codes from libusb functions */
libusb_device_handle* handle = 0;	/* Handle for USB device */
int kernelDriverDetached = 0;		/* Set to 1 if kernel driver detached */
int numBytes = 0;			/* Actual bytes transferred. */


int c;					/* Checksum loop indicator*/

void delay(int milliseconds)		/* Delay definition*/
{
    long pause;
    clock_t now,then;

    pause = milliseconds*(CLOCKS_PER_SEC/1000);
    now = then = clock();
    while( (now-then) < pause )
        now = clock();
}

void close_ps3mca()			/* Unmount the ps3mca*/
{

  /* Release interface #0. */
  res = libusb_release_interface(handle, 0);
  if (0 != res)
  {
    fprintf(stderr, "Error releasing interface.\n");
  }

  /* If we detached a kernel driver from interface #0 earlier, we'll now 
   * need to attach it again.  */
  if (kernelDriverDetached)
  {
    libusb_attach_kernel_driver(handle, 0);
  }

  /* Shutdown libusb. */
  libusb_exit(0);

}


/* --------------------------------------------PS3mca verification of card (PS1 or PS2)---------------------------------------------*/
int PS3mca_verify_card ()
{
  uint8_t cmd_card_verification[2];
  uint8_t response_card_verification[2];

  /* This is the command get id for memory card (SCPH-1020) or PocketStation (SCPH-4000)*/
  memset(cmd_card_verification, 0, sizeof(cmd_card_verification));
  /* first 4 byte are about ps3 memory card adapter protocol*/
  cmd_card_verification[0] = PS3MCA_CMD_FIRST;			/* First command for ps3mca protocol*/
  cmd_card_verification[1] = PS3MCA_CMD_VERIFY_CARD_TYPE;	/* Verify what type of card (PS1 or PS2)*/


  
  /* Send the message to endpoint with a 5000ms timeout. */
  res = libusb_bulk_transfer(handle, BULK_WRITE_ENDPOINT, cmd_card_verification, sizeof(cmd_card_verification), &numBytes, USB_TIMEOUT);
  if (res == 0)
  {
    printf("\nType of Memory Card:\n");
    /* See on screen what is transmitted for debug purpose*/
    #if DEBUG
    printf("\n%d bytes transmitted successfully.\n", numBytes);
    printf("Send:\n");
    printf("%x \n", cmd_card_verification[0]);
    printf("%x \n", cmd_card_verification[1]);
    printf("\n");
    #endif
  }
  else
  {
    fprintf(stderr, "Error sending message to device.\n");
  }

  /* Clean response.*/
  memset(response_card_verification, 0, sizeof(response_card_verification));

  /* Listen for a message.*/
  /* Wait up to 5 seconds for a message to arrive on endpoint*/
  res = libusb_bulk_transfer(handle, BULK_READ_ENDPOINT, response_card_verification, sizeof(response_card_verification), &numBytes, USB_TIMEOUT);
  if (0 == res)
  {
    if (numBytes == sizeof(response_card_verification))
    {
        /* See on screen what arrive for debug purpose*/
	#if DEBUG
	printf("Received:\n");
	printf("%x \n", response_card_verification[0]);
	printf("%x \n", response_card_verification[1]);
	printf("\n");
        #endif

        /* Verify if there is a PS1 card*/
        if (response_card_verification[0] == RESPONSE_CODE & response_card_verification[1] == RESPONSE_PS1_CARD)
        {
          printf("PS1 Memory Card.\n\n");
        } 

        /* Verify if there is a PS2 card*/
        else if (response_card_verification[0] == RESPONSE_CODE & response_card_verification[1] == RESPONSE_PS2_CARD)    
        {
          printf("PS2 Memory Card.\nFor the moment isn't in roadmap to support it (see FAQ).\n\n");
        }

        /* Other unknown PS3mca error*/
        else   
        {
          fprintf(stderr, "Unknown error on PS3mca protocol.\n");
        }

    }
    else
    {
      fprintf(stderr, "Received %d bytes, expected %d.\n", numBytes, sizeof(response_card_verification));
    }
  }
  else
  {
    fprintf(stderr, "Error receiving message.\n");
  }

}
/* -----------------------------------------End of PS3mca verification of card (PS1 or PS2)-----------------------------------------*/








/* -------------------------------------------------------PS1 command get id--------------------------------------------------------*/
int PS1_get_id ()
{
  uint8_t cmd_get_id[14];

  /* This is the command get id for memory card (SCPH-1020) or PocketStation (SCPH-4000)*/
  memset(cmd_get_id, 0, sizeof(cmd_get_id));
  /* first 4 byte are about ps3 memory card adapter protocol*/
  cmd_get_id[0] = PS3MCA_CMD_FIRST;			/* First command for ps3mca protocol*/
  cmd_get_id[1] = PS3MCA_CMD_TYPE_LONG;			/* PS1 type of command*/
  cmd_get_id[2] = 0x0a;					/* 14-4=10=0ah lenght of command. Can be cmd_get_id[2]=sizeof(cmd_get_id)-4*/
  cmd_get_id[3] = 0x00;					/* memset set all to 0x00, but I prefer to specify it a second time ;)*/
  /* this is the real command get id with lenght of 10=0x0a*/
  cmd_get_id[4] = PS1CARD_CMD_MEMORY_CARD_ACCESS;	/* Memory Card Access, principal command for any action with any memory card*/
  cmd_get_id[5] = PS1CARD_CMD_GET_ID;			/* Send Get ID Command (ASCII "S")*/
  cmd_get_id[6] = 0x00;					/* Ask Memory Card ID1*/
  cmd_get_id[7] = 0x00;					/* Ask Memory Card ID2*/
  cmd_get_id[8] = 0x00;					/* Ask Command Acknowledge 1*/
  cmd_get_id[9] = 0x00;					/* Ask Command Acknowledge 2*/
  cmd_get_id[10] = 0x00;				/* Ask the first two significant digits of the number of frame*/
  cmd_get_id[11] = 0x00;				/* Ask the last two significant digits of the number of frame*/
  cmd_get_id[12] = 0x00;				/* Ask the first two significant digits of the frame size*/
  cmd_get_id[13] = 0x00;				/* Ask the last two significant digits of the frame size*/


  
  /* Send the message to endpoint with a 5000ms timeout. */
  res = libusb_bulk_transfer(handle, BULK_WRITE_ENDPOINT, cmd_get_id, sizeof(cmd_get_id), &numBytes, USB_TIMEOUT);
  if (res == 0)
  {
    printf("\nSend PS1 GET ID COMMAND\n");
    /* See on screen what is transmitted for debug purpose*/
    #if DEBUG
    printf("\n%d bytes transmitted successfully.\n", numBytes);
    printf("Send:\n");
    printf("%x \n", cmd_get_id[0]);
    printf("%x \n", cmd_get_id[1]);
    printf("%x \n", cmd_get_id[2]);
    printf("%x \n", cmd_get_id[3]);
    printf("%x \n", cmd_get_id[4]);
    printf("%x \n", cmd_get_id[5]);
    printf("%x \n", cmd_get_id[6]);
    printf("%x \n", cmd_get_id[7]);
    printf("%x \n", cmd_get_id[8]);
    printf("%x \n", cmd_get_id[9]);
    printf("%x \n", cmd_get_id[10]);
    printf("%x \n", cmd_get_id[11]);
    printf("%x \n", cmd_get_id[12]);
    printf("%x \n", cmd_get_id[13]);
    printf("\n");
    #endif
  }
  else
  {
    fprintf(stderr, "Error sending message to device.\n");
  }
  /* Clean buffer.*/
  memset(bulk_buffer, 0, sizeof(bulk_buffer));

  /* Listen for a message.*/
  /* Wait up to 5 seconds for a message to arrive on endpoint*/
  res = libusb_bulk_transfer(handle, BULK_READ_ENDPOINT, bulk_buffer, sizeof(bulk_buffer), &numBytes, USB_TIMEOUT);
  if (0 == res)
  {
    if (numBytes <= sizeof(bulk_buffer))
    {
        /* See on screen what arrive for debug purpose*/
	#if DEBUG
	printf("Received:\n");
	printf("%x \n", bulk_buffer[0]);
	printf("%x \n", bulk_buffer[1]);
	printf("%x \n", bulk_buffer[2]);
	printf("%x \n", bulk_buffer[3]);
	printf("%x \n", bulk_buffer[4]);
	printf("%x \n", bulk_buffer[5]);
	printf("%x \n", bulk_buffer[6]);
	printf("%x \n", bulk_buffer[7]);
	printf("%x \n", bulk_buffer[8]);
	printf("%x \n", bulk_buffer[9]);
	printf("%x \n", bulk_buffer[10]);
	printf("%x \n", bulk_buffer[11]);
	printf("%x \n", bulk_buffer[12]);
	printf("%x \n", bulk_buffer[13]);
	printf("%x \n", bulk_buffer[14]);
	printf("%x \n", bulk_buffer[15]);
	printf("%x \n", bulk_buffer[16]);
	printf("%x \n", bulk_buffer[17]);
	printf("%x \n", bulk_buffer[18]);
	printf("%x \n", bulk_buffer[19]);
	printf("%x \n", bulk_buffer[20]);
	printf("%x \n", bulk_buffer[21]);
	printf("%x \n", bulk_buffer[22]);
	printf("%x \n", bulk_buffer[23]);
	printf("%x \n", bulk_buffer[24]);
	printf("%x \n", bulk_buffer[25]);
	printf("%x \n", bulk_buffer[26]);
	printf("%x \n", bulk_buffer[27]);
	printf("%x \n", bulk_buffer[28]);
	printf("%x \n", bulk_buffer[29]);
	printf("%x \n", bulk_buffer[30]);
	printf("%x \n", bulk_buffer[31]);
	printf("%x \n", bulk_buffer[32]);
	printf("%x \n", bulk_buffer[33]);
	printf("%x \n", bulk_buffer[34]);
	printf("%x \n", bulk_buffer[35]);
	printf("%x \n", bulk_buffer[36]);
	printf("%x \n", bulk_buffer[37]);
	printf("%x \n", bulk_buffer[38]);
	printf("%x \n", bulk_buffer[39]);
	printf("%x \n", bulk_buffer[40]);
	printf("%x \n", bulk_buffer[41]);
	printf("%x \n", bulk_buffer[42]);
	printf("%x \n", bulk_buffer[43]);
	printf("%x \n", bulk_buffer[44]);
	printf("%x \n", bulk_buffer[45]);
	printf("%x \n", bulk_buffer[46]);
	printf("%x \n", bulk_buffer[47]);
	printf("%x \n", bulk_buffer[48]);
	printf("%x \n", bulk_buffer[49]);
	printf("%x \n", bulk_buffer[50]);
	printf("%x \n", bulk_buffer[51]);
	printf("%x \n", bulk_buffer[52]);
	printf("%x \n", bulk_buffer[53]);
	printf("%x \n", bulk_buffer[54]);
	printf("%x \n", bulk_buffer[55]);
	printf("%x \n", bulk_buffer[56]);
	printf("%x \n", bulk_buffer[57]);
	printf("%x \n", bulk_buffer[58]);
	printf("%x \n", bulk_buffer[59]);
	printf("%x \n", bulk_buffer[60]);
	printf("%x \n", bulk_buffer[61]);
	printf("%x \n", bulk_buffer[62]);
	printf("%x \n", bulk_buffer[63]);
	printf("\n");
        #endif

        /* Verify if PS3mca send status succes code*/
        if (bulk_buffer[0] == RESPONSE_CODE & bulk_buffer[1] == RESPONSE_STATUS_SUCCES)
        {
	  #if DEBUG
          printf("Autentication verified.\n\n");
          #endif

          /* Verify if is a memory card (SCPH-1020) or a PocketStation (SCPH-4000)*/
          if (bulk_buffer[6] == PS1CARD_REPLY_MC_ID_1 & bulk_buffer[7] == PS1CARD_REPLY_MC_ID_2 & bulk_buffer[8] == PS1CARD_REPLY_COMMAND_ACKNOWLEDGE_1 & bulk_buffer[9] == PS1CARD_REPLY_COMMAND_ACKNOWLEDGE_2 & bulk_buffer[10] ==  PS1CARD_REPLY_NUMBER_FRAME_1 & bulk_buffer[11] == PS1CARD_REPLY_NUMBER_FRAME_2 & bulk_buffer[12] == PS1CARD_REPLY_FRAME_SIZE_1 & bulk_buffer[13] == PS1CARD_REPLY_FRAME_SIZE_2)
          {
            printf("This card seems to be a original PS memory card (SCPH-1020), (SCPH-1170), (SCPH-119X) or a PocketStation (SCPH-4000).\n");
	    printf("If you have a PocketStation (SCPH-4000) try to run the \"x\" option.\n\n");
          } 

          /* Other unknown memorycard*/
          else   
          {
            printf("This seems to be a unofficial memorycard.\n");
            printf("See FAQ for PS1 get id command.\n\n");
          }

        } 

        /* Verify if PS3mca send status wrong code*/
        else if (bulk_buffer[0] == RESPONSE_CODE & bulk_buffer[1] == RESPONSE_WRONG)    
        {
          fprintf(stderr, "Autentication failed.\n");
        }

        /* Other unknown PS3mca error*/
        else   
        {
          fprintf(stderr, "Unknown error on PS3mca protocol.\n");
        }

    }
    else
    {
      fprintf(stderr, "Received %d bytes, expected a maximum of %d.\n", numBytes, sizeof(bulk_buffer));
    }
  }
  else
  {
    fprintf(stderr, "Error receiving message.\n");
  }

}
/* ----------------------------------------------------End of PS1 command get id----------------------------------------------------*/






/* ---------------------------------------------------PocketStation command get id--------------------------------------------------*/
int PocketStation_get_id ()
{
  uint8_t cmd_get_id_pkst[9];

  /* This is the command get id for PocketStation (SCPH-4000)*/
  memset(cmd_get_id_pkst, 0, sizeof(cmd_get_id_pkst));
  /* first 4 byte are about ps3 memory card adapter protocol*/
  cmd_get_id_pkst[0] = PS3MCA_CMD_FIRST;			/* First command for ps3mca protocol*/
  cmd_get_id_pkst[1] = PS3MCA_CMD_TYPE_LONG;			/* PS1 type of command*/
  cmd_get_id_pkst[2] = 0x05;					/* 9-4=5=05h lenght of command. Can be cmd_get_id_pkst[2]=sizeof(cmd_get_id_pkst)-4*/
  cmd_get_id_pkst[3] = 0x00;					/* memset set all to 0x00, but I prefer to specify it a second time ;)*/
  /* this is the real command get id with lenght of 5=0x05*/
  cmd_get_id_pkst[4] = PS1CARD_CMD_MEMORY_CARD_ACCESS;		/* Memory Card Access, principal command for any action with any memory card*/
  cmd_get_id_pkst[5] = POCKETSTATION_CMD_GET_ID;		/* Send Get ID PocketStation Command (ASCII "X")*/
  cmd_get_id_pkst[6] = 0x00;					/* Ask ???*/
  cmd_get_id_pkst[7] = 0x00;					/* Ask ???*/
  cmd_get_id_pkst[8] = 0x00;					/* Ask ???*/


  
  /* Send the message to endpoint with a 5000ms timeout. */
  res = libusb_bulk_transfer(handle, BULK_WRITE_ENDPOINT, cmd_get_id_pkst, sizeof(cmd_get_id_pkst), &numBytes, USB_TIMEOUT);
  if (res == 0)
  {
    printf("\nSend POKETSTATION GET ID COMMAND\n");
    /* See on screen what is transmitted for debug purpose*/
    #if DEBUG
    printf("\n%d bytes transmitted successfully.\n", numBytes);
    printf("Send:\n");
    printf("%x \n", cmd_get_id_pkst[0]);
    printf("%x \n", cmd_get_id_pkst[1]);
    printf("%x \n", cmd_get_id_pkst[2]);
    printf("%x \n", cmd_get_id_pkst[3]);
    printf("%x \n", cmd_get_id_pkst[4]);
    printf("%x \n", cmd_get_id_pkst[5]);
    printf("%x \n", cmd_get_id_pkst[6]);
    printf("%x \n", cmd_get_id_pkst[7]);
    printf("%x \n", cmd_get_id_pkst[8]);
    printf("\n");
    #endif
  }
  else
  {
    fprintf(stderr, "Error sending message to device.\n");
  }
  /* Clean buffer.*/
  memset(bulk_buffer, 0, sizeof(bulk_buffer));

  /* Listen for a message.*/
  /* Wait up to 5 seconds for a message to arrive on endpoint*/
  res = libusb_bulk_transfer(handle, BULK_READ_ENDPOINT, bulk_buffer, sizeof(bulk_buffer), &numBytes, USB_TIMEOUT);
  if (0 == res)
  {
    if (numBytes <= sizeof(bulk_buffer))
    {
        /* See on screen what arrive for debug purpose*/
	#if DEBUG
	printf("Received:\n");
	printf("%x \n", bulk_buffer[0]);
	printf("%x \n", bulk_buffer[1]);
	printf("%x \n", bulk_buffer[2]);
	printf("%x \n", bulk_buffer[3]);
	printf("%x \n", bulk_buffer[4]);
	printf("%x \n", bulk_buffer[5]);
	printf("%x \n", bulk_buffer[6]);
	printf("%x \n", bulk_buffer[7]);
	printf("%x \n", bulk_buffer[8]);
	printf("%x \n", bulk_buffer[9]);
	printf("%x \n", bulk_buffer[10]);
	printf("%x \n", bulk_buffer[11]);
	printf("%x \n", bulk_buffer[12]);
	printf("%x \n", bulk_buffer[13]);
	printf("%x \n", bulk_buffer[14]);
	printf("%x \n", bulk_buffer[15]);
	printf("%x \n", bulk_buffer[16]);
	printf("%x \n", bulk_buffer[17]);
	printf("%x \n", bulk_buffer[18]);
	printf("%x \n", bulk_buffer[19]);
	printf("%x \n", bulk_buffer[20]);
	printf("%x \n", bulk_buffer[21]);
	printf("%x \n", bulk_buffer[22]);
	printf("%x \n", bulk_buffer[23]);
	printf("%x \n", bulk_buffer[24]);
	printf("%x \n", bulk_buffer[25]);
	printf("%x \n", bulk_buffer[26]);
	printf("%x \n", bulk_buffer[27]);
	printf("%x \n", bulk_buffer[28]);
	printf("%x \n", bulk_buffer[29]);
	printf("%x \n", bulk_buffer[30]);
	printf("%x \n", bulk_buffer[31]);
	printf("%x \n", bulk_buffer[32]);
	printf("%x \n", bulk_buffer[33]);
	printf("%x \n", bulk_buffer[34]);
	printf("%x \n", bulk_buffer[35]);
	printf("%x \n", bulk_buffer[36]);
	printf("%x \n", bulk_buffer[37]);
	printf("%x \n", bulk_buffer[38]);
	printf("%x \n", bulk_buffer[39]);
	printf("%x \n", bulk_buffer[40]);
	printf("%x \n", bulk_buffer[41]);
	printf("%x \n", bulk_buffer[42]);
	printf("%x \n", bulk_buffer[43]);
	printf("%x \n", bulk_buffer[44]);
	printf("%x \n", bulk_buffer[45]);
	printf("%x \n", bulk_buffer[46]);
	printf("%x \n", bulk_buffer[47]);
	printf("%x \n", bulk_buffer[48]);
	printf("%x \n", bulk_buffer[49]);
	printf("%x \n", bulk_buffer[50]);
	printf("%x \n", bulk_buffer[51]);
	printf("%x \n", bulk_buffer[52]);
	printf("%x \n", bulk_buffer[53]);
	printf("%x \n", bulk_buffer[54]);
	printf("%x \n", bulk_buffer[55]);
	printf("%x \n", bulk_buffer[56]);
	printf("%x \n", bulk_buffer[57]);
	printf("%x \n", bulk_buffer[58]);
	printf("%x \n", bulk_buffer[59]);
	printf("%x \n", bulk_buffer[60]);
	printf("%x \n", bulk_buffer[61]);
	printf("%x \n", bulk_buffer[62]);
	printf("%x \n", bulk_buffer[63]);
	printf("\n");
        #endif

        /* Verify if PS3mca send status succes code*/
        if (bulk_buffer[0] == RESPONSE_CODE & bulk_buffer[1] == RESPONSE_STATUS_SUCCES)
        {
	  #if DEBUG
          printf("Autentication verified.\n\n");
	  #endif

          printf("This card seems to be a original PocketStation (SCPH-4000).\n");
	  /* If responde appears as a Pocketstation, but can responde in various mode.*/
          printf("%x.\n", bulk_buffer[6]);
          printf("%x.\n", bulk_buffer[7]);
          printf("%x.\n", bulk_buffer[8]);

        } 

        /* Verify if PS3mca send status wrong code*/
        else if (bulk_buffer[0] == RESPONSE_CODE & bulk_buffer[1] == RESPONSE_WRONG)    
        {
          fprintf(stderr, "Autentication failed.\n");
	  printf("This seems to be a normal memorycard.\n\n");
        }

        /* Other unknown PS3mca error*/
        else   
        {
          fprintf(stderr, "Unknown error on PS3mca protocol.\n");
        }

    }

    else
    {
      fprintf(stderr, "Received %d bytes, expected a maximum of %d.\n", numBytes, sizeof(bulk_buffer));
    }
  }
  else
  {
    fprintf(stderr, "Error receiving message.\n");
  }

}
/* ------------------------------------------------End of PocketStation command get id----------------------------------------------*/








/* -------------------------------PocketStation command get Dir_index, ComFlags, F_SN, Date, and Time-------------------------------*/
/*
   Send Reply Comment
   81h  N/A   Memory Card Access
   5Ah  FLAG  Send Command 5Ah
   (0)  12h   Send dummy/zero, receive length of following data (12h)
   (0)  INDX  Send dummy/zero, receive curr_dir_index.bit8-15   (00h)
   (0)  INDX  Send dummy/zero, receive curr_dir_index.bit0-7    (00h..0Fh)
   (0)  FLG   Send dummy/zero, receive ComFlags.bit0            (00h or 01h)
   (0)  FLG   Send dummy/zero, receive ComFlags.bit1            (00h or 01h)
   (0)  FLG   Send dummy/zero, receive ComFlags.bit3            (00h or 01h)
   (0)  FLG   Send dummy/zero, receive ComFlags.bit2            (00h or 01h)
   (0)  SN    Send dummy/zero, receive F_SN.bit0-7              (whatever)
   (0)  SN    Send dummy/zero, receive F_SN.bit8-15             (whatever)
   (0)  SN    Send dummy/zero, receive F_SN.bit16-23            (whatever)
   (0)  SN    Send dummy/zero, receive F_SN.bit24-31            (whatever)
   (0)  DATE  Send dummy/zero, receive BCD Day                  (01h..31h)
   (0)  DATE  Send dummy/zero, receive BCD Month                (01h..12h)
   (0)  DATE  Send dummy/zero, receive BCD Year                 (00h..99h)
   (0)  DATE  Send dummy/zero, receive BCD Century              (00h..99h)
   (0)  TIME  Send dummy/zero, receive BCD Second               (00h..59h)
   (0)  TIME  Send dummy/zero, receive BCD Minute               (00h..59h)
   (0)  TIME  Send dummy/zero, receive BCD Hour                 (00h..23h)
   (0)  TIME  Send dummy/zero, receive BCD Day of Week          (01h..07h)
   At midnight, the function may accidently return the date for the old day, and the time for the new day.
*/
int PocketStation_get_dir_date ()
{
  uint8_t cmd_get_dir_date_pkst[25];

  /* This is the command get id for PocketStation (SCPH-4000)*/
  memset(cmd_get_dir_date_pkst, 0, sizeof(cmd_get_dir_date_pkst));
  /* first 4 byte are about ps3 memory card adapter protocol*/
  cmd_get_dir_date_pkst[0] = PS3MCA_CMD_FIRST;				/* First command for ps3mca protocol*/
  cmd_get_dir_date_pkst[1] = PS3MCA_CMD_TYPE_LONG;			/* PS1 type of command*/
  cmd_get_dir_date_pkst[2] = 0x15;					/* 25-4=21=15h lenght of command. Can be cmd_get_dir_date_pkst[2]=sizeof(cmd_get_dir_date_pkst)-4*/
  cmd_get_dir_date_pkst[3] = 0x00;					/* memset set all to 0x00, but I prefer to specify it a second time ;)*/
  /* this is the real command get id with lenght of 5=0x05*/
  cmd_get_dir_date_pkst[4] = PS1CARD_CMD_MEMORY_CARD_ACCESS;		/* Memory Card Access, principal command for any action with any memory card*/
  cmd_get_dir_date_pkst[5] = POCKETSTATION_CMD_5A;			/* Send Get Dir_index, ComFlags, F_SN, Date, and Time*/
  /* according to memset all value are set to 0x00*/

  
  /* Send the message to endpoint with a 5000ms timeout. */
  res = libusb_bulk_transfer(handle, BULK_WRITE_ENDPOINT, cmd_get_dir_date_pkst, sizeof(cmd_get_dir_date_pkst), &numBytes, USB_TIMEOUT);
  if (res == 0)
  {
    printf("\nSend POKETSTATION get Dir_index, ComFlags, F_SN, Date, and Time COMMAND\n");
    /* See on screen what is transmitted for debug purpose*/
    #if DEBUG
    printf("\n%d bytes transmitted successfully.\n", numBytes);
    printf("Send:\n");
    printf("%x \n", cmd_get_dir_date_pkst[0]);
    printf("%x \n", cmd_get_dir_date_pkst[1]);
    printf("%x \n", cmd_get_dir_date_pkst[2]);
    printf("%x \n", cmd_get_dir_date_pkst[3]);
    printf("%x \n", cmd_get_dir_date_pkst[4]);
    printf("%x \n", cmd_get_dir_date_pkst[5]);
    printf("\n");
    #endif
  }
  else
  {
    fprintf(stderr, "Error sending message to device.\n");
  }
  /* Clean buffer.*/
  memset(bulk_buffer, 0, sizeof(bulk_buffer));

  /* Listen for a message.*/
  /* Wait up to 5 seconds for a message to arrive on endpoint*/
  res = libusb_bulk_transfer(handle, BULK_READ_ENDPOINT, bulk_buffer, sizeof(bulk_buffer), &numBytes, USB_TIMEOUT);
  if (0 == res)
  {
    if (numBytes <= sizeof(bulk_buffer))
    {
        /* See on screen what arrive for debug purpose*/
	#if DEBUG
	printf("Received:\n");
	printf("%x \n", bulk_buffer[0]);
	printf("%x \n", bulk_buffer[1]);
	printf("%x \n", bulk_buffer[2]);
	printf("%x \n", bulk_buffer[3]);
	printf("%x \n", bulk_buffer[4]);
	printf("%x \n", bulk_buffer[5]);
	printf("%x \n", bulk_buffer[6]);
	printf("%x \n", bulk_buffer[7]);
	printf("%x \n", bulk_buffer[8]);
	printf("%x \n", bulk_buffer[9]);
	printf("%x \n", bulk_buffer[10]);
	printf("%x \n", bulk_buffer[11]);
	printf("%x \n", bulk_buffer[12]);
	printf("%x \n", bulk_buffer[13]);
	printf("%x \n", bulk_buffer[14]);
	printf("%x \n", bulk_buffer[15]);
	printf("%x \n", bulk_buffer[16]);
	printf("%x \n", bulk_buffer[17]);
	printf("%x \n", bulk_buffer[18]);
	printf("%x \n", bulk_buffer[19]);
	printf("%x \n", bulk_buffer[20]);
	printf("%x \n", bulk_buffer[21]);
	printf("%x \n", bulk_buffer[22]);
	printf("%x \n", bulk_buffer[23]);
	printf("%x \n", bulk_buffer[24]);
	printf("%x \n", bulk_buffer[25]);
	printf("%x \n", bulk_buffer[26]);
	printf("%x \n", bulk_buffer[27]);
	printf("%x \n", bulk_buffer[28]);
	printf("%x \n", bulk_buffer[29]);
	printf("%x \n", bulk_buffer[30]);
	printf("%x \n", bulk_buffer[31]);
	printf("%x \n", bulk_buffer[32]);
	printf("%x \n", bulk_buffer[33]);
	printf("%x \n", bulk_buffer[34]);
	printf("%x \n", bulk_buffer[35]);
	printf("%x \n", bulk_buffer[36]);
	printf("%x \n", bulk_buffer[37]);
	printf("%x \n", bulk_buffer[38]);
	printf("%x \n", bulk_buffer[39]);
	printf("%x \n", bulk_buffer[40]);
	printf("%x \n", bulk_buffer[41]);
	printf("%x \n", bulk_buffer[42]);
	printf("%x \n", bulk_buffer[43]);
	printf("%x \n", bulk_buffer[44]);
	printf("%x \n", bulk_buffer[45]);
	printf("%x \n", bulk_buffer[46]);
	printf("%x \n", bulk_buffer[47]);
	printf("%x \n", bulk_buffer[48]);
	printf("%x \n", bulk_buffer[49]);
	printf("%x \n", bulk_buffer[50]);
	printf("%x \n", bulk_buffer[51]);
	printf("%x \n", bulk_buffer[52]);
	printf("%x \n", bulk_buffer[53]);
	printf("%x \n", bulk_buffer[54]);
	printf("%x \n", bulk_buffer[55]);
	printf("%x \n", bulk_buffer[56]);
	printf("%x \n", bulk_buffer[57]);
	printf("%x \n", bulk_buffer[58]);
	printf("%x \n", bulk_buffer[59]);
	printf("%x \n", bulk_buffer[60]);
	printf("%x \n", bulk_buffer[61]);
	printf("%x \n", bulk_buffer[62]);
	printf("%x \n", bulk_buffer[63]);
	printf("\n");
        #endif

        /* Verify if PS3mca send status succes code*/
        if (bulk_buffer[0] == RESPONSE_CODE & bulk_buffer[1] == RESPONSE_STATUS_SUCCES)
        {
	  #if DEBUG
          printf("Autentication verified.\n\n");
	  #endif

          /* Verify lenght of data sended by PocketStation*/
	  if (bulk_buffer[6] == 0x12)
	  {
	    #if DEBUG
            printf("Good number of bytes sended by the pocketstation.\n\n");
	    #endif
	  }

	  else
	  {
            fprintf(stderr, "The pocketstation send wrong number of bytes.\n");
            fprintf(stderr, "%x instead of 12.\n\n", bulk_buffer[6]);
	  }

	  /* Set curr_dir_index.bit8-15*/
          printf("The curr_dir_index.bit8_15 is %x.\n", bulk_buffer[7]);
	  curr_dir_index_bit8_15 = bulk_buffer[7];

	  /* Set curr_dir_index.bit0-7*/
          printf("The curr_dir_index.bit0_7 is %x.\n", bulk_buffer[8]);
	  curr_dir_index_bit0_7 = bulk_buffer[8];

	  /* Set ComFlags.bit0*/
          printf("The ComFlags.bit0 is %x.\n", bulk_buffer[9]);
	  comflags_bit0 = bulk_buffer[9];

	  /* Set ComFlags.bit1*/
          printf("The ComFlags.bit1 is %x.\n", bulk_buffer[10]);
	  comflags_bit1 = bulk_buffer[10];

	  /* Set ComFlags.bit3*/
          printf("The ComFlags.bit3 is %x.\n", bulk_buffer[11]);
	  comflags_bit3 = bulk_buffer[11];

	  /* Set ComFlags.bit2*/
          printf("The ComFlags.bit2 is %x.\n", bulk_buffer[12]);
	  comflags_bit2 = bulk_buffer[12];

	  /* Set F_SN.bit0-7*/
          printf("The F_SN.bit0-7 is %x.\n", bulk_buffer[13]);
	  f_sn_bit0_7 = bulk_buffer[13];

	  /* Set F_SN.bit8-15*/
          printf("The F_SN.bit8-15 is %x.\n", bulk_buffer[14]);
	  f_sn_bit8_15 = bulk_buffer[14];

	  /* Set F_SN.bit16-23*/
          printf("The F_SN.bit16-23 is %x.\n", bulk_buffer[15]);
	  f_sn_bit16_23 = bulk_buffer[15];

	  /* Set F_SN.bit24-31*/
          printf("The F_SN.bit24-31 is %x.\n", bulk_buffer[16]);
	  f_sn_bit24_31 = bulk_buffer[16];


	  /* See date set on PocketStation*/
	  printf("For the Pocket Station is:\n");

          /* Day of the week*/
	  if (bulk_buffer[26] == 0x01)
	  {
            printf("Monday");
	  }
	  else if (bulk_buffer[26] == 0x02)
	  {
            printf("Tuesday");
	  }
	  else if (bulk_buffer[26] == 0x03)
	  {
            printf("Wednesday");
	  }
	  else if (bulk_buffer[26] == 0x04)
	  {
            printf("Thursday");
	  }
	  else if (bulk_buffer[26] == 0x05)
	  {
            printf("Friday");
	  }
	  else if (bulk_buffer[26] == 0x06)
	  {
            printf("Saturday");
	  }
	  else if (bulk_buffer[26] == 0x07)
	  {
            printf("Sunday");
	  }
	  else
	  {
	    fprintf(stderr, "error for day of the week, received %x expected a value from 01 to 07.\n", bulk_buffer[26]);
	  }

	  /* Day number*/
	  printf(" %x ", bulk_buffer[17]);
          /* Mounth name*/
	  if (bulk_buffer[18] == 0x01)
	  {
            printf("January");
	  }
	  else if (bulk_buffer[18] == 0x02)
	  {
            printf("February");
	  }
	  else if (bulk_buffer[18] == 0x03)
	  {
            printf("March");
	  }
	  else if (bulk_buffer[18] == 0x04)
	  {
            printf("April");
	  }
	  else if (bulk_buffer[18] == 0x05)
	  {
            printf("May");
	  }
	  else if (bulk_buffer[18] == 0x06)
	  {
            printf("June");
	  }
	  else if (bulk_buffer[18] == 0x07)
	  {
            printf("July");
	  }
	  else if (bulk_buffer[18] == 0x08)
	  {
            printf("August");
	  }
	  else if (bulk_buffer[18] == 0x09)
	  {
            printf("September");
	  }
	  else if (bulk_buffer[18] == 0x10)
	  {
            printf("October");
	  }
	  else if (bulk_buffer[18] == 0x11)
	  {
            printf("November");
	  }
	  else if (bulk_buffer[18] == 0x12)
	  {
            printf("December");
	  }
	  else
	  {
	    fprintf(stderr, "error for mounth, received %x expected a value from 01 to 12.\n", bulk_buffer[18]);
	  }
	  /* Year (century, year)*/
	  printf(" %x%x\n", bulk_buffer[20], bulk_buffer[19]);

	  /* See time set on PocketStation*/
	  printf("Clock set to:\n");
	  printf("%x.%x.%x\n\n", bulk_buffer[23], bulk_buffer[22], bulk_buffer[21]);	/* Hour.minutes.seconds*/

        } 


	/* Verify if PS3mca send status wrong code*/
        else if (bulk_buffer[0] == RESPONSE_CODE & bulk_buffer[1] == RESPONSE_WRONG)    
        {
          fprintf(stderr, "Autentication failed.\n");
        }

        /* Other unknown PS3mca error*/
        else   
        {
          fprintf(stderr, "Unknown error on PS3mca protocol.\n");
        }

    }

    else
    {
      fprintf(stderr, "Received %d bytes, expected a maximum of %d.\n", numBytes, sizeof(bulk_buffer));
    }
  }
  else
  {
    fprintf(stderr, "Error receiving message.\n");
  }

}
/* ------------------------------End of PocketStation command get Dir_index, ComFlags, F_SN, Date, and Time-------------------------*/








/* -------------------------------------------------------PS1 command read----------------------------------------------------------*/
/* Command for read every single frame*/
/* Reading Data from Memory Card
   Send Reply Comment
   81h  N/A   Memory Card Access (unlike 01h=Controller access), dummy response
   52h  FLAG  Send Read Command (ASCII "R"), Receive FLAG Byte
   00h  5Ah   Receive Memory Card ID1
   00h  5Dh   Receive Memory Card ID2
   MSB  (00h) Send Address MSB  ;\frame number (0..3FFh)
   LSB  (pre) Send Address LSB  ;/
   00h  5Ch   Receive Command Acknowledge 1  ;<-- late /ACK after this byte-pair
   00h  5Dh   Receive Command Acknowledge 2
   00h  MSB   Receive Confirmed Address MSB
   00h  LSB   Receive Confirmed Address LSB
   00h  ...   Receive Data Frame (128 bytes)
   00h  CHK   Receive Checksum (MSB xor LSB xor Data bytes)
   00h  47h   Receive Memory End Byte (should be always 47h="G"=Good for Read)
*/
int PS1_read ()
{
  uint8_t cmd_read[144];
  FILE *output=fopen( "Readed_memory_card.mcd", "wb" );	/* Open and create a binary file output in writing*/



  /* Start of frame to frame loop*/
  for (frame = PS1CARD_MIN_FRAME; frame <= PS1CARD_MAX_FRAME; frame++)
  {

  /* Split frame value in two*/
  msb = (uint8_t)((frame & 0xFF00) >> 8);
  lsb = (uint8_t)(frame & 0x00FF);
  /* Clean command read*/
  memset(cmd_read, 0, sizeof(cmd_read));
  /* This is the write command for memory card (SCPH-1020) or PocketStation (SCPH-4000)*/
  /* first 4 byte are about ps3 memory card adapter protocol*/
  cmd_read[0] = PS3MCA_CMD_FIRST;			/* First command for ps3mca protocol*/
  cmd_read[1] = PS3MCA_CMD_TYPE_LONG;			/* PS1 type of command*/
  cmd_read[2] = 0x8c;					/* 144-4=140=8ch lenght of command. Can be cmd_read[2]=sizeof(cmd_read)-4*/
  cmd_read[3] = 0x00;					/* memset set all to 0x00, but I prefer to specify it a second time ;)*/
  /* this is the real command read with lenght of 140=0x8c*/
  cmd_read[4] = PS1CARD_CMD_MEMORY_CARD_ACCESS;		/* Memory Card Access, principal command for any action with any memory card*/
  cmd_read[5] = PS1CARD_CMD_READ;			/* Send Read Command (ASCII "R")*/
  cmd_read[6] = 0x00;					/* Ask Memory Card ID1*/
  cmd_read[7] = 0x00;					/* Ask Memory Card ID2*/
  cmd_read[8] = msb;					/* First two significant digits of the frame value*/
  cmd_read[9] = lsb;					/* Last two significant digits of the frame value*/
  /* Send 0x00 134 times (2 Command Acknowledge + 2 Confirmed Address + 128 Data Frame + 1 Checksum + 1 Memory End Byte)*/


  
  /* Send the message to endpoint with a 5000ms timeout. */
  res = libusb_bulk_transfer(handle, BULK_WRITE_ENDPOINT, cmd_read, sizeof(cmd_read), &numBytes, USB_TIMEOUT);
  if (res == 0)
  {
    /* See on screen what is transmitted for debug purpose*/
    #if DEBUG
    printf("\n%d bytes transmitted successfully on frame %d:\n", numBytes, frame);
    printf("Send:\n");
    printf("%x \n", cmd_read[0]);
    printf("%x \n", cmd_read[1]);
    printf("%x \n", cmd_read[2]);
    printf("%x \n", cmd_read[3]);
    printf("%x \n", cmd_read[4]);
    printf("%x \n", cmd_read[5]);
    printf("%x \n", cmd_read[6]);
    printf("%x \n", cmd_read[7]);
    printf("%x \n", cmd_read[8]);
    printf("%x \n", cmd_read[9]);
    printf("%x \n", cmd_read[10]);
    printf("%x \n", cmd_read[11]);
    printf("%x \n", cmd_read[12]);
    printf("%x \n", cmd_read[13]);
    printf("%x \n", cmd_read[142]);
    printf("%x \n", cmd_read[143]);
    printf("\n");
    #endif
  }
  else
  {
    fprintf(stderr, "Error sending message to device.\n");
  }

  /* Clean buffer.*/
  memset(ps1_ram_buffer, 0, sizeof(ps1_ram_buffer));

  /* Listen for a message.*/
  /* Wait up to 5 seconds for a message to arrive on endpoint*/
  res = libusb_bulk_transfer(handle, BULK_READ_ENDPOINT, ps1_ram_buffer, sizeof(ps1_ram_buffer), &numBytes, USB_TIMEOUT);
  if (0 == res)
  {
    if (numBytes <= sizeof(ps1_ram_buffer))
    {
        /* Verify if PS3mca send status succes code*/
        if (ps1_ram_buffer[0] == RESPONSE_CODE & ps1_ram_buffer[1] == RESPONSE_STATUS_SUCCES)
        {
	  #if DEBUG
          printf("Autentication verified on frame %d.\n", frame);
          #endif
	  #if VERBOSE
	  printf("Reading frame %d.\n", frame);
	  #endif
        }

        /* Verify if PS3mca send status wrong code*/
        else if (ps1_ram_buffer[0] == RESPONSE_CODE & ps1_ram_buffer[1] == RESPONSE_WRONG)    
        {
          fprintf(stderr, "Autentication failed on frame %d.\n", frame);
        }

        /* Other unknown PS3mca error*/
        else   
        {
          fprintf(stderr, "Unknown error on PS3mca protocol on frame %d.\n", frame);
        }

        /* Verify command acknowledge*/
        if (ps1_ram_buffer[10] == PS1CARD_REPLY_COMMAND_ACKNOWLEDGE_1 & ps1_ram_buffer[11] == PS1CARD_REPLY_COMMAND_ACKNOWLEDGE_2)
        {
	  #if DEBUG
          printf("Good command acknowledge on frame %d.\n", frame);
          #endif
        }

        /* Unknown command acknowledge error*/
        else   
        {
          fprintf(stderr, "Unknown command acknowledge error on frame %d.\n", frame);
        }

        /* Verify msb and lsb*/
        if (ps1_ram_buffer[12] == msb & ps1_ram_buffer[13] == lsb)
        {
	  #if DEBUG
          printf("Confirmed frame number on frame %d.\n", frame);
          #endif
        }

        /* Unknown frame error*/
        else   
        {
          fprintf(stderr, "Unknown frame number error on frame %d.\n", frame);
          fprintf(stderr, "Return frame number %d %d.\n\n", ps1_ram_buffer[12], ps1_ram_buffer[13]);
        }

	/* This permit to select and save only the received Data Frame (PS1CARD_FRAME_SIZE=128 bytes).*/
	/* First 14 bytes are about PS3mca (4 bytes) and PS1 (10 bytes) protocol.*/
	/* Last 2 bytes are Checksum & Memory End Byte.*/
	fwrite(&ps1_ram_buffer[14], 1, PS1CARD_FRAME_SIZE, output);

	/* Clean checksum.*/
	checksum = 0x00;

	/* Calc supposed checksum.*/
	for (c = 12; c < 12+PS1CARD_FRAME_SIZE; c++)		/* Loop of msb + lsb + all data bytes*/
	{
	checksum = checksum ^ ps1_ram_buffer[c];		/* Checksum = MSB xor LSB xor all Data bytes*/
	}

        /* Verify checksum*/
        if (ps1_ram_buffer[142] == checksum)
        {
	#if DEBUG
	printf("Good checksum on frame %d.\n" frame);
        #endif
        }

        /* Checksum error*/
        else   
        {
          fprintf(stderr, "Incorrect checksum on frame %d.\n", frame);
          fprintf(stderr, "Received %x, should be %x.\n\n", ps1_ram_buffer[142], checksum);
        }


        /* Verify MEB*/
        if (ps1_ram_buffer[143] == PS1CARD_REPLY_MEB_GOOD)
        {
	#if DEBUG
	printf("Good Memory End Byte on frame %d.\n" frame);
        #endif
        }

        /* Other unknown MEB error*/
        else   
        {
          fprintf(stderr, "Unknown Memory End Byte on frame %d.\n", frame);
          fprintf(stderr, "Received %x, should be 47.\n\n", ps1_ram_buffer[143]);
        }


    }
    else
    {
      fprintf(stderr, "Received %d bytes, expected a maximum of %d bytes on frame %d.\n", numBytes, sizeof(ps1_ram_buffer), frame);
    }
  }


  else
  {
    fprintf(stderr, "Error receiving message.\n");
  }

  }

  /* Clean and close the file output*/
  fflush(output);
  fclose(output);

}
/* ----------------------------------------------------End of PS1 command read------------------------------------------------------*/








/* -------------------------------------------------------PS1 command write----------------------------------------------------------*/
/* Command for start writing every single frame*/
/* Writing Data to Memory Card
   Send Reply Comment
   81h  N/A   Memory Card Access (unlike 01h=Controller access), dummy response
   57h  FLAG  Send Write Command (ASCII "W"), Receive FLAG Byte
   00h  5Ah   Receive Memory Card ID1
   00h  5Dh   Receive Memory Card ID2
   MSB  (00h) Send Address MSB  ;\frame number (0..3FFh)
   LSB  (pre) Send Address LSB  ;/
   ...  (pre) Send Data Sector (128 bytes)
   CHK  (pre) Send Checksum (MSB xor LSB xor Data bytes)
   00h  5Ch   Receive Command Acknowledge 1
   00h  5Dh   Receive Command Acknowledge 2
   00h  4xh   Receive Memory End Byte (47h=Good, 4Eh=BadChecksum, FFh=BadSector)
*/
int PS1_write ()
{
  uint8_t cmd_write[142];
  FILE *input=fopen( "write.mcd", "rb" );		/* Open write.mcd in reading*/


  /* Start of frame to frame loop*/
  for (frame = PS1CARD_MIN_FRAME; frame <= PS1CARD_MAX_FRAME; frame++)
  {

  /* Split frame value in two*/
  msb = (uint8_t)((frame & 0xFF00) >> 8);
  lsb = (uint8_t)(frame & 0x00FF);
  /* Clean command write*/
  memset(cmd_write, 0, sizeof(cmd_write));
  /* Clean checksum*/
  checksum = 0x00;
  /* This is the write command for memory card (SCPH-1020) or PocketStation (SCPH-4000)*/
  /* first 4 byte are about ps3 memory card adapter protocol*/
  cmd_write[0] = PS3MCA_CMD_FIRST;			/* First command for ps3mca protocol*/
  cmd_write[1] = PS3MCA_CMD_TYPE_LONG;			/* PS1 type of command*/
  cmd_write[2] = 0x8a;					/* 142-4=138=8ah lenght of command. Can be cmd_write[2]=sizeof(cmd_write)-4*/
  cmd_write[3] = 0x00;					/* memset set all to 0x00, but I prefer to specify it a second time ;)*/
  /* this is the real command write with lenght of 138=0x8a*/
  cmd_write[4] = PS1CARD_CMD_MEMORY_CARD_ACCESS;	/* Memory Card Access, principal command for any action with any memory card*/
  cmd_write[5] = PS1CARD_CMD_WRITE;			/* Send write Command (ASCII "W")*/
  cmd_write[6] = 0x00;					/* Ask Memory Card ID1*/
  cmd_write[7] = 0x00;					/* Ask Memory Card ID2*/
  cmd_write[8] = msb;					/* First two significant digits of the frame value*/
  cmd_write[9] = lsb;					/* Last two significant digits of the frame value*/
  fread ( &cmd_write[10], 1, PS1CARD_FRAME_SIZE, input);/* Send Data Sector (128 bytes)*/
  for (c = 8; c < 8+PS1CARD_FRAME_SIZE; c++)		/* Loop started at msb(8) and finished at last data byte(137)*/
  {
	checksum = checksum ^ cmd_write[c];		/* Checksum = MSB xor LSB xor all Data bytes*/
  }
  cmd_write[138] = checksum;				/* Send Checksum  (MSB xor LSB xor all Data bytes)*/
  cmd_write[139] = 0x00;				/* Receive Command Acknowledge 1*/
  cmd_write[140] = 0x00;				/* Receive Command Acknowledge 2*/
  cmd_write[141] = 0x00;				/* Receive Memory End Byte (47h=Good, 4Eh=BadChecksum, FFh=BadSector)*/


  
  /* Send the message to endpoint with a 5000ms timeout. */
  res = libusb_bulk_transfer(handle, BULK_WRITE_ENDPOINT, cmd_write, sizeof(cmd_write), &numBytes, USB_TIMEOUT);
  if (res == 0)
  {
    /* See on screen what is transmitted for debug purpose*/
    #if DEBUG
    printf("%d bytes transmitted successfully  on frame %d.\n", numBytes, frame);
    #endif

    #if VERBOSE
    printf("Writing frame %d.\n", frame);
    #endif
  }
  else
  {
    fprintf(stderr, "Error sending message to device.\n");
  }


  /* Listen for a message.*/
  /* Wait up to 5 seconds for a message to arrive on endpoint*/
  res = libusb_bulk_transfer(handle, BULK_READ_ENDPOINT, ps1_ram_buffer, sizeof(ps1_ram_buffer), &numBytes, USB_TIMEOUT);
  if (0 == res)
  {
    if (numBytes <= sizeof(ps1_ram_buffer))
    {
        /* Verify if PS3mca send status succes code*/
        if (ps1_ram_buffer[0] == RESPONSE_CODE & ps1_ram_buffer[1] == RESPONSE_STATUS_SUCCES)
        {
	  #if DEBUG
          printf("Autentication verified on frame %d.\n", frame);
          #endif
        } 

        /* Verify if PS3mca send status wrong code*/
        else if (ps1_ram_buffer[0] == RESPONSE_CODE & ps1_ram_buffer[1] == RESPONSE_WRONG)    
        {
          fprintf(stderr, "Autentication failed on frame %d.\n", frame);
        }

        /* Other unknown PS3mca error*/
        else   
        {
          fprintf(stderr, "Unknown error on PS3mca protocol on frame %d.\n", frame);
        }


        /* Verify Memory End Byte (0x47h=Good)*/
        if (ps1_ram_buffer[141] == PS1CARD_REPLY_MEB_GOOD)
        {
	  #if DEBUG
          printf("Good Memory End Byte on frame %d.\n", frame);
          #endif
        }
 
        /* Verify Memory End Byte (0x4E=BadChecksum)*/
        else if (ps1_ram_buffer[141] == PS1CARD_REPLY_MEB_BAD_CHECKSUM)
        {
          fprintf(stderr, "Bad Checksum Memory End Byte on frame %d.\n", frame);
        }
 
        /* Verify Memory End Byte (0xFF=BadFrame)*/
        else if (ps1_ram_buffer[141] == PS1CARD_REPLY_MEB_BAD_FRAME)
        {
          fprintf(stderr, "Bad frame Memory End Byte on frame %d.\n", frame);
        }

	/* Verify Memory End Byte (0xFD=Reject write to Directory Entries of currently executed file)*/
        else if (ps1_ram_buffer[141] == POCKETSTATION_REPLY_REJECT_EXECUTED)
        {
          fprintf(stderr, "WARNING Reject write to Directory Entries of currently executed file on frame %d.\n", frame);
          fprintf(stderr, "aborting for prevent to delete the currently executed file.\n");
	  /* Clean and close the file input*/
	  fflush(input);
	  fclose(input);
	  /* Unmount the ps3mca*/
	  close_ps3mca();
	  /* Close program with error status*/
	  return -1;
        }

	/* Verify Memory End Byte (0xFE=Reject write to write-protected Broken Frame region)*/
        else if (ps1_ram_buffer[141] == POCKETSTATION_REPLY_REJECT_PROTECTED)
        {
          fprintf(stderr, "WARNING The write-protection is enabled by ComFlags.bit10 on frame %d.\n", frame);
          fprintf(stderr, "Please unable write protection.\nAborting...\n");
	  /* Clean and close the file input*/
	  fflush(input);
	  fclose(input);
	  /* Unmount the ps3mca*/
	  close_ps3mca();
	  /* Close program with error status*/
	  return -1;
        }

    }
    else
    {
      fprintf(stderr, "Received %d bytes, expected a maximum of %d  on frame %d.\n", numBytes, sizeof(ps1_ram_buffer), frame);
    }
  }


  /* Wait for give time to write, on original card (slower) this time is important.*/
  /* This time can make slower the writing data, if it's possible can be better implement directly the clock.*/
  //sleep(1);
  delay(50);


  /* Clean buffer.*/
  memset(ps1_ram_buffer, 0, sizeof(ps1_ram_buffer));

  /* End of frame to frame loop*/
  }

  /* Clean and close the file input*/
  fflush(input);
  fclose(input);

}
/* ----------------------------------------------------End of PS1 command write------------------------------------------------------*/









/*-----------------------------------------------------------Main program-----------------------------------------------------------*/
int main(int argc, char*argv[])
{
  /* Initialise libusb. */
  res = libusb_init(0);
  if (res != 0)
  {
    fprintf(stderr, "Error initialising libusb.\n");
    return 1;
  }

  /* Get the first device with the matching Vendor ID and Product ID. */
  handle = libusb_open_device_with_vid_pid(0, USB_VENDOR, USB_PRODUCT);
  if (!handle)
  {
    fprintf(stderr, "Unable to open device.\n");
    fprintf(stderr, "Please verify PS3mca CECHZM1 (SCPH-98042) connection.\n");
    return 1;
  }

  /* Check whether a kernel driver is attached to interface #0. If so, we'll 
   * need to detach it.
   */
  if (libusb_kernel_driver_active(handle, 0))
  {
    res = libusb_detach_kernel_driver(handle, 0);
    if (res == 0)
    {
      kernelDriverDetached = 1;
    }
    else
    {
      fprintf(stderr, "Error detaching kernel driver.\n");
      return 1;
    }
  }

  /* Claim interface #0. */
  res = libusb_claim_interface(handle, 0);
  if (res != 0)
  {
    fprintf(stderr, "Error claiming interface.\n");
    return 1;
  }


  /* Switch-case argv*/
  while (++(*argv))
  {
	if ( **argv == '-' )
	{
            switch (*argv[1])  
            {
		default:
		fprintf(stderr, "Unknown option %c\n", (*argv)[1]);
		break;

		case 'v':
		return PS3mca_verify_card ();
		break;

		case 's':
		return PS1_get_id ();
		break;

		case 'r':
		return PS1_read ();
		break;

		case 'w':
		return PS1_write ();
		break;

		case 'x':
		return PocketStation_get_id ();
		break;

		case 'z':
		return PocketStation_get_dir_date ();
		break;
            }
	}
  }

  /* Unmount the ps3mca*/
  close_ps3mca();

  return 0;
}
/*--------------------------------------------------------End of Main program-------------------------------------------------------*/


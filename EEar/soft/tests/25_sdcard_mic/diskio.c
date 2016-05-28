/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "main.h"
#include "diskio.h"		/* FatFs lower layer API */
#include <stdio.h>



DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
  return 0;
}

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
  return 0;
}



DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
  SD_Error Status = SD_ReadMultiBlocks(buff, (uint64_t)sector*512, 512, count);
  Status = SD_WaitReadOperation();
  while(SD_GetStatus() != SD_TRANSFER_OK);
  return Status == SD_OK ? RES_OK : RES_ERROR;
}


#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write */
)
{
  SD_Error Status = SD_WriteMultiBlocks((uint8_t*)buff, (uint64_t)sector*512, 512, count);
  Status = SD_WaitWriteOperation();
  while(SD_GetStatus() != SD_TRANSFER_OK);
  return Status == SD_OK ? RES_OK : RES_ERROR;
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
  return RES_OK;
}
#endif


DWORD get_fattime (void)
{
  return ((2016-1980)<<25)+(1<<21)+(1<<16);
}


/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include <stdio.h>


#define SDRIVE  "\\\\.\\F:"



/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
  return 0;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
  return 0;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	DWORD sector,	/* Sector address in LBA */
	UINT count		/* Number of sectors to read */
)
{
  DRESULT rc = 0;
  HANDLE h;

  printf("read %d %d %d\n",pdrv,sector,count);

  
  h = CreateFile(SDRIVE,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
  if ( h != INVALID_HANDLE_VALUE )
     {
       __int64 pos = (__int64)sector*512;
       
       if ( SetFilePointer(h,(DWORD)pos,((DWORD*)&pos)+1,FILE_BEGIN) != INVALID_SET_FILE_POINTER )
          {
            DWORD bytes = 0;
            if ( !ReadFile(h,buff,count*512,&bytes,NULL) || bytes != count*512 )
               {
                 rc = 1;
               }
          }

       CloseHandle(h);
     }
  else
    rc = 1;

  return rc;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	DWORD sector,		/* Sector address in LBA */
	UINT count			/* Number of sectors to write */
)
{
  DRESULT rc = 0;
  HANDLE h;

  printf("write %d %d %d\n",pdrv,sector,count);

  
  
  h = CreateFile(SDRIVE,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,0,NULL);
  if ( h != INVALID_HANDLE_VALUE )
     {
       __int64 pos = (__int64)sector*512;
       
       if ( SetFilePointer(h,(DWORD)pos,((DWORD*)&pos)+1,FILE_BEGIN) != INVALID_SET_FILE_POINTER )
          {
            DWORD bytes = 0;
            if ( !WriteFile(h,buff,count*512,&bytes,NULL) || bytes != count*512 )
               {
                 rc = 1;
               }
          }

       CloseHandle(h);
     }
  else
    rc = 1;

  return rc;
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
  printf("ioctl %d %d\n",pdrv,cmd);
  
  return 0;
}
#endif


DWORD get_fattime (void)
{
  return ((2016-1980)<<25)+(1<<21)+(1<<16);
}



#include <stdio.h>
#include "ff.h"


void main()
{
  FIL f;
  
  FRESULT res;

  FATFS ffs;


  
  res = f_mount(&ffs,"0:",1);
  if ( res != 0 )
    printf("f_mount error %d\n",res);

  
  if ( (res = f_open(&f,"0:test.txt",FA_READ|FA_WRITE|FA_OPEN_EXISTING)) == FR_OK )
     {
       printf("OK!\n");

       char s[111] = "";
       UINT rb = 0;
       f_read(&f,s,sizeof(s)-1,&rb);

       printf("%s\n",s);
       
       char c[111] = "123";
       UINT wb = 0;
       if ( f_write(&f,c,3,&wb) == FR_OK && wb == 3 )
        printf("write ok!\n");

       f_sync(&f);

       f_close(&f);
     }
  else
    printf("error %d\n",res);

}

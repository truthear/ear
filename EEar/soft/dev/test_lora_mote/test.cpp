
#include <windows.h>
#include <shlwapi.h>
#include <stdio.h>
#include <conio.h>
#include <string>


void PrintCommState(DCB dcb)
{
    //  Print some of the DCB structure values
    printf( "\nBaudRate = %d, ByteSize = %d, Parity = %d, StopBits = %d\n", 
              dcb.BaudRate, 
              dcb.ByteSize, 
              dcb.Parity,
              dcb.StopBits );
}


std::string GetCurrentTimeStr()
{
  SYSTEMTIME st;
  GetLocalTime(&st);

  char s[100];
  sprintf(s,"%02d:%02d:%02d",st.wHour,st.wMinute,st.wSecond);
  return s;
}


static const unsigned char char2hex_t[256] =
{
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,
  0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF
};


void Cmd(HANDLE h,const char *cmd,BOOL need_decode_answer=FALSE)
{
  if ( cmd )
   printf("> %s\n",cmd);
  
  DWORD wb = 0;
  if ( !cmd || (WriteFile(h,cmd,lstrlen(cmd),&wb,NULL) && wb == lstrlen(cmd) && WriteFile(h,"\r\n",2,&wb,NULL) && wb == 2) )
     {
       std::string rs;
       
       while (1)
       {
         char c;
         DWORD rb = 0;
         
         if ( ReadFile(h,&c,1,&rb,NULL) && rb == 1 )
            {
              if ( need_decode_answer )
                 {
                   if ( ((unsigned char)c >= '0' && (unsigned char)c <= '9') || ((unsigned char)c >= 'A' && (unsigned char)c <= 'F') )
                      {
                        rs += c;
                      }
                 }
              
              if ( c == '\r' )
                 {
                   printf(" [%s]",GetCurrentTimeStr().c_str());
                 }
              
              printf("%c",c);
              fflush(stdout);

              if ( c == '\n' )
              {
                if ( need_decode_answer && !rs.empty() )
                   {
                     if ( (rs.size() % 2) == 0 )
                        {
                          for ( int n = 0; n < rs.size()/2; n++ )
                              {
                                int symb = ((int)char2hex_t[(unsigned char)rs[n*2+0]] << 4) | (int)char2hex_t[(unsigned char)rs[n*2+1]];
                                printf("%c",symb);
                              }
                          printf("\n");
                        }
                   }
                
                break;
              }
            }
         else
            {
              printf("\nRead error\n");
              break;
            }
       }
     }
  else
   printf("Write failed\n");
}


void main()
{
  HANDLE h = CreateFile("\\\\.\\COM12",GENERIC_READ|GENERIC_WRITE,0/*FILE_SHARE_READ|FILE_SHARE_WRITE*/,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if ( h != INVALID_HANDLE_VALUE )
     {
       DCB dcb;
       ZeroMemory(&dcb,sizeof(dcb));
       dcb.DCBlength = sizeof(DCB);

       GetCommState(h,&dcb);
       dcb.BaudRate = 57600;
       SetCommState(h,&dcb);
       ZeroMemory(&dcb,sizeof(dcb));
       dcb.DCBlength = sizeof(DCB);
       GetCommState(h,&dcb);
       //PrintCommState(dcb);

       FILE *f = fopen("cmd.txt","rt");
       if ( f )
          {
            char s[500];
            
            while ( fgets(s,sizeof(s),f) )
            {
              StrTrim(s,"\r\n ");
              if ( s[0] && s[0] != ';' )
                 {
                   Cmd(h,s);
                 }
            }

            while (!kbhit())
            {
              #ifndef SENDER
              Cmd(h,"mac pause");
              Cmd(h,"radio rx 0");
              Cmd(h,NULL,TRUE);
              Cmd(h,"radio get snr");
              #else
              Cmd(h,"mac pause");
              Cmd(h,"radio tx 48484848484848484848484848484848484848484848484848484848484848484848484848484848");
              Cmd(h,NULL);
              Sleep(1000);
              #endif
            }

            fclose(f);
          }
       else
          printf("File not found\n");

       CloseHandle(h);
     }
  else
    printf("Error opening port\n");
}




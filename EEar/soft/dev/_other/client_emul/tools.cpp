
#include "include.h"



std::string Base64Encode(const void *buff,unsigned len)
{
  std::string rc;

  const char *b64t = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

  const unsigned char *p = (const unsigned char*)buff;

  while ( len >= 3 )
  {
    unsigned t = 0;
    t |= (unsigned)(*p++) << 16; 
    t |= (unsigned)(*p++) << 8;  
    t |= (unsigned)(*p++);       

    rc += b64t[(t >> 18) & 0x3F];
    rc += b64t[(t >> 12) & 0x3F];
    rc += b64t[(t >> 6) & 0x3F];
    rc += b64t[t & 0x3F];

    len -= 3;
  }

  if ( len == 2 )
     {
       unsigned t = 0;
       t |= (unsigned)(*p++) << 16; 
       t |= (unsigned)(*p++) << 8;  

       rc += b64t[(t >> 18) & 0x3F];
       rc += b64t[(t >> 12) & 0x3F];
       rc += b64t[(t >> 6) & 0x3F];
       rc += '=';
     }
  else
  if ( len == 1 )
     {
       unsigned t = 0;
       t |= (unsigned)(*p++) << 16; 

       rc += b64t[(t >> 18) & 0x3F];
       rc += b64t[(t >> 12) & 0x3F];
       rc += '=';
       rc += '=';
     }

  return rc;
}


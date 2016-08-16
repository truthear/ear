
#ifndef __AES_H__
#define __AES_H__

#ifdef __cplusplus
 extern "C" {
#endif


#ifndef _UINT32_T_DECLARED
typedef unsigned int uint32_t;
#endif

typedef struct
{
    int nr;                     /*!<  number of rounds  */
    uint32_t *rk;               /*!<  AES round keys    */
    uint32_t buf[68];           /*!<  unaligned data    */
} AESCONTEXT;


int aes_setkey_enc(AESCONTEXT *ctx, const unsigned char *key, unsigned int keysize); // keysize in bits!
void aes_crypt_ecb_enc(AESCONTEXT *ctx,const unsigned char input[16],unsigned char output[16]);

#ifdef USE_AES_DEC
int aes_setkey_dec(AESCONTEXT *ctx, const unsigned char *key, unsigned int keysize); // keysize in bits!
void aes_crypt_ecb_dec(AESCONTEXT *ctx,const unsigned char input[16],unsigned char output[16]);
#endif


#ifdef __cplusplus
}
#endif

#endif

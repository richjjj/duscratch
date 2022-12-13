#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <gmp.h>
//#include <gmpxx.h>

#define MODULU_BITS 1024
#define MODULU_BYTES MODULU_BITS / 8
//#define MODULU_BYTES 2000
//=================================================生成公钥-密钥对=========================================

void GenerateKey(mpz_t e, mpz_t d, mpz_t N)
{
  // 选择两个不同的大素数
  // 设置随机生成数
  gmp_randstate_t grst;
  gmp_randinit_default(grst);
  gmp_randseed_ui(grst, time(NULL));
  // 生成随机大素数
  mpz_t p, q, N1;
  mpz_inits(p, q, N1, NULL);
  //验证e和N的欧拉是否互斥
  mpz_t mol;
  mpz_init(mol);
  // 0 <= p,q <= 2^MODULU_BITS
  while (1)
  {
    mpz_urandomb(p, grst, MODULU_BITS / 2 - 1);
    mpz_urandomb(q, grst, MODULU_BITS / 2 + 1);
    // 调整为素数
    mpz_nextprime(p, p);
    mpz_nextprime(q, q);

    // N = p*q
    mpz_mul(N, p, q);

    // phi(N) = (p-1)*(q-1)
    mpz_sub_ui(p, p, 1);
    mpz_sub_ui(q, q, 1);
    mpz_mul(N1, p, q);

    // chose e
    // such that 1 < e < phi(N), and gcd(e, phi(N))=1
    // 1. 65537
    mpz_init_set_ui(e, 65537);

    // 求e的逆元, ed mod phi(N) = 1
    mpz_invert(d, e, N1);

    // gmp_printf("PUB: \ne = %Zx\nN = %Zx\n", e, N);
    // gmp_printf("PRI: \nd = %Zx\nN = %Zx\n", d, N);
    mpz_gcd(mol, N1, e);
    // gmp_printf("%Zd\n", mol);

    char buf[2 * MODULU_BYTES + 1] = {'\0'};
    //cout << "strlen(buf): " << strlen(buf) << endl;

    gmp_sprintf(buf, "%0Zx", N);

    // cout << "strlen(buf): " << strlen(buf) << endl;
    // cout << "buf: " << buf << endl;

    if (strlen(buf) == MODULU_BYTES * 2 && mpz_cmp_ui(mol, 1) == 0)
    {
      //mpz_clear(mol);
      break;
    }
  }

  //释放内存
  mpz_clears(p, q, N1, mol, NULL);
}

//=================================================生成公钥-密钥对=========================================

/*
 * OS2IP converts an octet string to a nonnegative integer
 * Input: X octet string to be converted
 * Output: x corresponding nonnegative integer
 */
void OS2IP(const char unsigned *X, mpz_t x, unsigned int xLen)
{
  assert(X != NULL);
  char *padding = (char *)malloc((2 * xLen + 1) * sizeof(char));
  padding[2 * xLen] = '\0';
  for (int i = 0; i < 2 * xLen; i++)
  {
    padding[i] = X[i];
  }

  mpz_set_str(x, (const char *)padding, 16);

  free(padding);
}

/*
 * I2OSP converts a nonnegative integer to an octet string of a specified length
 * Input: x -- nonnegation integer to be converted
 *        xLen -- intended length of the resulting octet string
 * Output: X corresponding octet string of length xLen
 * 
 * Error: "integer too large"
 */
void I2OSP(mpz_t x, unsigned int xLen, unsigned char *X)
{
  mpz_t temp;
  mpz_init(temp);
  //temp = 256
  mpz_set_ui(temp, 256);
  //temp = 256 ^ xLen
  mpz_pow_ui(temp, temp, xLen);
  //if x >= 256 ^ xLen
  if (mpz_cmp(x, temp) >= 0)
  {
    mpz_clear(temp);
    printf("ERROR: integer too large\n");
    exit(-1);
  }
  char *buf = (char *)malloc((2 * xLen + 1) * sizeof(char));
  buf[2 * xLen] = '\0';
  buf[0] = '\0';
  // gmp_printf("x is \n%Zx\n", x);
  gmp_sprintf(buf, "%0Zx", x);
  // printf("strlen(buf) = %d\n", strlen(buf));
  // printf("buf = %s\n", buf);

  //完善buf并将结果赋值给X
  //assert(2 * xLen == strlen(buf) + 3);
  int j = 2 * xLen - strlen(buf);
  // cout << "j is: " << j << endl;

  for (int i = 0; i < j; ++i)
  {
    X[i] = '0';
  }

  for (int i = j; i < 2 * xLen; ++i)
  {
    X[i] = buf[i - j];
  }

  // printf("I2OSP: X is %s", X);

  //释放内存
  free(buf);
  mpz_clear(temp);
}

/*
 * EME-PKCS1-v1_5 encoding:
 * a. Generate an octet string PS of length k - mLen - 3
 *    consisting of pseudo-randomly generated nonzero octets.
 *    The length of PS will be at least eight octets.
 * 
 * b. Concatenate PS, the message M, and other padding to form
 *    an encoded message EM of length k octets as
 * 
 */
void PKCS1_encoding(const unsigned char *M, int mLen, int k, unsigned char *EM)
{
  int PS_length = k - mLen - 3;
  int PS_int[PS_length];
  char buf[3] = {'\0'};
  char *PS_char = (char *)malloc((2 * PS_length + 1) * sizeof(char));
  PS_char[2 * PS_length] = '\0';
  PS_char[0] = '\0';
  // EM = (char*)malloc((2 * k + 1) * sizeof(char));
  // EM[2*k] = '\0';

  srand(time(0));
  for (int i = 0; i < PS_length; ++i)
  {
    PS_int[i] = 1 + rand() % 255;
    //cout << PS_int[i] << endl;
    sprintf(buf, "%02x", PS_int[i]);
    strcat(PS_char, buf);
  }

  EM[0] = '0';
  EM[1] = '0';
  EM[2] = '0';
  EM[3] = '2';
  EM[4] = '\0';
  strcat((char *)EM, PS_char);
  EM[4 + 2 * PS_length] = '0';
  EM[5 + 2 * PS_length] = '0';
  EM[6 + 2 * PS_length] = '\0';
  strcat((char *)EM, (const char *)M);

  //释放内存
  free(PS_char);
}

/*
 * EME-PKCS1-v1_5 decoding
 * Separate the encoded message EM into an octet string PS consisting of nonzero octets and a message M as
 *                 EM = 0x00 || 0x02 || PS || 0x00 || M
 */
void PKCS1_decoding(const unsigned char *EM, int k, unsigned char *M, int mLen)
{
  if (EM[0] != '0' || EM[1] != '0' || EM[2] != '0' || EM[3] != '2')
  {
    printf("ERROR: decryption error\n");
    exit(-1);
  }
  int PS_length = 0;
  int i = 4;
  while (i < 2 * k)
  {
    if (EM[i] == '0' && EM[i + 1] == '0')
    {
      break;
    }
    else
    {
      PS_length++;
      i = i + 2;
    }
  }

  if (PS_length < 8 || PS_length == k - 2)
  {
    printf("ERROR: decryption error\n");
    exit(-1);
  }

  for (int j = i + 2; j < 2 * k; ++j)
  {
    M[j - i - 2] = EM[j];
  }
  M[2 * k - i - 2] = '\0';
  mLen = strlen((const char *)M);

  // printf("PKCS1_decoding: EM is %s\n", EM);
  // printf("PKCS1_decoding: M is %s\n", M);
}

/*
 * RSAEP ((n, e), m)
 * Input: (n, e): RSA public key
 *         m: message representative, an integer between 0 and n - 1
 * Output: c ciphertext representative, an integer between 0 and n - 1
 * Error: "message representative out of range"
 */
void RSAEP(mpz_t n, mpz_t e, mpz_t m, mpz_t c)
{
  mpz_t n1;
  mpz_init(n1);
  //n1 = n - 1
  mpz_sub_ui(n1, n, 1);
  // m > n1
  if (mpz_cmp(m, n1) > 0)
  {
    mpz_clear(n1);
    printf("ERROR: message representative out of range\n");
    exit(-1);
  }

  //c = m^e % n
  mpz_powm(c, m, e, n);
  mpz_clear(n1);
}

/*
 * RSADP((n, d), c)
 * Input: (n, d): RSA private kry
 *         c: ciphertext representative, an integer between 0 and n - 1
 * Output: m: message representative, an integer between 0 and n - 1
 * Error: "ciphertext representative out of range"
 */
void RSADP(mpz_t n, mpz_t d, mpz_t c, mpz_t m)
{
  mpz_t n1;
  mpz_init(n1);
  //n1 = n - 1
  mpz_sub_ui(n1, n, 1);
  // m > n1
  if (mpz_cmp(c, n1) > 0)
  {
    mpz_clear(n1);
    printf("ERROR: message representative out of range\n");
    exit(-1);
  }

  //m = c^d % n
  mpz_powm(m, c, d, n);
  mpz_clear(n1);
}

//========================================================加密=================================================================

/* 
 * RSAES-PKCS1-V1_5-ENCRYPT ((n, e), M)
 * Iuput: (n, e): recipient’s RSA public key (k denotes the length in octets of the modulus n)
 *         M: message to be encrypted, an octet string of length mLen, where mLen <= k - 11
 * Ouput: C: ciphertext, an octet string of length k
 * Error: "message too long"
 */
void RSAES_PKCS1_V1_5_ENCRYPT(mpz_t n, mpz_t e, const unsigned char *M, unsigned char *C)
{
  unsigned int m2Len = strlen((const char *)M);
  unsigned int mLen = m2Len / 2;
  // printf("###################");
  // printf("%d",m2Len);
  // printf("###################");
  //1. Length checking
  if (mLen > MODULU_BYTES - 11)
  {
    printf("ERROR: message too long\n");
    exit(-1);
  }

  //2. EME-PKCS1-v1_5 encoding
  char EM[2 * MODULU_BYTES + 1] = {'\0'};
  PKCS1_encoding(M, mLen, MODULU_BYTES, (unsigned char *)EM);
  // cout << "EM is: " << EM << endl;
  // cout << "strlen(EM) is: " << strlen(EM) << endl;

  //3. RSA encryption:
  // 3.a OS2IP
  mpz_t m;
  mpz_init(m);
  OS2IP((unsigned char *)EM, m, MODULU_BYTES);
  //3.b RSAEP
  mpz_t c;
  mpz_init(c);
  RSAEP(n, e, m, c);

  //3.c I2OSP
  I2OSP(c, MODULU_BYTES, C);

  //释放内存
  mpz_clear(m);
  mpz_clear(c);
}

//========================================================加密=================================================================

//========================================================解密=================================================================

/* 
 * RSAES-PKCS1-V1_5-DECRYPT ((n, d), C)
 * Iuput: (n, d): recipient’s RSA private key 
 *         C: ciphertext to be decrypted, an octet string of length k
 *            where k is the length in octets of the RSA modulus n
 * Ouput: M: message, an octet string of length at most k - 11
 * Error: "decryption error"
 */
void RSAES_PKCS1_V1_5_DECRYPT(mpz_t n, mpz_t d, const unsigned char *C, unsigned char *M)
{
  //1. Length checking
  if (strlen((const char *)C) != 2 * MODULU_BYTES || MODULU_BYTES < 11)
  {
    printf("ERROR: decryption error\n");
    exit(-1);
  }

  // cout << " hello1" << endl;

  //2. RSA decryption
  //2.a OS2IP
  mpz_t c;
  mpz_init(c);
  OS2IP(C, c, MODULU_BYTES);
  //2.b RSADP
  mpz_t m;
  mpz_init(m);
  RSADP(n, d, c, m);
  //2.c I2OSP
  unsigned char *EM = (unsigned char *)malloc((2 * MODULU_BYTES + 1) * sizeof(char));
  EM[2 * MODULU_BYTES] = '\0';
  I2OSP(m, MODULU_BYTES, EM);

  //3. EME-PKCS1-v1_5 decoding
  int mLen = 0;
  PKCS1_decoding(EM, MODULU_BYTES, M, mLen);

  //释放内存
  free(EM);
  mpz_clear(c);
  mpz_clear(m);
}

//========================================================解密=================================================================
/*
    Crystal Space utility library: SHA256 class
    Original C code written by Christophe Devine (see below)
    Adapted for Crystal Space by Stefano Angeleri <weltall2@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
/*
 *  FIPS-180-2 compliant SHA-256 implementation
 *  written by Christophe Devine
 *
 *  This code has been distributed as PUBLIC DOMAIN.
 *
 *  Although normally licensed under the GPL on the author's web site,
 *  he has given me permission to distribute it as public domain as 
 *  part of md5deep. THANK YOU! Software authors are encouraged to
 *  use the GPL'ed version of this code available at:
 *  http://www.cr0.net:8040/code/crypto/sha256/ whenever possible.
 */

#include "cssysdef.h"

#include "csutil/sha256.h"

#include "csgeom/math.h"

#include <string.h>

namespace CS
{
namespace Utility
{
namespace Checksum
{
  #define GET_UINT32(n,b,i)                       \
  {                                               \
      (n) = ((uint32) (b)[(i)    ] << 24)         \
          | ((uint32) (b)[(i) + 1] << 16)         \
          | ((uint32) (b)[(i) + 2] <<  8)         \
          | ((uint32) (b)[(i) + 3]      );        \
  }

  #define PUT_UINT32(n,b,i)                       \
  {                                               \
      (b)[(i)    ] = (uint8) ((n) >> 24);         \
      (b)[(i) + 1] = (uint8) ((n) >> 16);         \
      (b)[(i) + 2] = (uint8) ((n) >>  8);         \
      (b)[(i) + 3] = (uint8) ((n)      );         \
  }

  SHA256::SHA256 ()
  {
    total[0] = 0;
    total[1] = 0;

    state[0] = 0x6A09E667;
    state[1] = 0xBB67AE85;
    state[2] = 0x3C6EF372;
    state[3] = 0xA54FF53A;
    state[4] = 0x510E527F;
    state[5] = 0x9B05688C;
    state[6] = 0x1F83D9AB;
    state[7] = 0x5BE0CD19;
  }

  void SHA256::Process (const uint8 data[64])
  {
    uint32 temp1, temp2, W[64];
    uint32 A, B, C, D, E, F, G, H;

    GET_UINT32(W[0],  data,  0);
    GET_UINT32(W[1],  data,  4);
    GET_UINT32(W[2],  data,  8);
    GET_UINT32(W[3],  data, 12);
    GET_UINT32(W[4],  data, 16);
    GET_UINT32(W[5],  data, 20);
    GET_UINT32(W[6],  data, 24);
    GET_UINT32(W[7],  data, 28);
    GET_UINT32(W[8],  data, 32);
    GET_UINT32(W[9],  data, 36);
    GET_UINT32(W[10], data, 40);
    GET_UINT32(W[11], data, 44);
    GET_UINT32(W[12], data, 48);
    GET_UINT32(W[13], data, 52);
    GET_UINT32(W[14], data, 56);
    GET_UINT32(W[15], data, 60);

  #define  SHR(x,n) ((x & 0xFFFFFFFF) >> n)
  #define ROTR(x,n) (SHR(x,n) | (x << (32 - n)))

  #define S0(x) (ROTR(x, 7) ^ ROTR(x,18) ^  SHR(x, 3))
  #define S1(x) (ROTR(x,17) ^ ROTR(x,19) ^  SHR(x,10))

  #define S2(x) (ROTR(x, 2) ^ ROTR(x,13) ^ ROTR(x,22))
  #define S3(x) (ROTR(x, 6) ^ ROTR(x,11) ^ ROTR(x,25))

  #define F0(x,y,z) ((x & y) | (z & (x | y)))
  #define F1(x,y,z) (z ^ (x & (y ^ z)))

  #define R(t)                                    \
  (                                               \
      W[t] = S1(W[t -  2]) + W[t -  7] +          \
             S0(W[t - 15]) + W[t - 16]            \
  )

  #define P(a,b,c,d,e,f,g,h,x,K)                  \
  {                                               \
      temp1 = h + S3(e) + F1(e,f,g) + K + x;      \
      temp2 = S2(a) + F0(a,b,c);                  \
      d += temp1; h = temp1 + temp2;              \
  }

    A = state[0];
    B = state[1];
    C = state[2];
    D = state[3];
    E = state[4];
    F = state[5];
    G = state[6];
    H = state[7];

    P(A, B, C, D, E, F, G, H, W[ 0], 0x428A2F98);
    P(H, A, B, C, D, E, F, G, W[ 1], 0x71374491);
    P(G, H, A, B, C, D, E, F, W[ 2], 0xB5C0FBCF);
    P(F, G, H, A, B, C, D, E, W[ 3], 0xE9B5DBA5);
    P(E, F, G, H, A, B, C, D, W[ 4], 0x3956C25B);
    P(D, E, F, G, H, A, B, C, W[ 5], 0x59F111F1);
    P(C, D, E, F, G, H, A, B, W[ 6], 0x923F82A4);
    P(B, C, D, E, F, G, H, A, W[ 7], 0xAB1C5ED5);
    P(A, B, C, D, E, F, G, H, W[ 8], 0xD807AA98);
    P(H, A, B, C, D, E, F, G, W[ 9], 0x12835B01);
    P(G, H, A, B, C, D, E, F, W[10], 0x243185BE);
    P(F, G, H, A, B, C, D, E, W[11], 0x550C7DC3);
    P(E, F, G, H, A, B, C, D, W[12], 0x72BE5D74);
    P(D, E, F, G, H, A, B, C, W[13], 0x80DEB1FE);
    P(C, D, E, F, G, H, A, B, W[14], 0x9BDC06A7);
    P(B, C, D, E, F, G, H, A, W[15], 0xC19BF174);
    P(A, B, C, D, E, F, G, H, R(16), 0xE49B69C1);
    P(H, A, B, C, D, E, F, G, R(17), 0xEFBE4786);
    P(G, H, A, B, C, D, E, F, R(18), 0x0FC19DC6);
    P(F, G, H, A, B, C, D, E, R(19), 0x240CA1CC);
    P(E, F, G, H, A, B, C, D, R(20), 0x2DE92C6F);
    P(D, E, F, G, H, A, B, C, R(21), 0x4A7484AA);
    P(C, D, E, F, G, H, A, B, R(22), 0x5CB0A9DC);
    P(B, C, D, E, F, G, H, A, R(23), 0x76F988DA);
    P(A, B, C, D, E, F, G, H, R(24), 0x983E5152);
    P(H, A, B, C, D, E, F, G, R(25), 0xA831C66D);
    P(G, H, A, B, C, D, E, F, R(26), 0xB00327C8);
    P(F, G, H, A, B, C, D, E, R(27), 0xBF597FC7);
    P(E, F, G, H, A, B, C, D, R(28), 0xC6E00BF3);
    P(D, E, F, G, H, A, B, C, R(29), 0xD5A79147);
    P(C, D, E, F, G, H, A, B, R(30), 0x06CA6351);
    P(B, C, D, E, F, G, H, A, R(31), 0x14292967);
    P(A, B, C, D, E, F, G, H, R(32), 0x27B70A85);
    P(H, A, B, C, D, E, F, G, R(33), 0x2E1B2138);
    P(G, H, A, B, C, D, E, F, R(34), 0x4D2C6DFC);
    P(F, G, H, A, B, C, D, E, R(35), 0x53380D13);
    P(E, F, G, H, A, B, C, D, R(36), 0x650A7354);
    P(D, E, F, G, H, A, B, C, R(37), 0x766A0ABB);
    P(C, D, E, F, G, H, A, B, R(38), 0x81C2C92E);
    P(B, C, D, E, F, G, H, A, R(39), 0x92722C85);
    P(A, B, C, D, E, F, G, H, R(40), 0xA2BFE8A1);
    P(H, A, B, C, D, E, F, G, R(41), 0xA81A664B);
    P(G, H, A, B, C, D, E, F, R(42), 0xC24B8B70);
    P(F, G, H, A, B, C, D, E, R(43), 0xC76C51A3);
    P(E, F, G, H, A, B, C, D, R(44), 0xD192E819);
    P(D, E, F, G, H, A, B, C, R(45), 0xD6990624);
    P(C, D, E, F, G, H, A, B, R(46), 0xF40E3585);
    P(B, C, D, E, F, G, H, A, R(47), 0x106AA070);
    P(A, B, C, D, E, F, G, H, R(48), 0x19A4C116);
    P(H, A, B, C, D, E, F, G, R(49), 0x1E376C08);
    P(G, H, A, B, C, D, E, F, R(50), 0x2748774C);
    P(F, G, H, A, B, C, D, E, R(51), 0x34B0BCB5);
    P(E, F, G, H, A, B, C, D, R(52), 0x391C0CB3);
    P(D, E, F, G, H, A, B, C, R(53), 0x4ED8AA4A);
    P(C, D, E, F, G, H, A, B, R(54), 0x5B9CCA4F);
    P(B, C, D, E, F, G, H, A, R(55), 0x682E6FF3);
    P(A, B, C, D, E, F, G, H, R(56), 0x748F82EE);
    P(H, A, B, C, D, E, F, G, R(57), 0x78A5636F);
    P(G, H, A, B, C, D, E, F, R(58), 0x84C87814);
    P(F, G, H, A, B, C, D, E, R(59), 0x8CC70208);
    P(E, F, G, H, A, B, C, D, R(60), 0x90BEFFFA);
    P(D, E, F, G, H, A, B, C, R(61), 0xA4506CEB);
    P(C, D, E, F, G, H, A, B, R(62), 0xBEF9A3F7);
    P(B, C, D, E, F, G, H, A, R(63), 0xC67178F2);

    state[0] += A;
    state[1] += B;
    state[2] += C;
    state[3] += D;
    state[4] += E;
    state[5] += F;
    state[6] += G;
    state[7] += H;
  }

  void SHA256::AppendInternal (const uint8* input, uint32 length)
  {
    uint32 left, fill;

    if(!length) return;

    left = total[0] & 0x3F;
    fill = 64 - left;

    total[0] += length;
    total[0] &= 0xFFFFFFFF;

    if(total[0] < length)
      total[1]++;

    if(left && length >= fill)
    {
      memcpy((void *)(buffer + left), (void *)input, fill);
      Process (buffer);
      length -= fill;
      input  += fill;
      left = 0;
    }

    while(length >= 64)
    {
      Process (input);
      length -= 64;
      input  += 64;
    }

    if(length)
    {
      memcpy((void *)(buffer + left), (void *)input, length);
    }
  }

  void SHA256::Append (const uint8* input, size_t length)
  {
  #if CS_PROCESSOR_SIZE == 32
    AppendInternal (input, length);
  #else
    // Hash data in 4GB chunks
    const uint32 u32_max = (uint32)~0;
    
    if (length <= u32_max)
    {
      AppendInternal (input, uint32 (length));
      return;
    }
    
    while (length > 0)
    {
      uint32 chunk = uint32 (csMin (length, size_t (u32_max)));
      AppendInternal (input, chunk);
      input += chunk;
      length -= chunk;
    }
  #endif
  }

  static const uint8 sha256_padding[64] =
    {
      0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
      0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

  void SHA256::Finish (uint8 digest[32])
  {
    uint32 last, padn;
    uint32 high, low;
    uint8 msglen[8];

    high = (total[0] >> 29) | (total[1] <<  3);
    low  = (total[0] <<  3);

    PUT_UINT32(high, msglen, 0);
    PUT_UINT32(low,  msglen, 4);

    last = total[0] & 0x3F;
    padn = (last < 56) ? (56 - last) : (120 - last);

    Append (sha256_padding, padn);
    Append (msglen, 8);

    PUT_UINT32(state[0], digest,  0);
    PUT_UINT32(state[1], digest,  4);
    PUT_UINT32(state[2], digest,  8);
    PUT_UINT32(state[3], digest, 12);
    PUT_UINT32(state[4], digest, 16);
    PUT_UINT32(state[5], digest, 20);
    PUT_UINT32(state[6], digest, 24);
    PUT_UINT32(state[7], digest, 28);
  }

  SHA256::Digest SHA256::Finish ()
  {
    Digest digest;
    Finish (digest.data);
    return digest;
  }


  SHA256::Digest SHA256::Encode(csString const& s)
  {
    return Encode(s.GetData(), (int)s.Length());
  }

  SHA256::Digest SHA256::Encode(const void* data, size_t nbytes)
  {
    SHA256 ctx;
    ctx.Append ((const uint8*)data, nbytes);
    return ctx.Finish ();
  }

  SHA256::Digest SHA256::Encode(const char* s)
  {
    return Encode(s, strlen(s));
  }

}//namespace Checksum
}//namespace Utility
}//namespace CS

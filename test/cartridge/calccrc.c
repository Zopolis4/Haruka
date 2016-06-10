/* 
   CRC-16-CCITT
 */

#include <stdio.h>
#include <stdint.h>

int
main ()
{
  uint16_t crc = 0xffff;
  int c;
  while ((c = getchar ()) != EOF)
    {
      uint16_t a = ((c << 8) ^ crc) & 0xff00;
      int i;
      for (i = 0; i < 8; i++)
	if (a & 0x8000)
	  a = 0x1021 ^ (a << 1);
	else
	  a <<= 1;
      crc = a ^ (crc << 8);
    }
  printf ("%c%c", crc >> 8, crc);
  return 0;
}

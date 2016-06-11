/* cat FONT_8.ROM FONT_9.ROM FONT_A.ROM | ./rom2txt |
   awk '{a[NR]=$2;b[NR]=$3;if(NF==3&&!c[$2])c[$2]=$3;}
   END{for(i=1;i<=NR;i++)print c[a[i]];}' | 
   sed -e 's/^88\([4-7]\)/98\1/' -e 's/^$/81a1/' */

#include <stdio.h>
#include <stdint.h>

int
main ()
{
  uint32_t addr = 0x80000;
  int c;
  while ((c = getchar ()) != EOF)
    {
      if (!(addr & 0x1f))
	printf ("%x  ", addr);
      printf ("%02x", c);
      if ((addr & 0x1f) == 0x1f)
	{
	  if (addr >= 0x80000 && addr < 0x80000 + 256 * 32)
	    {
	      uint32_t sjis = (addr >> 5) ^ 0x4000;
	      printf ("  %04x", sjis);
	    }
	  else
	    {
	      uint32_t sjis = (addr >> 5) ^ 0xc000;
	      /* Russian code conversion */
	      /* 8100-8120 <=> 8440-8460 */
	      /* 8200-8221 <=> 8470-8491 */
	      if (sjis >= 0x8100 && sjis <= 0x8120)
		sjis += 0x340;
	      else if (sjis >= 0x8200 && sjis <= 0x8221)
		sjis += 0x270;
	      else if (sjis >= 0x8440 && sjis <= 0x8460)
		sjis -= 0x340;
	      else if (sjis >= 0x8470 && sjis <= 0x8491)
		sjis -= 0x270;
	      uint8_t sjis1 = sjis >> 8, sjis2 = sjis;
	      if (sjis1 >= 0x81 && sjis1 <= 0x9f &&
		  sjis2 >= 0x40 && sjis2 <= 0xfc && sjis2 != 0x7f)
		printf ("  %04x", sjis);
	    }
	  printf ("\n");
	}
      addr++;
    }
  return 0;
}

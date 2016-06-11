/* cat FONT_8.ROM FONT_9.ROM FONT_A.ROM FONT_B.ROM | ./checksumtest */

#include <stdio.h>
#include <err.h>

static unsigned int
get (unsigned int len)
{
  unsigned int i;
  unsigned int sum = 0;
  for (i = 0; i < len; i++)
    {
      int c = getchar ();
      if (c == EOF)
	err (1, "getchar");
      sum += c;
    }
  return sum;
}

int
main ()
{
  printf ("80000-87FFF : %02x\n", get (0x8000) & 0xff);
  get (0x8000);
  unsigned int sum1 = 0, sum2 = 0, sum3 = 0;
  int i;
  for (i = 0; i < 16; i++)
    {
      get (0x800);
      sum1 += get (0x800);
      sum2 += get (0x800);
      sum3 += get (0x800);
    }
  printf ("90800-90FFF, 92800-92FFF, ..., AE800-AEFFF : %02x\n", sum1 & 0xff);
  printf ("91000-917FF, 93000-937FF, ..., AF000-AF7FF : %02x\n", sum2 & 0xff);
  printf ("91800-91FFF, 93800-93FFF, ..., AF800-AFFFF : %02x\n", sum3 & 0xff);
  return 0;
}

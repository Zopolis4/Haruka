#include <err.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

struct data
{
  uint16_t code;
  uint16_t bitmap[16];
  int index;
  int refcount;
  struct data *next;
};

struct order
{
  int index;
  struct order *next;
};

static void
add_to_list (struct data **list, uint32_t encoding, uint16_t bitmap[16], int n)
{
  uint16_t sjis;
  if (encoding < 256)
    sjis = encoding;
  else if (encoding >= 0x2100 && encoding <= 0x5eff &&
	   (encoding & 0xff) >= 0x21 && (encoding & 0xff) <= 0x7e)
    sjis = ((((encoding - 0x2100) >> 9) + 0x81) << 8) |
      ((encoding & 0xff) - 0x21 + 0x40 + ((encoding & 0x100) ? 0 : 0x5e) +
       (((encoding & 0x1ff) ^ 0x100) >= 0x60));
  else
    sjis = 0;
  if (n != 14 && n != 16)
    sjis = 0;
  if (sjis)
    {
      struct data *next = malloc (sizeof *next);
      if (!next)
	err (1, "malloc");
      next->code = sjis;
      int i;
      if (n == 14)
	{
	  /* 7x14 -> 16x16 conversion */
	  next->bitmap[0] = 0;
	  for (i = 0; i < 14; i++)
	    {
	      if (bitmap[i] & 1)
		warnx ("7x14 -> 8x16 conversion error bitmap %04x code %04x",
		       bitmap[i], sjis);
	      next->bitmap[i + 1] = bitmap[i] << 7;
	    }
	  next->bitmap[15] = 0;
	}
      else if (sjis == 0x81a1)
	/* code 81a1 is filled with ffff */
	for (i = 0; i < 16; i++)
	  next->bitmap[i] = 0xffff;
      else
	for (i = 0; i < 16; i++)
	  next->bitmap[i] = bitmap[i];
      next->index = -1;
      next->refcount = 0;
      next->next = *list;
      *list = next;
    }
}

static int
get_index_from_code (struct data *list, uint16_t code)
{
  struct data *p;
  static int next_index;
  if (!list)
    return next_index;
  for (p = list; p; p = p->next)
    {
      if (p->code == code)
	{
	  if (p->index < 0)
	    p->index = next_index++;
	  p->refcount++;
	  return p->index;
	}
    }
  warnx ("Code %04x not found", code);
  return code >= 256;
}

static void
output_bitmap_by_index (struct data *list, int index)
{
  for (; list; list = list->next)
    {
      if (list->index == index)
	{
	  int i;
	  for (i = 0; i < 16; i++)
	    {
	      putchar (list->bitmap[i] >> 8);
	      putchar (list->bitmap[i] & 255);
	    }
	  return;
	}
    }
  errx (1, "internal index not found");
}

static void
output_bitmap (struct data *list, int nindex)
{
  putchar (nindex & 255);
  putchar (nindex >> 8);
  int index;
  for (index = 0; index < nindex; index++)
    output_bitmap_by_index (list, index);
}

static void
output_order (struct order *o)
{
  for (; o; o = o->next)
    {
      uint16_t index = o->index;
      uint8_t count = 0;
      if (o->next)
	{
	  if (o->next->index == index)
	    {
	      for (; o->next && o->next->index == index && count < 255;
		   o = o->next)
		count++;
	      index |= 0x8000;
	    }
	  else if (o->next->index == index + 1)
	    {
	      for (; o->next && o->next->index == index + 1 && count < 255;
		   o = o->next)
		count++, index++;
	      index = (index - count) | 0x4000;
	    }
	}
      putchar (index & 255);
      putchar (index >> 8);
      if (count)
	putchar (count);
    }
}

int
main (int argc, char **argv)
{
  FILE *fp;
  int i;
  struct data *list = NULL;
  /* Read every bdf font into memory */
  for (i = 1; i < argc; i++)
    {
      fp = fopen (argv[i], "r");
      if (!fp)
	err (1, argv[i]);
      char word[32];
      uint32_t encoding = 0;
      uint16_t bitmap[16];
      int bitmap_offset = -1;
      while (fscanf (fp, "%31s", word) == 1)
	{
	  if (!strcmp (word, "ENCODING"))
	    {
	      if (fscanf (fp, "%" SCNu32, &encoding) != 1)
		encoding = 0;
	    }
	  else if (!strcmp (word, "BITMAP"))
	    bitmap_offset = 0;
	  else if (!strcmp (word, "ENDCHAR"))
	    {
	      add_to_list (&list, encoding, bitmap, bitmap_offset);
	      bitmap_offset = -1;
	    }
	  else if (bitmap_offset >= 0 && bitmap_offset < 16)
	    {
	      if (sscanf (word, "%" SCNx16, &bitmap[bitmap_offset++]) != 1)
		bitmap_offset = -1;
	    }
	  else
	    {
	      int c;
	      while ((c = getc (fp)) != EOF)
		if (c == '\n')
		  break;
	    }
	}
      fclose (fp);
    }
  if (!list)
    errx (1, "No font added");
  /* Assign index for bitmap 00 and ff */
  get_index_from_code (list, 0x20);
  get_index_from_code (list, 0x81a1);
  /* Read order */
  uint16_t sjis;
  struct order *o = NULL, **onext = &o;
  while (scanf ("%" SCNx16, &sjis) == 1)
    {
      *onext = malloc (sizeof **onext);
      if (!*onext)
	err (1, "malloc");
      (*onext)->index = get_index_from_code (list, sjis);
      (*onext)->next = NULL;
      onext = &(*onext)->next;
    }
  /* Make output */
  output_bitmap (list, get_index_from_code (NULL, 0));
  output_order (o);
  return 0;
}

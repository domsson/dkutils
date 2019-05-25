#include <stdio.h>
#include <stdlib.h>
#include <string.h>

long read_long (FILE *fp);
void write_long (FILE *fp, unsigned long x);
long file_length (char *path);
void write_short (FILE *fp, unsigned short x);
void write_bmp (char *fname, int width, int height, unsigned char *pal, 
		char *data,
		int red, int green, int blue, int mult);

int main (int argc, char **argv)
{
    char *buffer;
    FILE *datfp;
    FILE *palfp;
    char palette[768];
    char fname[50];
    int i;
    
    palfp = fopen ("main.pal", "rb");
    if (!palfp)
    {
	printf ("Can't open main.pal\n");
	exit (1);
    }
    fread (palette, 768, 1, palfp);
    fclose (palfp);
    buffer = malloc (557056);
    if (!buffer)
    {
	printf ("Out of memory.\n");
	exit (1);
    }
    for (i=0; i < 4; i++)
    {
	sprintf (fname, "tmapa00%d.dat", i);
	datfp = fopen (fname, "rb");
	if (!datfp)
	{
	    printf ("Can't open %s\n", fname);
	    exit (1);
	}
	fread (buffer, 557056, 1, datfp);
	fclose (datfp);
	sprintf (fname, "block%d.bmp", i);
	write_bmp (fname, 32*8, 2176, palette, buffer, 0, 1, 2, 4);
    }
}

void write_bmp (char *fname, int width, int height, 
		unsigned char *pal, char *data,
		int red, int green, int blue, int mult)
{
    long l;
    int i, j;
    FILE *out;
    
    out = fopen (fname, "wb");
    if (!out)
    {
	printf ("Can't open file %s. Aborting.\n", fname);
	exit (1);
    }
    l = width*height;
    fprintf (out, "BM");
    write_long (out, l+0x436);
    write_long (out, 0);
    write_long (out, 0x436);
    write_long (out, 40);
    write_long (out, width);
    write_long (out, height);
    write_short (out, 1);
    write_short (out, 8);
    write_long (out, 0);
    write_long (out, 0);
    write_long (out, 0);
    write_long (out, 0);
    write_long (out, 0);
    write_long (out, 0);
    
    for (i=0; i < 256; i++)
    {
	fputc (pal[i*3+blue]*mult, out);
	fputc (pal[i*3+green]*mult, out);
	fputc (pal[i*3+red]*mult, out);
	fputc (0, out);
    }
    
    for (i=1; i <= height; i++)
    {
	fwrite (data+(height-i)*width, width, 1, out);
	if (width & 3)
	    for (j=0; j < 4-(width&3); j++)
		fputc (0, out);
    }
    fclose (out);
}

void write_short (FILE *fp, unsigned short x)
{
    fputc ((int) (x&255), fp);
    fputc ((int) ((x>>8)&255), fp);
}

void write_long (FILE *fp, unsigned long x)
{
    fputc ((int) (x&255), fp);
    fputc ((int) ((x>>8)&255), fp);
    fputc ((int) ((x>>16)&255), fp);
    fputc ((int) ((x>>24)&255), fp);
}

long read_long (FILE *fp)
{
    long l;
    l = fgetc (fp);
    l += fgetc (fp)<<8;
    l += fgetc (fp)<<16;
    l += fgetc (fp)<<24;
    return l;
}

long file_length (char *path)
{
    FILE *fp;
    long ret;
    
    fp = fopen (path, "rb");
    if (!fp)
	return -1;
    fseek (fp, 0, SEEK_END);
    ret = ftell (fp);
    fclose (fp);
    return ret;
}

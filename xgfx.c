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
    char tab[6];
    long off;
    int width;
    int height;
    int picnum=0;
    int r, c, i;
    FILE *tabfp;
    FILE *datfp;
    FILE *palfp;
    FILE *animfp=NULL;
    char fname[50];
    char palette[768];
    char g;
    
    sprintf (fname, "%s.tab", argv[1]);
    tabfp = fopen (fname, "rb");
    if (!tabfp)
    {
	printf ("Can't open %s.\n", fname);
	return 1;
    }
    sprintf (fname, "%s.dat", argv[1]);
    datfp = fopen (fname, "rb");
    if (!datfp)
    {
	printf ("Can't open %s.\n", fname);
	return 1;
    }
    sprintf (fname, "%s.pal", argv[2]);
    palfp = fopen (fname, "rb");
    if (!palfp)
    {
	printf ("Can't open %s.\n", fname);
	return 1;
    }
    fread (tab, 6, 1, tabfp);
    fread (palette, 768, 1, palfp);
    animfp = fopen ("animate", "wb");
    while (!feof (tabfp))
    {
	printf ("\rWorking... picture number %d", picnum);
	sprintf (fname, "pic%d.bmp", picnum++);
	off = read_long (tabfp);
	if (off < 0)
	{
	    printf ("\nError - negative offset\n");
	    return 1;
	}
	width = fgetc (tabfp);
	if (width==-1)
	{
	    printf ("\nError - negative width\n");
	    return 1;
	}
	height = fgetc (tabfp);
	buffer = malloc (width*height);
	for (i=0; i < width*height; i++)
	    buffer[i]=0;
	r=0;
	c=0;
	fseek (datfp, off, SEEK_SET);
	while (r < height)
	{
	    off++;
	    g = (char) (fgetc (datfp)&255);
	    if (g < 0)
		c-=g;
	    else if (!g)
	    {
		c=0;
		r++;
	    }
	    else
	    {
		for (i=0; i < g; i++)
		{
		    if (r >= height || c >= width)
		    {
			printf ("\nError - colour leak\n");
			return 1;
		    }
		    buffer[(width*r)+c]=fgetc (datfp);
		    c++;
		    off++;
		}
	    }
	}
	write_bmp (fname, width, height, palette, buffer, 0, 1, 2, 4);
	free (buffer);
    }
    return 0;
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
	printf ("\nCan't open file %s. Aborting.\n", fname);
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

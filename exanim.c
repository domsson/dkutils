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

struct tab_entry
{
    long offset;
    long anim;
    int width;
    int height;
    int xoff;
    int yoff;
    long myst;
};

struct tab_entry read_tab(FILE *fp);

int main (int argc, char **argv)
{
    char *buffer;
    int picnum=0;
    int animnum=0;
    int r, c, ni, i;
    long curanim;
    long animwidth=0;
    long animheight=0;
    long animstart=0;
    FILE *tabfp;
    FILE *datfp;
    FILE *palfp;
    FILE *animfp=NULL;
    char fname[50];
    char palette[768];
    char g;
    int nanims;
    struct tab_entry tab;
    
    animfp = fopen ("anims.txt", "wb");
    fclose (animfp);
    tabfp = fopen ("creature.tab", "rb");
    if (!tabfp)
    {
	printf ("Can't open creature.tab.\n");
	return 1;
    }
    datfp = fopen ("creature.jty", "rb");
    if (!datfp)
    {
	printf ("Can't open creature.jty.\n");
	return 1;
    }
    palfp = fopen ("main.pal", "rb");
    if (!palfp)
    {
	printf ("Can't open main.pal.\n");
	return 1;
    }
    fread (palette, 768, 1, palfp);
    while (picnum < 9137)
    {
	fprintf (animfp, "anim%d.gif: ", animnum++);
	/* Find out the size of the animation */
	animstart = ftell (tabfp);
	tab = read_tab (tabfp);
	nanims=1;
	curanim=tab.anim;
	animwidth=tab.width+tab.xoff;
	animheight=tab.height+tab.yoff;
	
	tab = read_tab (tabfp);
	while (tab.anim == curanim)
	{
	    nanims++;
	    if (tab.width+tab.xoff > animwidth)
		animwidth=tab.width+tab.xoff;
	    if (tab.height+tab.yoff > animheight)
		animheight=tab.height+tab.yoff;
	    tab = read_tab (tabfp);
	}
	fseek (tabfp, animstart, SEEK_SET);
	
	for (ni=0; ni < nanims; ni++)
	{
	    tab = read_tab (tabfp);
	    sprintf (fname, "pic%d.bmp ", picnum);
	    fprintf (animfp, "%s ", fname);
	    printf ("\rWorking... frame number %d", picnum);
	    sprintf (fname, "pic%d.bmp", picnum++);
	    fflush (stdout);
	    buffer = malloc (animwidth*animheight);
	    for (i=0; i < animwidth*animheight; i++)
		buffer[i]=0;
	    r=tab.yoff;
	    c=tab.xoff;
	    fseek (datfp, tab.offset, SEEK_SET);
	    while (r-tab.yoff < tab.height)
	    {
		g = (char) (fgetc (datfp)&255);
		if (g < 0)
		    c-=g;
		else if (!g)
		{
		    c=tab.xoff;
		    r++;
		}
		else
		{
		    for (i=0; i < g; i++)
			buffer[(animwidth*r)+(c++)]=fgetc (datfp);
		}
	    }
	    write_bmp (fname, animwidth, animheight, 
		       palette, buffer, 0, 1, 2, 4);
	    free (buffer);
	}
	fprintf (animfp, "\n");
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

struct tab_entry read_tab(FILE *fp)
{
    struct tab_entry ret;
    
    if (feof (fp))
    {
	printf ("End of tab file!\n");
	exit (1);
    }
    ret.offset = read_long (fp);
    ret.width = fgetc (fp);
    ret.height = fgetc (fp);
    ret.anim = read_long (fp);
    ret.xoff = fgetc (fp);
    ret.yoff = fgetc (fp);
    ret.myst = read_long (fp);
    return ret;
}

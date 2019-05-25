#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char **argv)
{
    unsigned char buffer[8];
    char *bigbuf;
    FILE *fp, *in;
    unsigned char path[16];
    int i=0;
    unsigned long l;
    unsigned long off=0;
    
    
    if (argc != 2)
    {
	printf ("Must specify input file\n");
	return 1;
    }
    in = fopen (argv[1], "rb");
    if (!in)
    {
	printf ("Couldn't open %s\n", argv[1]);
	return 1;
    }
    
    while (!feof (in))
    {
	if (fread (buffer, 8, 1, in) != 1)
	{
	    printf ("Couldn't read 8 bytes\n");
	    return 1;
	}
	if (strncmp (buffer, "RIFF", 4))
	{
	    printf ("End of wave data. Finished.\n");
	    return 0;
	}
	l = (unsigned long) buffer[4];
	l+= ((unsigned long)buffer[5])<<8;
	l+= ((unsigned long)buffer[6])<<16;
	l+= ((unsigned long)buffer[7])<<24;
	bigbuf = malloc (l);
	if (!bigbuf)
	{
	    printf ("Out of memory\n");
	    return 1;
	}
	sprintf (path, "fx%d.wav", i);
	printf ("%s : %ld bytes\n", path, l);
	i++;
	fp = fopen (path, "wb");
	if (!fp)
	{
	    printf ("Can't open output file %s\n", path);
	    return 1;
	}
	fread (bigbuf, l, 1, in);
	fwrite (buffer, 8, 1, fp);
	fwrite (bigbuf, l, 1, fp);
	fclose (fp);
	l+=8;
	off+=(l+16)&0xfffffff0;
	fread (bigbuf, 16-(l&15), 1, in);
	free (bigbuf);
    }
    return 0;
}

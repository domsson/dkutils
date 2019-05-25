#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char lowerof(unsigned char letter);
char *stristr (const char * str1, const char * str2);

int main (int argc, char **argv)
{
    unsigned char buffer[8];
    unsigned char *bigbuf;
    FILE *fp, *in;
    char *x;
    int chunk;
    int i=0, j, n=0;
    unsigned long l, l2;
    
    if (argc != 2)
    {
	printf ("Usage: rsfx <filename>\n");
	return 1;
    }
    if (!stristr (argv[1], ".dat"))
    {
	printf ("File must be a \".dat\" file.\n");
	return 1;
    }
    in = fopen (argv[1], "rb");
    if (!in)
    {
	printf ("Couldn't open %s\n", argv[1]);
	return 1;
    }
    x = strdup (argv[1]);
    if (!x)
    {
	printf ("Out of memory.\n");
	return 1;
    }
    strcpy (stristr (x, ".dat"), ".new");
    fp = fopen (x, "wb");
    if (!fp)
    {
	printf ("Can't open file %s\n", x);
	return 1;
    }
    printf ("Writing to file %s\n", x);
    while (!feof (in))
    {
	printf ("\rWorking on sample: %d", n++);
	fflush (stdout);
	if (fread (buffer, 8, 1, in) != 1)
	{
	    printf ("\nUnexpected end of file\n");
	    return 1;
	}
	if (strncmp (buffer, "RIFF", 4))
	{
	    fwrite (buffer, 8, 1, fp);
	    /* Finished main bit, now just copy the rest verbatim */
	    while ((i=fgetc (in)) != EOF)
		fputc (i, fp);
	    printf ("\nDone.\n");
	    fclose (in);
	    fclose (fp);
	    return 0;
	}
	l = (unsigned long) buffer[4];
	l+= ((unsigned long)buffer[5])<<8;
	l+= ((unsigned long)buffer[6])<<16;
	l+= ((unsigned long)buffer[7])<<24;
	bigbuf = malloc (l);
	if (!bigbuf)
	{
	    printf ("\nOut of memory\n");
	    return 1;
	}
	fread (bigbuf, l, 1, in);
	fwrite (buffer, 8, 1, fp);
	if (l < 0x24)
	{
	    printf ("\nrData chunk too small\n");
	    return 1;
	}
	chunk=bigbuf[0x18];
	for (i=0; i < 0x24; i++)
	    fputc (bigbuf[i], fp);
	l2 = (unsigned long) bigbuf[0x20];
	l2+= ((unsigned long)bigbuf[0x21])<<8;
	l2+= ((unsigned long)bigbuf[0x22])<<16;
	l2+= ((unsigned long)bigbuf[0x23])<<24;
	if (l2%chunk)
	{
	    printf("\nSample doesn't contain a whole number of chunks.\n");
	    return 1;
	}
	for (i=0; i < l2/chunk; i++)
	    for (j=0; j < chunk; j++)
		fputc (bigbuf[0x24+l2-(i*chunk)-chunk+j], fp);
	/* Put the gumph at the end of the sound */
	for (i=l2+0x24; i < l; i++)
	    fputc (bigbuf[i], fp);
	l+=8;
	for (i=0; i < 16-(l&15); i++)
	{
	    j = fgetc (in);
	    if (j < 0)
	    {
		printf ("\nUnexpected end of file\n");
		return 1;
	    }
	    fputc (j, fp);
	}
	free (bigbuf);
    }
    printf ("\nUnexpected end of file\n");
    return 1;
}

char *stristr (const char * str1, const char * str2)
{
    char *cp = (char *) str1;
    char *s1, *s2;
    
    if ( !*str2 )
	return((char *)str1);
    
    while (*cp)
    {
	s1 = cp;
	s2 = (char *) str2;
	
	while ( *s1 && *s2 && !(lowerof((unsigned char) *s1)-
				lowerof((unsigned char) *s2)) )
	    s1++, s2++;
	
	if (!*s2)
	    return(cp);
	
	cp++;
    }
    
    return(NULL);
    
}

unsigned char lowerof(unsigned char letter)
{
    if( (letter>='A') && (letter<='Z'))
    {
	return(letter - ('A' - 'a'));
    }
    else
	return(letter);
}

	
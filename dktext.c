#include <stdlib.h>
#include <stdio.h>
#include <string.h>

char **text;
int size=0;
int capacity=0;

void save_file (char *fname);
void display_usage (void);
void load_file (char *fname);
void add_text (char *str);

int main (int argc, char **argv)
{
    static char message[4096];
    int i;
    int num=0;
    if (argc==1)
	display_usage();
    load_file (argv[1]);
    if (argc==2) /* List strings */
    {
	for (i=0; i < size; i++)
	    printf ("%d: %s\r\n\r\n", i, text[i]);
	return 0;
    }
    switch (argv[2][0])
    {
      case 'v': /* Display a single message */
	num = atoi (argv[3]);
	if (num==0 && argv[3][0]!='0')
	{
	    printf ("Error: %s is not a number.\r\n", argv[3]);
	    return 1;
	}
	if (num<0 || num >= size)
	{
	    printf ("Error: Message number must be in range 0-%d.\r\n", 
		    size-1);
	    return 1;
	}
	printf ("%d: %s\r\n", num, text[num]);
	return 0;
      case 'e': /* Edit a message */
	num = atoi (argv[3]);
	if (num==0 && argv[3][0]!='0')
	{
	    printf ("Error: %s is not a number.\r\n", argv[3]);
	    return 1;
	}
	if (num<0 || num >= size)
	{
	    printf ("Error: Message number must be in range 0-%d.\r\n", 
		    size-1);
	    return 1;
	}
	break;
      case 'a':
	num=-1;
	break;
      default:
	printf ("Error: Unknown option %s\r\n", argv[2]);
	display_usage();
	break;
    }
    printf ("Enter message: ");
    fgets (message, 4095, stdin);
    for (i=0; message[i]; i++)
    {
	if (message[i]<32)
	{
	    message[i]=0;
	    break;
	}
    }
    if (!*message)
    {
	printf ("Operation cancelled.\r\n");
	return 0;
    }
    if (num != -1)
	text[num]=strdup(message);
    else
    {
	printf ("Adding message %d\n", size);
	add_text (strdup (message));
    }
    save_file (argv[1]);
    return 0;
}

void save_file (char *fname)
{
    int i;
    FILE *fp;
    fp = fopen (fname, "wb");
    if (!fp)
    {
	printf ("Error: Can't open %s.\r\n", fname);
	exit (1);
    }
    for (i=0; i < size; i++)
	fprintf (fp, "%s%c", text[i], 0);
    fputc (0, fp);
    fclose (fp);
    printf ("File saved.\r\n");
}

void load_file (char *fname)
{
    static unsigned char buffer [4096]; /* Should be enough! */
    int l=0, g;
    FILE *fp;
    fp = fopen (fname, "rb");
    if (!fp)
    {
	printf ("Error: Can't open %s.\r\n", fname);
	exit (1);
    }
    while ((g=fgetc (fp)) != EOF)
    {
	buffer[l++]=(unsigned char)g;
	if (l==4096)
	{
	    printf ("Error: Length of message %d exceeds 4096 bytes.\r\n",
		    size);
	    exit (1);
	}
	if (!g)
	{
	    l=0;
	    add_text (strdup (buffer));
	}
    }
    fclose (fp);
    size--; /* Last message is always blank */
}

void add_text (char *str)
{
    if (size==capacity)
    {
	capacity+=5;
	if (size)
	    text = realloc (text, capacity*sizeof (char *));
	else
	    text = malloc (capacity*sizeof (char *));
	if (!text)
	{
	    printf ("Error: Out of memory.\r\n");
	    exit (1);
	}
    }
    text[size++]=str;
}

void display_usage (void)
{
    printf ("Usage:\r\n"
	    "  dktext <filename>            - List all messages\r\n"
	    "  dktext <filename> v <number> - Display specified message\r\n"
	    "  dktext <filename> e <number> - Replace message\r\n"
	    "  dktext <filename> a          - Add message to end\r\n");
    exit (0);
}

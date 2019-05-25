#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define NFILES 8

static char *file_exts[]={"apt", "clm", "dat", "own", "slb",
    "tng", "txt", "wib"};

struct level_info
{
    int valid;
    char *name;
    char *author;
    char *desc;
    char *date;
    int nplayers;
    int inf;
    long flen[NFILES];
};

#if !defined (unix)
#include <dos.h>
int fill_in_wildcards (int nargs, char ***arg);
#endif

long file_length (char *fname);
void show_usage(char *name);
void create_archive (int nargs, char **arg);
void view_archive (int nargs, char **arg);
void extract_archive (int nargs, char **arg);
FILE *open_archive (char *orig);
char *load_string (FILE *fp);
void write_string (FILE *fp, char *string);
void write_long (FILE *fp, long g);
long read_long (FILE *fp);
struct level_info load_info (FILE *fp);
void build_text_default (FILE *out, int nplayers);
void strip_crlf(char *string_in);

#if defined(unix) && !defined (GO32)
#define SEP '/'
#else
#define SEP '\\'
#endif

int main (int argc, char **argv)
{
    int mode = -1;
    if (argc == 1)
    {
	show_usage (argv[0]);
	return 0;
    }
    if (!strcmp (argv[1], "c"))
	mode=0;
    if (!strcmp (argv[1], "v"))
	mode=1;
    if (!strcmp (argv[1], "x"))
	mode=2;
    if (argc == 2)
    {
	printf ("Too few arguments\n\n");
	show_usage(argv[0]);
	return 1;
    }
    if (mode == -1)
    {
	printf ("Unknown action: %s\n\n", argv[1]);
	show_usage (argv[0]);
	return 1;
    }
    switch (mode)
    {
      case 0:
	create_archive (argc-2, argv+2);
	break;
      case 1:
	view_archive (argc-2, argv+2);
	break;
      case 2:
	extract_archive (argc-2, argv+2);
	break;
    }
    return 0;
}

void create_archive (int nargs, char **arg)
{
    char *oname;
    char name[40];
    char author[40];
    char *date;
    char desc[80];
    char nplayers[5];
    char *filenames[NFILES];
    char ext[5];
    char *buffer;
    int filelengths[NFILES];
    int n, i, j;
    int inf;
    time_t tim;
    FILE *out, *in;

    buffer = malloc (65536);
    if (!buffer)
    {
	printf ("Out of memory. Aborting.\n");
	exit (1);
    }
    
    tim = time (NULL);
    date = ctime (&tim);
    strip_crlf (date);
    if (nargs < 2)
    {
	printf ("Too few arguments given. Aborting.\n\n");
	show_usage ("dklevel");
	exit (1);
    }
    /* FIXME - PUT DOS HACK FOR map.* IN HERE */
#if !defined (unix)
    nargs = fill_in_wildcards (nargs, &arg);
#endif
    oname = malloc (strlen(arg[0])+4);
    if (!oname)
    {
	printf ("Out of memory. Aborting.\n");
	exit (1);
    }
    strcpy (oname, arg[0]);
    if (!strstr(oname, ".dk") && !strstr(oname, ".DK"))
	strcat (oname, ".dk");
    
    out = fopen (oname, "wb");
    if (!out)
    {
	printf ("Can't open output file %s. Aborting.\n", oname);
	exit (1);
    }
    printf ("Level name: ");
    fgets (name, 39, stdin);
    printf ("Level description: ");
    fgets (desc, 79, stdin);
    printf ("Author name: ");
    fgets (author, 39, stdin);
    strip_crlf (name);
    strip_crlf (desc);
    strip_crlf (author);
    n=0;
    while (n<1 || n>4)
    {
	printf ("Number of players: ");
	fgets (nplayers, 5, stdin);
	n=atoi (nplayers);
	if (n<1 || n>4)
	    printf ("Invalid number. Must be between 1 and 4.\n");
    }
    printf ("Creating archive %s\n", oname);
    fputc ('D', out);
    fputc ('K', out);
    fputc ('L', out);
    fputc ('V', out);
    write_string (out, name);
    write_string (out, author);
    write_string (out, desc);
    write_string (out, date);
    fputc (n, out);
    ext[0]='.';
    inf=-1;
    /* Create the inf entry */
    for (j=1; inf < 0 && j < nargs; j++)
    {
	if (strstr (arg[j], ".INF") || strstr (arg[j], ".inf"))
	{
	    in = fopen (arg[j], "rb");
	    if (!in)
		continue;
	    inf = fgetc (in);
	    fclose (in);
	}
    }
    if (inf == -1)
	inf=0;
    fputc (inf, out);
    /* Create the rest of the files */
    for (i=0; i < NFILES; i++)
    {
	strcpy (ext+1, file_exts[i]);
	filenames[i]=NULL;
	filelengths[i]=0;
	for (j=1; !filelengths[i] && j < nargs; j++)
	{
	    if (strstr (arg[j], ext))
	    {
		filenames[i]=arg[j];
		filelengths[i]=file_length(arg[j]);
	    }
	}
	for (j=1; j < 4; j++)
	    ext[j]=toupper(ext[j]);
	for (j=1; !filelengths[i] && j < nargs; j++)
	{
	    if (strstr (arg[j], ext))
	    {
		filenames[i]=arg[j];
		filelengths[i]=file_length(arg[j]);
	    }
	}
    }
    for (i=0; i < NFILES; i++)
	write_long (out, filelengths[i]);
    for (i=0; i < NFILES; i++)
    {
	if (filelengths[i])
	{
	    in = fopen (filenames[i], "rb");
	    if (!in)
	    {
		printf ("Can't open %s. Aborting.\n", filenames[i]);
		exit (1);
	    }
	    while (filelengths[i] > 65536)
	    {
		fread (buffer, 65536, 1, in);
		fwrite (buffer, 65536, 1, out);
		filelengths[i]-=65536;
	    }
	    fread (buffer, filelengths[i], 1, in);
	    fwrite (buffer, filelengths[i], 1, out);
	    filelengths[i]=-1;
	    fclose (in);
	}
    }
    fclose (out);
    printf ("Done.\n");
}

long file_length (char *fname)
{
    FILE *fp;
    long ret;
    
    fp = fopen (fname, "rb");
    if (!fp)
	return -1;
    fseek (fp, 0L, SEEK_END);
    ret = ftell (fp);
    fclose (fp);
    return ret;
}

void view_archive (int nargs, char **arg)
{
    struct level_info lev;
    int i;
    FILE *fp;
    if (nargs > 1)
	printf ("Too many filenames given. Ignoring all but the first.\n");
    fp = open_archive (arg[0]);
    lev = load_info (fp);
    if (!lev.valid)
    {
	printf ("Not a valid DK level archive\n");
	return;
    }
    printf ("Level name: %s\n", lev.name);
    printf ("Description: %s\n", lev.desc);
    printf ("Author: %s\n", lev.author);
    printf ("Creation date: %s\n", lev.date);
    printf ("Number of players: %d\n", lev.nplayers);
    printf ("Contains files: ");
    for (i=0; i < NFILES; i++)
	if (lev.flen[i])
	    printf ("%s ", file_exts[i]);
    printf ("\n");
}

void extract_archive (int nargs, char **arg)
{
    struct level_info lev;
    long i, j, k, m;
    unsigned char *buffer;
    char *path;
    int levnum;
    char *fname;
    unsigned char slb;
    FILE *fp;
    FILE *out;
    
    buffer = malloc (65536);
    if (!buffer)
    {
	printf ("Out of memory.\n");
	exit (1);
    }
    if (nargs > 3)
	printf ("Too many arguments given. Ignoring final ones.\n");
    if (nargs < 2)
    {
	printf ("Too few arguments given. Aborting.\n\n");
	show_usage ("dklevel");
	exit (1);
    }
    fp = open_archive (arg[0]);
    lev = load_info (fp);
    if (!lev.valid)
    {
	printf ("Not a valid DK level archive\n");
	exit (1);
    }
    levnum = atoi (arg[1]);
    if (!levnum)
    {
	printf ("Invalid level number. Aborting.\n");
	exit (1);
    }
    printf ("Extracting level.\n");
    if (nargs==3)
	path=arg[2];
    else
	path=".";
    fname=malloc (strlen(path)+20); /* Should be enough */
    
    /* Create inf file */
    sprintf (fname, "%s%cmap%.5d.inf", path, SEP, levnum);
    out = fopen (fname, "wb");
    if (!out)
    {
	printf ("Can't open output file %s. Aborting.\n", fname);
	exit (1);
    }
    fputc (lev.inf, out);
    fclose (out);
    
    /* Create the rest of the files */
    for (i=0; i < NFILES; i++)
    {
	if (lev.flen[i])
	{
	    sprintf (fname, "%s%cmap%.5d.%s", path, SEP, levnum, file_exts[i]);
	    out = fopen (fname, "wb");
	    if (!out)
	    {
		printf ("Can't open output file %s. Aborting.\n", fname);
		exit (1);
	    }
	    printf ("Extracting %s...\n", fname);
	    while (lev.flen[i] > 65536)
	    {
		fread (buffer, 65536, 1, fp);
		fwrite (buffer, 65536, 1, out);
		lev.flen[i]-=65536;
	    }
	    fread (buffer, lev.flen[i], 1, fp);
	    fwrite (buffer, lev.flen[i], 1, out);
	    lev.flen[i]=-1;
	    fclose (out);
	}
    }
    fclose (fp);
    for (i=0; i < NFILES; i++)
    {
	if (!lev.flen[i])
	{
	    sprintf (fname, "%s%cmap%.5d.%s", path, SEP, levnum, file_exts[i]);
	    out = fopen (fname, "wb");
	    if (!out)
	    {
		printf ("Can't open output file %s. Aborting.\n", fname);
		exit (1);
	    }
	    printf ("Imagining %s...\n", fname);
	    switch (i)
	    {
	      case 1: /* clm */
	      case 2: /* Dat */
	      case 3: /* Slb */
	      case 4: /* Own */
	      case 5: /* Tng */
		printf ("Can't recreate file. Aborting.\n");
		exit (1);
		break;
	      case 0: /* Apt */
		for (j=0; j < 4; j++)
		    fputc (0, out);
		break;
	      case 6: /* Txt */
		build_text_default (out, lev.nplayers);
		break;
	      case 7: /* Wib */
		/* We should already have the .slb file */
		sprintf (fname, "%s%cmap%.5d.slb", path, 
			 SEP, levnum);
		/* If we can't load the slb file */
		if (file_length(fname) != 14450 || !(fp=fopen(fname, "rb")))
		{
		    for (j=0; j < 65536; j++)
			fputc (1, out);
		}
		else
		{
		    fread (buffer, 14450, 1, fp);
		    fclose (fp);
		    for (j=0; j < 85; j++)
		    {
			for (m=0; m < 3; m++)
			{
			    for (k=0; k < 85; k++)
			    {
				slb = buffer[(j*85+k)*2];
				if (slb == 12 || slb==13)
				{
				    fputc (2, out);
				    fputc (2, out);
				    fputc (2, out);
				}
				else
				{
				    fputc (1, out);
				    fputc (1, out);
				    fputc (1, out);
				}
			    }
			    fputc (0, out);
			}
		    }
		    for (j=0; j < 256; j++)
			fputc (0, out);
		}
		break;
	    }		
	    fclose (out);
	}
    }
    free (buffer);
    printf ("Done.\n");
}

struct level_info load_info (FILE *fp)
{
    char buffer[4];
    struct level_info ret;
    int i;
    
    fread (buffer, 4, 1, fp);
    ret.valid=0;
    if (strncmp (buffer, "DKLV", 4))
	return ret;
    ret.name=load_string (fp);
    ret.author=load_string (fp);
    ret.desc=load_string (fp);
    ret.date=load_string (fp);
    ret.nplayers = fgetc (fp);
    ret.inf = fgetc (fp);
    for (i=0; i < NFILES; i++)
	ret.flen[i]=read_long (fp);
    ret.valid=1;
    return ret;
}

long read_long (FILE *fp)
{
    long ret;
    ret = fgetc (fp);
    ret+=fgetc(fp)<<8;
    ret+=fgetc(fp)<<16;
    ret+=fgetc(fp)<<24;
    return ret;
}

void write_long (FILE *fp, long g)
{
    fputc (g&0xff, fp);
    fputc ((g>>8)&0xff, fp);
    fputc ((g>>16)&0xff, fp);
    fputc ((g>>24)&0xff, fp);
}
    
void write_string (FILE *fp, char *string)
{
    int l;
    
    l = strlen (string);
    fputc (l&255, fp);
    fputc (l>>8, fp);
    fwrite (string, l, 1, fp);
}

char *load_string (FILE *fp)
{
    int len;
    char *ret;
    
    len = fgetc (fp);
    len += fgetc (fp)<<8;
    ret = malloc (len+1);
    if (!ret)
    {
	printf ("Out of memory.\n");
	exit (1);
    }
    fread (ret, len, 1, fp);
    ret[len]=0;
    return ret;
}

FILE *open_archive (char *orig)
{
    char *ret;
    char *aftersep;
    char *x;
    FILE *fp;
    
    ret = malloc (strlen (orig)+4);
    if (!ret)
    {
	printf ("Out of memory.\n");
	exit (1);
    }
    strcpy (ret, orig);
    aftersep = strrchr (ret, SEP);
    if (!aftersep)
	aftersep=ret;
    else
	aftersep++;
    fp = fopen (ret, "rb");
    if (fp)
	return fp;
    for (x=aftersep; *x; x++)
	*x=tolower (*x);
    fp = fopen (ret, "rb");
    if (fp)
	return fp;
    for (x=aftersep; *x; x++)
	*x=toupper (*x);
    fp = fopen (ret, "rb");
    if (fp)
	return fp;
    strcpy (ret, orig);
    strcat (ret, ".dk");
    fp = fopen (ret, "rb");
    if (fp)
	return fp;
    for (x=aftersep; *x; x++)
	*x=tolower (*x);
    fp = fopen (ret, "rb");
    if (fp)
	return fp;
    for (x=aftersep; *x; x++)
	*x=toupper (*x);
    fp = fopen (ret, "rb");
    if (fp)
	return fp;
    printf ("Can't open %s or any variation on it. Aborting.\n", orig);
    exit (1);
}

void show_usage(char *name)
{
    printf ("Usage:\n");
    printf ("%s c <archive> <files> - Create a Dungeon Keeper "
	    "level archive\n", 
	    name);
    printf ("%s v <archive> - View the description of a level archive\n",
	    name);
    printf ("%s x <archive> <level number> [path] - Extract an archive\n\n",
	    name);
    printf ("When extracting an archive, you can give the path "
	    "of the levels \ndirectory, eg c:\\games\\keeper\\levels\n");
}

void build_text_default (FILE *out, int nplayers)
{
    int i;
    static const char *creatures[]=
    {"BILE_DEMON", "DRAGON", "DARK_MISTRESS", "HELL_HOUND", 
	"TENTACLE", "DEMONSPAWN", "SORCEROR", "TROLL", "SPIDER",
	"BUG", "FLY", "ORC", NULL};
    
    static const char *rooms[]=
    {"TREASURE", "RESEARCH", "GARDEN", "LAIR", "GRAVEYARD",
	"PRISON", "TORTURE", "TRAINING", "SCAVENGER", "BARRACKS",
	"TEMPLE", "GUARD_POST",	"WORKSHOP", NULL};
    
    static const char *magic[]=
    {"HAND","OBEY", "IMP", "SLAP", "SIGHT", "CALL_TO_ARMS",
    "CAVE_IN", "HEAL_CREATURE", "HOLD_AUDIENCE", "CHICKEN",
    "SPEED", "LIGHTNING", "CONCEAL", "PROTECT", "DISEASE",
    "DESTROY_WALLS", "ARMAGEDDON", NULL};
    
    static const char *doors[]={"WOOD","BRACED", "STEEL", "MAGIC"};
    static const char *traps[]=
    {"ALARM", "POISON_GAS", "LAVA", "BOULDER", "LIGHTNING",
	"WORD_OF_POWER", NULL};

    fprintf (out, "SET_GENERATE_SPEED (400)\r\n");
    fprintf (out, "MAX_CREATURES(ALL_PLAYERS,17)\r\n");
    fprintf (out, "START_MONEY(ALL_PLAYERS,10000)\r\n");
    for (i=0; creatures[i]; i++)
	fprintf (out, "ADD_CREATURE_TO_POOL(%s,20)\r\n", creatures[i]);
    for (i=0; creatures[i]; i++)
	fprintf (out, "CREATURE_AVAILABLE(ALL_PLAYERS,%s,1,1)\r\n",
		 creatures[i]);
    for (i=0; rooms[i]; i++)
	fprintf (out, "ROOM_AVAILABLE(ALL_PLAYERS,%s,1,%d)\r\n",
		 rooms[i], i < 4 ? 1 : 0);
    for (i=0; magic[i]; i++)
	fprintf (out, "MAGIC_AVAILABLE(ALL_PLAYERS,POWER_%s,1,%d)\r\n",
		 magic[i], i < 4 ? 1 : 0);
    for (i=0; doors[i]; i++)
	fprintf (out, "DOOR_AVAILABLE(ALL_PLAYERS,%s,%d,0)\r\n",
		 doors[i], i < 4 ? 1 : 0);
    for (i=0; traps[i]; i++)
	fprintf (out, "TRAP_AVAILABLE(ALL_PLAYERS,%s,%d,0)\r\n",
		 traps[i], i < 4 ? 1 : 0);
    for (i=0; i < nplayers; i++)
    {
	fprintf (out, "IF(PLAYER%d, ALL_DUNGEONS_DESTROYED == 1)\r\n", i);
	fprintf (out, "WIN_GAME\r\n");
	fprintf (out, "DISPLAY_OBJECTIVE(0, PLAYER%d)\r\nENDIF\r\n", i);
    }
}

void strip_crlf(char *string_in)
{
    int i;
    unsigned char *string = (unsigned char *) string_in;
    
    for(i=strlen(string)-1;i>=0;i--)
    {
        if(string[i]<32)
            string[i]=0;
        else
            break;
    }
}

#if !defined (unix)
int fill_in_wildcards (int nargs, char ***arg)
{
    char **ret;
    struct find_t buffer;
    int retargs;
    char **names;
    int j;
    int flag;
    
    retargs=1;
    for (j=1; j < nargs; j++)
    {
	flag = _dos_findfirst ((*arg)[j], 0xe1, &buffer);
	retargs += (flag ? 0 :1);
	while (!flag)
	{
	    flag=_dos_findnext (&buffer);
	    retargs += (flag ? 0 : 1);
	}
    }
    ret = malloc (retargs*sizeof (char *));
    ret[0]=(*arg)[0];
    retargs=1;
    for (j=1; j < nargs; j++)
    {
	flag = _dos_findfirst ((*arg[j]), 0xe1, &buffer);
	while (!flag)
	{
	    ret[retargs++]=strdup (buffer.name);
	    flag=_dos_findnext (&buffer);
	}
    }
    *arg = ret;
    return retargs;
}
#endif

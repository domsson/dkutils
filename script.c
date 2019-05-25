#include <stdlib.h>
#include <stdio.h>
#include <slang.h>
#include <string.h>

#if defined(unix) && !defined (GO32)
#define SEPARATOR "/"
#else
#define SEPARATOR "\\"
#endif

#if defined(unix) && !defined(GO32)
# include <unistd.h>
# include <signal.h>
#include <sys/ioctl.h>
#include <termios.h>
#elif defined(MSDOS)
#include <dos.h>
#include <process.h>
#endif

#if defined(unix) && !defined(GO32)
int sigwinch (int sigtype);
#endif

#include "script.h"

#define NUMERIC 0
#define BOOLEAN 1
#define CHOICE 2
#define MENU 3
#define SPECIAL 4
#define STATIC 5

typedef struct _menuitem
{
    /* Specified */
    int type;
    int x, y;
    char *label;
    long *value;
    void *param;
    
    /* Set by addItem */
    int num;
    int up;
    int down;
    int left;
    int right;
    
} *menuitem;

typedef struct _menu 
{
    struct _menu *back;
    int nitems;
    menuitem *items;
    char *title;
    int first;
    int last;
} *menu;

union special
{
    long *longptr;
    long val;
};

void load (void);
void save (void);

char **script=NULL;
int snum=0;

void output (FILE *fp, char *com, long (*data)[5], char **names);
int get_pool (char *s);
int get_availability (char *s, long (*data)[5], char **from);
int parse_line (char *s);
union special make_special (long x);
union special get_special (long *x);
long *create_special (long x);
void process_special (long num);
void flush_keybuffer(void);
void draw_scr (void);
unsigned char get_key (void);
void init_slang(void);
void die (char *x);
void done(void);
void proc_key (void);
void action_key (int key);
void get_screen_size (void);
void addLabels (menu m);

int levnum=0;
int slang_going=0;
int finished=0;
int rows, cols;
int choicemin, choicemax, choicen, choicecols, choicew;

menuitem newItem (int x, int y, char *label, int type, long *value,
		  void *param);
menu newMenu(char *title);
void addItem (menu dest, menuitem src);
menu initMenus(void);
menu currentMenu;
int currentItem=0;
int itemOpen=0;
char tempValue[12]; /* Don't allow more than 12 digit numbers! */

long mpool[17]; /* Pool for monsters */
long hpool[14]; /* Pool for heroes */

long mavail[17][5]; /* Availability of heroes/monsters*/
long havail[14][5];

long davail[4][5]; /* Availability of doors */
long tavail[6][5]; /* Availability of traps */
long savail[18][5]; /* Availability of spells */
long ravail[14][5]; /* Availability of rooms */

static char *AVAIL[4]={"None", "Res.", "Got", NULL};

static char *TRAPS[6]={"Boulder", "Alarm", "Poison gas", "Lightning",
    "Word of Power", "Lava"};

static char *FILETRAPS[7]={"BOULDER", "ALARM", "POISON_GAS", "LIGHTNING",
    "WORD_OF_POWER", "LAVA", NULL};

static char *DOORS[4]={"Wooden", "Braced", "Iron", "Magic"};

static char *FILEDOORS[5]={"WOOD", "BRACED", "STEEL", "MAGIC", NULL};

static char *SPELLS[18]={"Hand of Evil", "Possess", "Create imp", "Must obey",
    "Slap", "Evil Sight", "Call to arms", "Cave in", "Heal creature",
    "Hold audience", "Lightning", "Speed", "Protect", "Conceal",
    "Disease", "Chicken", "Destroy walls", 
    "Armageddon"};
static char *FILESPELLS[19]= {"POWER_HAND","POWER_POSSESS","POWER_IMP",
    "POWER_OBEY","POWER_SLAP","POWER_SIGHT","POWER_CALL_TO_ARMS", 
    "POWER_CAVE_IN", "POWER_HEAL_CREATURE","POWER_HOLD_AUDIENCE", 
    "POWER_LIGHTNING", "POWER_SPEED","POWER_PROTECT", 
    "POWER_CONCEAL", "POWER_DISEASE","POWER_CHICKEN", "POWER_DESTROY_WALLS",
    "POWER_ARMAGEDDON", NULL};

static char *ROOMS[]={"Treasure room", "Lair", "Hatchery", "Training room",
    "Library", "Bridge", "Workshop", "Guard post", 
    "Prison", "Torture room", "Barracks", "Temple", "Graveyard", 
    "Scavenger room"};

static char *FILEROOMS[15]={"TREASURE", "LAIR", "GARDEN", "TRAINING",
    "RESEARCH", "BRIDGE", "WORKSHOP", "GUARD_POST", "PRISON", 
    "TORTURE", "BARRACKS", "TEMPLE", "GRAVEYARD", "SCAVENGER", NULL};

static char *MONSTERS[17]={"Imp", "Beetle", "Fly", "Spider",
    "Bile demon", "Demon spawn", "Orc", "Troll", "Skeleton", "Ghost",
    "Warlock", "Dragon", "Vampire", "Dark mistress", "Tentacle", 
    "Horned reaper", "Floating spirit"};

static char *FILEMONSTERS[18]={"IMP", "BUG", "FLY", "SPIDER", 
    "BILE_DEMON", "DEMONSPAWN", "ORC", "TROLL", "SKELETON", "GHOST",
    "SORCEROR", "DRAGON", "VAMPIRE", "DARK_MISTRESS", 
    "TENTACLE", "HORNY", "FLOATING_SPIRIT", NULL};
    
static char *HEROES[14]={"Tunneller", "Dwarf", "Thief", "Samurai",
    "Archer", "Monk", "Barbarian", "Wizard", "Giant", "Monk",
    "Fairy", "Witch", "Knight", "Avatar"};

static char *FILEHEROES[15]={"TUNNELLER", "DWARF", "THIEF", "SAMURAI",
    "ARCHER", "MONK", "BARBARIAN", "WIZARD", "GIANT", "MONK",
    "FAIRY", "WITCH", "KNIGHT", "AVATAR", NULL};
    
int main (int argc, char **argv)
{
    if (argc > 2)
    {
	printf ("Usage: script [level number]\n");
	exit (1);
    }
    if (argc == 2)
	levnum = atoi (argv[1]);
    load();
    currentMenu=initMenus();
    currentItem=currentMenu->first;
    init_slang();
    do 
    {
	draw_scr ();
	proc_key ();
    } while (!finished);
    done();
    return 0;
}

void draw_scr (void)
{
    int i;
    menu m;
    menuitem it;
    
    m=currentMenu;
    it = m->items[currentItem];
    SLtt_set_cursor_visibility (0);
    rows = SLtt_Screen_Rows;
    cols = SLtt_Screen_Cols;
    SLsmg_set_color (0);
    SLsmg_cls();
    SLsmg_set_color (1);
    SLsmg_gotorc (0, 40-strlen (m->title)/2);
    SLsmg_write_string (m->title);
    SLsmg_set_color (0);
    for (i=0; i < m->nitems; i++)
    {
	SLsmg_gotorc (m->items[i]->y, m->items[i]->x);
	if (currentItem==i && (m->items[i]->type==MENU
			       || m->items[i]->type==SPECIAL))
	    SLsmg_set_color (2);
	SLsmg_write_string (m->items[i]->label);
	SLsmg_write_char (' ');
	if (currentItem==i)
	    SLsmg_set_color (2);
	switch (m->items[i]->type)
	{
	  case NUMERIC:
	    if (itemOpen && currentItem == i)
		SLsmg_write_string (tempValue);
	    else
		SLsmg_printf ("%ld", *(m->items[i]->value));
	    break;
	  case BOOLEAN:
	    SLsmg_write_string (*m->items[i]->value ? "Yes" : "No");
	    break;
	  case CHOICE:
	    SLsmg_write_string (((char **)m->items[i]->param)
				[*m->items[i]->value]);
	    break;
	}
	if (currentItem==i)
	    SLsmg_set_color (0);
    }
    if (itemOpen && it->type==CHOICE)
    {
	int cr, cx, cy;
	
	SLsmg_set_color (3);
	cr = choicemax-choicemin > 10 ? 10 : choicemax-choicemin;
	cy = (rows-cr)/2;
	cx = (cols-choicecols*choicew)/2;
	SLsmg_fill_region (cy-1, cx-1, 
			   cr+2, choicew*choicecols+2, ' ');
	for (i=0; i < choicemax-choicemin; i++)
	{
	    SLsmg_gotorc ((i%10)+cy, (i/10)*choicew+cx);
	    if (i==choicen)
	    {
		SLsmg_set_color (4);
		SLsmg_write_string (((char **)it->param)[i+choicemin]);
		SLsmg_set_color (3);
	    }
	    else
		SLsmg_write_string (((char **)it->param)[i+choicemin]);
	}
    }
    
    /* Place cursor appropriately */
    if (itemOpen && it->type==NUMERIC)
	SLsmg_gotorc (it->y, it->x+strlen (it->label)+1+strlen (tempValue));
    else
	SLsmg_gotorc (SLtt_Screen_Rows-1, SLtt_Screen_Cols-1);
    SLsmg_refresh ();
}

void die (char *x)
{
    if (slang_going)
	done();
    fprintf (stderr, "%s\n", x);
    exit (1);
}

void done(void) 
{
    SLsmg_reset_smg ();
    SLang_reset_tty ();
    slang_going=0;
}

unsigned char get_key (void)
{
    unsigned char ret;
#if defined(unix) && !defined(GO32)
    if (update_required)
	update();
    safe_update = TRUE;
#endif
    ret = SLang_getkey();
#if defined(unix) && !defined(GO32)
    safe_update = FALSE;
#endif
    return ret;
}

void flush_keybuffer(void)
{
    while (SLang_input_pending (0))
	SLang_getkey();
}

void proc_key (void) 
{
    int g;
    g = get_key();
#if defined unix && !defined GO32
    if (g == 27 && SLang_input_pending (0)) /* Escaped character */
	g=1000+get_key();
    if (g==1000+'[')
	g=2000+get_key();
    if (g==1000+'O')
	g=3000+get_key();
    if (g>=2048 && g <= 2057) /* Yuk! */
    {
	int k;
	g -= 2048;
	while ((k=get_key()) != '~')
	    g=10*g+k-48;
	g+=4000;
    }
#else
    if (!g)
    {
	g=get_key();
	switch (g)
	{
	  case 73 : /* Page up */
	    g = 4005;
	    break;
	  case 81 : /* Page down */
	    g=4006; 
	    break;
	  case 83: /* Delete */
	    g=127;
	    break;
	  case 3:
	    g=0;
	    break;
	  case 72 :
	    g = 2065;
	    break;
	  case 75 :
	    g = 2068; /* Left */
	    break;
	  case 77 :
	    g = 2067; /* Right */
	    break;
	  case 80 : /* Down */
	    g=2066;
	    break;
	  case 59 : /* F1 */
	    g=4011;
	    break;
	}
    }
#endif
    action_key (g);
}

#if defined(unix) && !defined(GO32)
volatile int safe_update, update_required;

void update (void) {
    SLsmg_reset_smg ();
    get_screen_size ();
    SLsmg_init_smg ();
    draw_scr ();
}

int sigwinch (int sigtype) {
    if (safe_update)
	update();
    else
	update_required = TRUE;
    signal (SIGWINCH, (void *) sigwinch);
    return 0;
}
#endif

#ifdef __GNUC__
__inline__
#endif
union special make_special (long x)
{
    union special ret;
    ret.val=x;
    return ret;
}

#ifdef __GNUC__
__inline__
#endif
long *create_special (long x)
{
    union special y;
    y = make_special (x);
    return y.longptr;
}

#ifdef __GNUC__
__inline__
#endif
union special get_special (long *x)
{
    union special ret;
    ret.longptr=x;
    return ret;
}

void action_key (int key)
{
    int x, y;
    char **c;
    
    menu m;
    int it, l;
    
    m = currentMenu;
    it = currentItem;
    if (itemOpen && m->items[it]->type==NUMERIC)
	l = strlen (tempValue);
    else
	l=0;
    x = m->items[it]->x;
    y = m->items[it]->y;

    if (key == 'q' || key=='Q')
    {
	finished=1;
	return;
    }
    if (itemOpen)
	key = -(5000*m->items[it]->type+key);

    switch (key)
    {
      case 27 :
	if (m->back)
	{
	    currentMenu = m->back;
	    currentItem = currentMenu->first;
	}
	break;
      case 's' :
      case 'S' :
	save();
	break;
      case 2065 :
	currentItem=m->items[it]->up;
	break;
      case 2066 :
	currentItem=m->items[it]->down;
	break;
      case 2067 :
	currentItem=m->items[it]->right;
	break;
      case 2068 :
	currentItem=m->items[it]->left;
	break;
      case 4005 :
	currentItem=m->first;
	break;
      case 4006 :
	currentItem=m->last;
	break;
      case 13 : /* Enter */
	switch (m->items[it]->type)
	{
	  case MENU :
	    currentMenu = (menu) m->items[it]->param;
	    currentItem = currentMenu->first;
	    break;
	  case CHOICE :
	    itemOpen=1;
	    choicen=0;
	    c = (char **)m->items[it]->param;
	    choicemin=0;
	    while (!c[choicemin])
		choicemin++;
	    choicemax=choicemin;
	    choicew=0;
	    while (c[choicemax])
	    {
		if (strlen (c[choicemax])>choicew)
		    choicew=strlen(c[choicemax]);
		choicemax++;
	    }
	    choicew++;
	    choicecols = (choicemax-choicemin+9)/10;
	    break;
	  case NUMERIC :
	    itemOpen=1;
	    sprintf (tempValue, "%ld", *(m->items[it]->value));
	    break;
	  case BOOLEAN :
	    *(m->items[it]->value)=1-*(m->items[it]->value);
	    break;
	  case SPECIAL:
	    process_special (get_special(m->items[it]->value).val);
	    break;
	}
	break;
      case -(27+CHOICE*5000) :
	itemOpen=0;
	break;
      case -(27+NUMERIC*5000) :
	itemOpen=0;
	break;
      case -(13+CHOICE*5000) :
	*m->items[it]->value=choicemin+choicen;
	itemOpen=0;
	break;
      case -(13+NUMERIC*5000) :
	if (l)
	    *m->items[it]->value=atoi (tempValue);
	itemOpen=0;
	break;
      case -('0'+5000*NUMERIC) :
      case -('1'+5000*NUMERIC) :
      case -('2'+5000*NUMERIC) :
      case -('3'+5000*NUMERIC) :
      case -('4'+5000*NUMERIC) :
      case -('5'+5000*NUMERIC) :
      case -('6'+5000*NUMERIC) :
      case -('7'+5000*NUMERIC) :
      case -('8'+5000*NUMERIC) :
      case -('9'+5000*NUMERIC) :
	if (l<11)
	{
	    tempValue[l]=-(key+5000*NUMERIC);
	    tempValue[l+1]=0;
	}
	else
	    SLtt_beep();
	break;
      case -('-'+5000*NUMERIC) :
	if (!l)
	{
	    tempValue[0]='-';
	    tempValue[1]=0;
	}
	break;
      case -(8+NUMERIC*5000) :
      case -(127+5000*NUMERIC) : /* del/backspace */
	if (l)
	    tempValue [l-1]=0;
	break;
      case -(2065+CHOICE*5000) :
	if (choicen)
	    choicen--;
	else
	    choicen=choicemax-choicemin-1;
	break;
      case -(2066+CHOICE*5000) :
	choicen++;
	if (choicen==choicemax-choicemin)
	    choicen=0;
	break;
      case -(2067+CHOICE*5000) :
	choicen+=10;
	if (choicen >= choicemax-choicemin)
	    choicen-=10; /* FIXME */
	break;
      case -(2068+CHOICE*5000) :
	choicen-=10;
	if (choicen < 0)
	    choicen += 10; /* FIXME */
	break;
    }
}

void process_special (long num)
{
    int i;
    int j;
    int k;
    
    j = num%100;
    k=num/1000;
    /* 
     * Grotty, but it'll do for now... shame I don't 
     * have actual long **s earlier
     */
    switch ((num%1000)/100)
    {
      case 0:
	for (i=0; i < 5; i++)
	    savail[j][i]=k;
	break;
      case 1:
	for (i=0; i < 5; i++)
	    ravail[j][i]=k;
	break;
      case 2:
	for (i=0; i < 5; i++)
	    tavail[j][i]=k;
	break;
      case 3:
	for (i=0; i < 5; i++)
	    davail[j][i]=k;
	break;
      case 4:
	for (i=0; i < 5; i++)
	    havail[j][i]=k;
	break;
      case 5:
	for (i=0; i < 5; i++)
	    mavail[j][i]=k;
	break;
    }
}

void get_screen_size (void)
{
    int r = 0, c = 0;

#ifdef TIOCGWINSZ
    struct winsize wind_struct;

    if ((ioctl(1,TIOCGWINSZ,&wind_struct) == 0)
	|| (ioctl(0, TIOCGWINSZ, &wind_struct) == 0)
	|| (ioctl(2, TIOCGWINSZ, &wind_struct) == 0)) {
        c = (int) wind_struct.ws_col;
        r = (int) wind_struct.ws_row;
    }
#elif defined(MSDOS)
    union REGS regs;

    regs.h.ah = 0x0F;
    int86 (0x10, &regs, &regs);
    c = regs.h.ah;

    regs.x.ax = 0x1130, regs.h.bh = 0;
    int86 (0x10, &regs, &regs);
    r = regs.h.dl + 1;
#endif

    if ((r <= 0) || (r > 200)) r = 24;
    if ((c <= 0) || (c > 250)) c = 80;
    SLtt_Screen_Rows = r;
    SLtt_Screen_Cols = c;
}

void init_slang(void) 
{
    SLtt_get_terminfo();

    if (SLang_init_tty (ABORT, 1, 0) == -1) 
    {
	fprintf(stderr, "axe: SLang_init_tty: returned error code\n");
	exit (1);
    }
    SLang_set_abort_signal (NULL);

    get_screen_size ();
    SLtt_Use_Ansi_Colors = TRUE;
#if defined(unix) && !defined(GO32)
    signal (SIGWINCH, (void *) sigwinch);
#endif

    if (!SLsmg_init_smg ()) {
	fprintf(stderr, "axe: SLsmg_init_smg: returned error code\n");
	SLang_reset_tty ();
	exit (1);
    
    }
    SLtt_set_color (0, "buffer", "lightgray", "black");
    SLtt_set_color (1, "title", "yellow", "black");
    SLtt_set_color (2, "status", "yellow", "blue");
    SLtt_set_color (3, "escape", "yellow", "red");
    SLtt_set_color (4, "invalid", "yellow", "blue");
    SLtt_set_color (9, "cursor", "red", "white");
    slang_going=1;
}

menuitem newItem (int x, int y, char *label, int type, long *value,
		  void *param)
{
    menuitem ret;
    
    ret = malloc (sizeof (struct _menuitem));
    ret->type=type;
    ret->x=x;
    ret->y=y;
    ret->label=label;
    ret->value=value;
    ret->param=param;
    
    return ret;
}

void addItem (menu dest, menuitem src)
{
    int i, j, x, y, p, ip, jp;
    int n;
    dest->nitems++;
    dest->items=realloc (dest->items, dest->nitems*sizeof (menuitem));
    if (!dest->items)
	die ("Out of memory");
    dest->items[dest->nitems-1]=src;
    
    src->num=dest->nitems-1;
    src->up=-1;
    src->down=-1;
    src->left=-1;
    src->right=-1;

    if (src->type==STATIC)
	return;
    
    if (dest->first==-1 || src->y < dest->items[dest->first]->y ||
	(src->y == dest->items[dest->first]->y && 
	 src->x < dest->items[dest->first]->x))
	dest->first=src->num;
	
    if (dest->last==-1 || src->y > dest->items[dest->last]->y ||
	(src->y == dest->items[dest->last]->y && 
	 src->x > dest->items[dest->last]->x))
	dest->last=src->num;
    
    n=0;
    for (i=0; i < src->num; i++)
	if (dest->items[i]->type != STATIC)
	    n++;
    if (!n)
    {
	src->up=src->num;
	src->down=src->num;
	src->left=src->num;
	src->right=src->num;
    }
    else if (n==1)
    {
	for (i=0; i < src->num; i++)
	    if (dest->items[i]->type != STATIC)
		n=i;
	src->up=n;
	src->down=n;
	src->left=n;
	src->right=n;
	dest->items[n]->up=src->num;
	dest->items[n]->down=src->num;
	dest->items[n]->left=src->num;
	dest->items[n]->right=src->num;
    }
    else
    {
	x = src->x;
	y = src->y;
	p = (x<<8)+y;
	for (i=0; src->up==-1 && i < src->num; i++)
	{
	    if (dest->items[i]->type==STATIC)
		continue;
	    ip = (dest->items[i]->x<<8)+dest->items[i]->y;
	    if (ip==p)
	    {
		if (!slang_going)
		    printf ("Item location: %d,%d\n", x, y);
		die ("Eek! Two items at same place!");
	    }
	    j = dest->items[i]->up;
	    if (j==-1)
		die ("This shouldn't happen - no up");
	    jp = (dest->items[j]->x<<8)+dest->items[j]->y;
	    if ((jp < ip && p < ip && p > jp) ||
		((jp > ip) && (p < ip || p > jp)))
	    {
		src->up=j;
		src->down=i;
		dest->items[i]->up=src->num;
		dest->items[j]->down=src->num;
	    }
	}
	p = (y<<8)+x;
	for (i=0; src->left==-1 && i < src->num; i++)
	{
	    if (dest->items[i]->type==STATIC)
		continue;
	    ip = (dest->items[i]->y<<8)+dest->items[i]->x;
	    j = dest->items[i]->left;
	    if (j==-1)
		die ("This shouldn't happen - no left");
	    jp = (dest->items[j]->y<<8)+dest->items[j]->x;
	    if ((jp < ip && p < ip && p > jp) ||
		((jp > ip) && (p < ip || p > jp)))
	    {
		src->left=j;
		src->right=i;
		dest->items[i]->left=src->num;
		dest->items[j]->right=src->num;
	    }
	}
    }
}

menu newMenu(char *title)
{
    menu ret;
    ret = malloc (sizeof (struct _menu));
    ret->nitems=0;
    ret->items=NULL;
    ret->back=NULL;
    ret->title=title;
    ret->first=-1;
    ret->last=-1;
    return ret;
}    

void addLabels (menu m)
{
    addItem (m, newItem (25, 2, "Player 0", STATIC, NULL, NULL));
    addItem (m, newItem (35, 2, "1", STATIC, NULL, NULL));
    addItem (m, newItem (42, 2, "2", STATIC, NULL, NULL));
    addItem (m, newItem (49, 2, "3", STATIC, NULL, NULL));
    addItem (m, newItem (56, 2, "Heroes", STATIC, NULL, NULL));
    addItem (m, newItem (65, 2, "(All players)", STATIC, NULL, NULL));
}

menu initMenus(void)
{
    int i, j;
    menu root, monst, hero, rooms, trapdoors, misc, spells;
    int lx[5]={24, 34, 41, 48, 55};
    
    spells = newMenu ("Spell availability");
    addItem (spells, newItem (2, 2, "Spell", STATIC, NULL, NULL));
    addLabels (spells);
    for (i=0; i < 18; i++)
    {
	addItem (spells, newItem (2, i+4, SPELLS[i], STATIC, NULL, NULL));
	for (j=0; j < 5; j++)
	    addItem (spells, newItem (lx[j], i+4, "", CHOICE, &savail[i][j], AVAIL));
	addItem (spells, newItem (65, i+4, "None", SPECIAL, create_special (i), NULL));
	addItem (spells, newItem (71, i+4, "Res.", SPECIAL, create_special (i+1000), NULL));
	addItem (spells, newItem (76, i+4, "Got", SPECIAL, create_special (i+2000), NULL));
    }

    rooms = newMenu ("Room availability");
    addItem (rooms, newItem (2, 2, "Room", STATIC, NULL, NULL));
    addLabels (rooms);
    for (i=0; i < 14; i++)
    {
	addItem (rooms, newItem (2, i+4, ROOMS[i], STATIC, NULL, NULL));
	for (j=0; j < 5; j++)
	    addItem (rooms, newItem (lx[j], i+4, "", CHOICE, &ravail[i][j], AVAIL));
	addItem (rooms, newItem (65, i+4, "None", SPECIAL, create_special (i+100), NULL));
	addItem (rooms, newItem (71, i+4, "Res.", SPECIAL, create_special (i+1100), NULL));
	addItem (rooms, newItem (76, i+4, "Got", SPECIAL, create_special (i+2100), NULL));
    }
    
    trapdoors = newMenu ("Trap and door availability");
    addItem (trapdoors, newItem (2, 2, "Trap", STATIC, NULL, NULL));
    addLabels (trapdoors);
    for (i=0; i < 6; i++)
    {
	addItem (trapdoors, newItem (2, i+4, TRAPS[i], STATIC, NULL, NULL));
	for (j=0; j < 5; j++)
	    addItem (trapdoors, newItem (lx[j], i+4, "", CHOICE, &tavail[i][j], AVAIL));
	addItem (trapdoors, newItem (65, i+4, "None", SPECIAL, create_special (i+100), NULL));
	addItem (trapdoors, newItem (71, i+4, "Res.", SPECIAL, create_special (i+1200), NULL));
	addItem (trapdoors, newItem (76, i+4, "Got", SPECIAL, create_special (i+2200), NULL));
    }
    addItem (trapdoors, newItem (2, 11, "Door", STATIC, NULL, NULL));
    for (i=0; i < 4; i++)
    {
	addItem (trapdoors, newItem (2, i+13, DOORS[i], STATIC, NULL, NULL));
	for (j=0; j < 5; j++)
	    addItem (trapdoors, newItem (lx[j], i+13, "", CHOICE, &davail[i][j], AVAIL));
	addItem (trapdoors, newItem (65, i+13, "None", SPECIAL, create_special (i+300), NULL));
	addItem (trapdoors, newItem (71, i+13, "Res.", SPECIAL, create_special (i+1300), NULL));
	addItem (trapdoors, newItem (76, i+13, "Got", SPECIAL, create_special (i+2300), NULL));
    }
    
    hero = newMenu ("Hero availability");
    addLabels (hero);
    addItem (trapdoors, newItem (2, 2, "Hero", STATIC, NULL, NULL));
    addItem (hero, newItem (19, 2, "Pool", STATIC, NULL, NULL));
    for (i=0; i < 14; i++)
    {
	addItem (hero, newItem (2, i+4, HEROES[i], STATIC, NULL, NULL));
	addItem (hero, newItem (18, i+4, "", NUMERIC, &hpool[i], NULL));
	for (j=0; j < 5; j++)
	    addItem (hero, newItem (lx[j], i+4, "", BOOLEAN, &havail[i][j], NULL));
	addItem (hero, newItem (65, i+4, "No", SPECIAL, create_special (i+400), NULL));
	addItem (hero, newItem (71, i+4, "Yes", SPECIAL, create_special (i+1400), NULL));
    }

    monst = newMenu ("Monster availability");
    addLabels (monst);
    addItem (trapdoors, newItem (2, 2, "Monster", STATIC, NULL, NULL));
    addItem (monst, newItem (19, 2, "Pool", STATIC, NULL, NULL));
    for (i=0; i < 17; i++)
    {
	addItem (monst, newItem (2, i+4, MONSTERS[i], STATIC, NULL, NULL));
	addItem (monst, newItem (18, i+4, "", NUMERIC, &mpool[i], NULL));
	for (j=0; j < 5; j++)
	    addItem (monst, newItem (lx[j], i+4, "", BOOLEAN, &mavail[i][j], NULL));
	addItem (monst, newItem (65, i+4, "No", SPECIAL, create_special (i+500), NULL));
	addItem (monst, newItem (71, i+4, "Yes", SPECIAL, create_special (i+1500), NULL));
    }
	
    /* Create root menu */
    
    root=newMenu("Dungeon Keeper script editor");
    root->back=NULL;
    addItem (root, newItem (2, 2, "Spells", MENU, NULL, spells));
    addItem (root, newItem (2, 3, "Creatures (monsters)", MENU, NULL, monst));
    addItem (root, newItem (2, 4, "Creature (heroes)", MENU, NULL, hero));
    addItem (root, newItem (2, 5, "Rooms", MENU, NULL, rooms));
    addItem (root, newItem (2, 6, "Traps and doors", MENU, NULL, trapdoors));
    addItem (root, newItem (2, 7, "Miscellaneous", MENU, NULL, NULL));

    addItem (root, newItem (2, 9, "On the availability menus, \"Res.\" means you can research", 
			    STATIC, NULL, NULL));
    addItem (root, newItem (2, 10, "that item, \"Got\" means it is research already.", 
			    STATIC, NULL, NULL));
	     
    spells->back=root;
    rooms->back=root;
    monst->back=root;
    hero->back=root;
    trapdoors->back=root;
    
    return root;
}

void load (void)
{
    FILE *fp;
    char *buffer;
    
    int i, j;
    for (i=0; i < 5; i++)
    {
	for (j=0; j < 17; j++)
	    mavail[j][i]=0;
	for (j=0; j < 14; j++)
	    havail[j][i]=0;
	for (j=0; j < 4; j++)
	    davail[j][i]=0;
	for (j=0; j < 6; j++)
	    tavail[j][i]=0;
	for (j=0; j < 18; j++)
	    savail[j][i]=0;
	savail[1][i]=2; /* Special case */
	for (j=0; j < 14; j++)
	    ravail[j][i]=0;
    }
    for (i=0; i < 17; i++)
	mpool[i]=0;
    for (i=0; i < 14; i++)
	hpool[i]=0;
    if (levnum)
    {
	buffer = malloc (1024);
	if (!buffer)
	    die ("Out of memory");
	sprintf (buffer, "levels"SEPARATOR"map%05d.txt", levnum);
	fp = fopen (buffer, "rb");
	if (!fp)
	{
	    printf ("Can't open %s. Aborting.\n", buffer);
	    exit (1);
	}
	while (fgets (buffer, 1023, fp))
	{
	    if (!parse_line (buffer))
	    {
		if (!snum)
		    script = malloc (sizeof (char *));
		else
		    script = realloc (script, (snum+1)*sizeof (char *));
		if (!script)
		    die ("Out of memory");
		script[snum++]=strdup (buffer);
	    }
	}
	fclose (fp);
    }	
}

void save (void)
{
    FILE *fp;
    int i;
    fp = fopen ("newmap.txt", "wb");
    /* Do writing stuff here FIXME*/
    if (!fp)
	die ("Can't write to newmap.txt");
    for (i=0; i < 17; i++)
	if (mpool[i])
	    fprintf (fp, "ADD_CREATURE_TO_POOL(%s,%ld)\r\n", 
		     FILEMONSTERS[i], mpool[i]);
    for (i=0; i < 14; i++)
	if (hpool[i])
	    fprintf (fp, "ADD_CREATURE_TO_POOL(%s,%ld)\r\n", 
		     FILEHEROES[i], hpool[i]);
    output (fp, "ROOM_AVAILABLE", ravail, FILEROOMS);
    output (fp, "MAGIC_AVAILABLE", savail, FILESPELLS);
    output (fp, "CREATURE_AVAILABLE", mavail, FILEMONSTERS);
    output (fp, "CREATURE_AVAILABLE", havail, FILEHEROES);
    output (fp, "TRAP_AVAILABLE", tavail, FILETRAPS);
    output (fp, "DOOR_AVAILABLE", davail, FILEDOORS);
    for (i=0; i < snum; i++)
	fputs (script[i], fp);
    fclose (fp);
}

void output (FILE *fp, char *com, long (*data)[5], char **names)
{
    int i, j;
    static char *players[5]={"PLAYER0", "PLAYER1", "PLAYER2",
	"PLAYER3", "PLAYER_GOOD"};
    static char *out[3]={"0,0", "1,0", "1,1"};
    int flag;
    for (i=0; names[i]; i++)
    {
	flag=1;
	for (j=1; j < 5; j++)
	    if (data[i][j] != data[i][0])
		flag=0;
	if (flag && data[i][j])
	    fprintf (fp, "%s(ALL_PLAYERS,%s,%s)\r\n", 
		     com, names[i], out[data[i][0]]);
	else
	{
	    for (j=0; j < 5; j++)
		if (data[i][j])
		    fprintf (fp, "%s(%s,%s,%s)\r\n", 
			     com, players[j], names[i], out[data[i][j]]);
	}
    }
}

int parse_line (char *s)
{
    /* Test various cases */
    if (strstr (s, "ROOM_AVAILABLE"))
	return get_availability (s, ravail, FILEROOMS);
    if (strstr (s, "MAGIC_AVAILABLE"))
	return get_availability (s, savail, FILESPELLS);
    if (strstr (s, "CREATURE_AVAILABLE"))
    {
	if (get_availability (s, mavail, FILEMONSTERS))
	    return 1;
	else
	    return get_availability (s, havail, FILEHEROES);
    }    
    if (strstr (s, "TRAP_AVAILABLE"))
	return get_availability (s, tavail, FILETRAPS);
    if (strstr (s, "DOOR_AVAILABLE"))
	return get_availability (s, davail, FILEDOORS);
    if (strstr (s, "ADD_CREATURE_TO_POOL"))
	return get_pool (s);
    return 0;
}

int get_pool (char *s)
{
    char *x;
    int i;
    for (i=0; i < 17; i++)
    {
	x = strstr (s, FILEMONSTERS[i]);
	if (x)
	{
	    x+=strlen (FILEMONSTERS[i])+1;
	    mpool[i]=atoi (x);
	    return 1;
	}
    }
    for (i=0; i < 14; i++)
    {
	x = strstr (s, FILEHEROES[i]);
	if (x)
	{
	    x+=strlen (FILEHEROES[i])+1;
	    mpool[i]=atoi (x);
	    return 1;
	}
    }
    return 0;
}

int get_availability (char *s, long (*data)[5], char **from)
{
    int i;
    char *x;
    char *a;
    char *b;
    int l;
    int j=-1;
    int t;
    static char *players[6]={"PLAYER0", "PLAYER1", "PLAYER2",
	"PLAYER3", "PLAYER_GOOD", "ALL_PLAYERS"};
    for (i=0; i < 6; i++)
	if (strstr (s, players[i]))
	    j=i;
    if (j==-1)
	return 0;
    for (i=0; from[i]; i++)
    {
	if ((x=strstr (s, from[i])))
	{
	    l = strlen (from[i]);
	    x+=l;
	    a = strchr (x, ',');
	    a++;
	    b = strchr (x, ',');
	    b++;
	    t = (*a=='0' ? 0 : 1);
	    if (*b=='1')
		t=2;
	    if (j==5)
		for (l=0; l < 5; l++)
		    data[i][l]=t;
	    else
		data[i][j]=t;
	}
    }
    return 1;
}

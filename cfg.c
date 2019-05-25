#include <stdlib.h>
#include <stdio.h>
#include <slang.h>
#include <string.h>

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

#include "cfg.h"

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

static char *ROOMS[]={"None", "Portal", "Treasure", "Library", "Prison",
    "Torture", "Training", "Heart", "Workshop", "Scavenger",
    "Temple", "Graveyard", "Barracks", "Hatchery", "Lair",
    "Bridge", "Guard post", NULL};
static char *CREATURES[]={"None", "Wizard", "Barbarian",
    "Archer", "Monk", "Dwarf", "Knight", "Avatar", "Tunneller",
    "Witch", "Giant", "Fairy", "Thief", "Samurai", "Horned Reaper",
    "Skeleton", "Troll", "Dragon", "Demon Spawn", "Fly", "Dark Mistress",
    "Warlock", "Bile Demon", "Imp", "Beetle", "Vampire", "Spider",
    "Hell Hound", "Ghost", "Tentacle", "Orc", "Floating spirit", NULL};
static char *POWERS[]={"Hand of Evil", "Create imp", "Must obey",
    "Slap", "Evil Sight", "Call to arms", "Cave in", "Heal creature",
    "Hold audience", "Lightning", "Speed", "Protect", "Conceal",
    "Disease", "Chicken", "Destroy walls", "Time bomb", "Possess",
    "Armageddon", NULL};
static char *SHOTS[]={"Fireball", "Firebomb", "Freeze", "Lightning",
    "Poison cloud", "Nav. Missile", "Flame breathe", "Wind", "Missile",
    "Slow", "Grenade", "Drain", "Hail storm", "Arrow", "Boulder",
    "God lightning", "Spike", "Vortex", "Alarm", "Solid boulder",
    "Swing sword", "Swing fist", "Dig", "Lightning ball", "Group",
    "Disease", "Chicken", "Time bomb", "Trap Lightning", NULL};
static char *INSTANCE[]={"None", "Swing sword", "Swing fist", 
    "Escape", "Fire arrow", "Fireball",	"Fire bomb", "Freeze", 
    "Armour", "Lightning", "Rebound", "Heal", "Poision cloud", 
    "Invisibility", "Teleport", "Speed", "Slow", "Drain", "Fear", 
    "Missile", "Nav. Missile", "Flame breath", "Wind", "Light", 
    "Fly", "Sight", "Grenade", "Hailstorm", "Word of Power", 
    "Fart", "Dig", "Pretty path", "Destroy area", "Tunnel", 
    "Wave", "Reinforce", "Eat", "Attack slab", "Damage wall", 
    "1st person dig", "Cast group", "Cast disease", "Cast chicken", 
    "Cast time bomb", "Moan", "Tortured", NULL};
static char *SPELLS[]={"Fireball", "Fire bomb", "Freeze",
    "Armour", "Lightning", "Rebound", "Heal", "Poison cloud",
    "Invisibility", "Teleport", "Speed", "Slow", "Drain", "Fear",
    "Missile", "Nav. Missile", "Flame breath", "Wind", "Light",
    "Fly", "Sight", "Grenade", "Hailstorm", "Word of Power",
    "Crazy gas", "Disease", "Chicken", "Time bomb", NULL};

static char *JOBS[]={"None", "Tunnel", "Dig", "Research", "Train",
    "Manufacture", "Scavenge", "Kinky torture", "Fight", "Seek enemy",
    "Guard", "Group", "Barrack", "Temple", "Freeze prisoners", "Explore",
    NULL};

static char *ALIGN_JOBS[]={"None            ", "Tunnel          ", 
    "Dig             ", "Research        ", "Train           ", 
    "Manufacture     ", "Scavenge        ", "Kinky torture   ", 
    "Fight           ", "Seek enemy      ", "Guard           ", 
    "Group           ", "Barrack         ", "Temple          ", 
    "Freeze prisoners", "Explore         ",
    NULL};


static char *ANGER_JOBS[]={"Kill creatures", "Destroy rooms",
    "Leave dungeon", "Steal gold", "Damage walls", "Mad psycho", "Persuade",
    "Join enemy", NULL};

static char *SLABS[]={"None", "Gold", "Rock", "None", "None", "None",
    "None", "None", "None", "None", "None", "Path", "Lava", "Water"};

static char *ATTACKS[]={"None", "Melee", "Ranged", NULL};
static char *EYES[]={"Normal", "Wibble", "Fish eye", "Compound",
    "Water", "Double Water", "Dark Water", "Blob", "2xSmoke",
    "Edge smoke", "2xshit", "Quake", "Smoke", "Dog", "Vampire", NULL};

static char *BASIC_STATS[]={"Health", "Strength", "Armour", "Dexterity", 
    "Fear", "Defence", "Luck", "Pay", "Recovery", "Visual Range", 
    "Max Angle Change", "Eye Height", "Size XY", "Size YZ", 
    "Flying", "Thing Size XY", "Thing Size YZ", "Hearing", 
    "Bleeds", "Affected By Wind", "Can See Invisible", 
    "Through Doors", "Humanoid", 
    "Piss On Dead", "Hurt By Lava", "Base Speed", "Gold Hold", 
    "Walk Anim Speed", "Immune to Gas", "Usual Attack", 
    "Field Of View", "Eye Effect", "Rebirth", "Heal Requirment", 
    "Heal Threshold", "Hero Vs Keeper Cost", "Real Training", 
    "Torture Time", "Slaps To Kill", "Creature Loyalty", 
    "Loyalty Level", "Entrance Force", "Damage To Boulder"};

static int STAT_OFF[]={5, 8, 9, 10, 11, 12, 13, 75, 14, 29,
    43, 44, 45, 46, 47, 117, 118, 78, 120, 122, 113, 114, 73,
    74, 19, 24, 28, 48, 49, 50, 51, 110, 112, 6, 7, 76, 103,
    104, 106, 107, 108, 111, 116};

static int STAT_TYPE[]={0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,0,0,0,1,1,1,1,1,1,1,0,0,0,1,2,0,2,1,0,
    0,0,0,0,0,0,0,0,0};

static void *STAT_PARAM[]={NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
    ATTACKS, NULL, EYES, NULL, NULL, NULL, NULL, NULL,
    NULL,NULL,NULL,NULL,NULL,NULL};

static char *LIVING[]={"Primary job", "Secondary job", 
    "Sleep experience slab", "Sleep experience", 
    "Hitting experience", "Scavenge value", "Scavenge requirement",
    "Scavenge cost", "Training value", "Training cost", "Hunger rate",
    "Hunger fill", "Lair size", "Manufacture value", "Research value",
    "Lair enemy", "Primary entrance", "Secondary entrance", 
    "Tertiary entrance", "Primary slabs", "Secondary slabs", "Tertiary slabs"};

static int LIVING_OFF[]={1, 2, 20, 21, 25, 22, 23,
    30, 26, 27, 15, 16, 18, 77, 72, 109, 79, 80, 81, 82, 83, 84};

static int LIVING_TYPE[]={2,2,2,0,
    0,0,0,0,0,0,0,0,0,0,0,
    2,2,2,2,0,0,0};

static void *LIVING_PARAM[]={JOBS, JOBS, SLABS, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    CREATURES, ROOMS, ROOMS, ROOMS, NULL, NULL, NULL};
    
static void *ANNOY[]={"In Hand", "Eat Food", "Will Not Do Job", 
    "No Lair", "No Hatchery", "Woken Up", "Standing On Dead Enemy", 
    "Sulking", "No Salary", "Slapped", "Standing On Dead Friend", 
    "In Torture", "In Temple", "Sleeping", "Got Wage", "Win Battle", 
    "Untrained Time", "Untrained", "Job Stress", "Annoyance Job Stress",
    "Queue", "Others Leaving", "Annoy Level"};

static int ANNOY_OFF[]={121, 119, 105, 85, 86, 87, 88,
    89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 17, 115};

void load (void);
void save (void);

long *values;
long *jobanger;
long *jobnotdo;
unsigned char *txtfile;
long size;

void flush_keybuffer(void);
void draw_scr (void);
unsigned char get_key (void);
void init_slang(void);
void die (char *x);
void done(void);
void proc_key (void);
void action_key (int key);
void get_screen_size (void);

int slang_going=0;
int finished=0;
int rows, cols;
int choicemin, choicemax, choicen, choicecols, choicew;

long tempLong;

menuitem newItem (int x, int y, char *label, int type, long *value,
		  void *param);
menu newMenu(char *title);
void addItem (menu dest, menuitem src);
menu initMenus(void);
menu currentMenu;
int currentItem=0;
int itemOpen=0;
char tempValue[12]; /* Don't allow more than 12 digit numbers! */

int main (int argc, char **argv)
{
    load();
    currentMenu=initMenus();
    currentItem=currentMenu->first;
/*  load_file() */    
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
	if (currentItem==i && m->items[i]->type==MENU)
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

menu initMenus(void)
{
    int i, j;
    char temp[60];

    static int lx[3]={0,23,47};
    static int sx[3]={16, 40, 66};
    
    menu root, creatures, rooms, powers1, powers2, misc1, misc2;
    menu spell, shots, cstat, cliving, cannoy, clevel;
    menu trap, health, instance1, instance2, instance3, research;

    
    /* Create creatures menus... ouch! */
    
    creatures = newMenu ("Creature variables");
    for (i=0; i < 31; i++)
    {
	/* Create a menu for this creature */
	sprintf (temp, "Basic stats for %s", CREATURES[i+1]);
	cstat = newMenu (strdup(temp));
	sprintf (temp, "Annoyance levels for %s", CREATURES[i+1]);
	cannoy = newMenu (strdup(temp));
	sprintf (temp, "Level stats for %s", CREATURES[i+1]);
	clevel = newMenu (strdup(temp));
	sprintf (temp, "Daily life for %s", CREATURES[i+1]);
	cliving = newMenu (strdup(temp));
	
	addItem (creatures, newItem ((i>>4)*40+2, (i&15)+2, CREATURES[i+1], MENU, NULL, cstat));

	addItem (cstat, newItem (0, 23, "Daily life", MENU, NULL, cliving));
	addItem (cstat, newItem (26, 23, "Level stats", MENU, NULL, clevel));
	addItem (cstat, newItem (52, 23, "Annoyance levels", MENU, NULL, cannoy));
	
	addItem (cliving, newItem (0, 23, "Basic stats", MENU, NULL, cstat));
	addItem (cliving, newItem (26, 23, "Level stats", MENU, NULL, clevel));
	addItem (cliving, newItem (52, 23, "Annoyance levels", MENU, NULL, cannoy));

	addItem (clevel, newItem (0, 23, "Basic stats", MENU, NULL, cstat));
	addItem (clevel, newItem (26, 23, "Daily life", MENU, NULL, cliving));
	addItem (clevel, newItem (52, 23, "Annoyance levels", MENU, NULL, cannoy));

	addItem (cannoy, newItem (0, 23, "Basic stats", MENU, NULL, cstat));
	addItem (cannoy, newItem (26, 23, "Daily life", MENU, NULL, cliving));
	addItem (cannoy, newItem (52, 23, "Level stats", MENU, NULL, clevel));

	for (j=0; j < 43; j++)
	{
	    addItem (cstat, newItem (lx[j/15], (j%15)+2, BASIC_STATS[j], STATIC, NULL, NULL));
	    addItem (cstat, newItem (sx[j/15], (j%15)+2, "", STAT_TYPE[j], values+i*123+STAT_OFF[j], STAT_PARAM[j]));
	}

	for (j=0; j < 23; j++)
	{
	    addItem (cannoy, newItem ((j/12)*40, (j%12)+2, ANNOY[j], STATIC, NULL, NULL));
	    addItem (cannoy, newItem ((j/12)*40+30, (j%12)+2, "", NUMERIC, values+i*123+ANNOY_OFF[j], NULL));
	}
	addItem (cannoy, newItem (0, 15, "Behaviour when angry", STATIC, NULL, NULL));
	for (j=0; j < 8; j++)
	{
	    addItem (cannoy, newItem ((j/4)*40, (j%4)+17, ANGER_JOBS[j], STATIC, NULL, NULL));
	    addItem (cannoy, newItem ((j/4)*40+30, (j%4)+17, "", BOOLEAN, jobanger+i*8+j, NULL));
	}

	for (j=0; j < 22; j++)
	{
	    addItem (cliving, newItem ((j/11)*40, (j%11)+2, LIVING[j], STATIC, NULL, NULL));
	    addItem (cliving, newItem ((j/11)*40+22, (j%11)+2, "", LIVING_TYPE[j], values+i*123+LIVING_OFF[j], LIVING_PARAM[j]));
	}
	
	addItem (cliving, newItem (0, 14, "\"Not do\" jobs", STATIC, NULL, NULL));
	for (j=0; j < 15; j++)
	    addItem (cliving, newItem ((j/5)*26, (j%5)+16, ALIGN_JOBS[j+1], BOOLEAN, jobnotdo+i*15+j, NULL));
	
	addItem (clevel, newItem (0, 2, "Level", STATIC, NULL, NULL));
	addItem (clevel, newItem (5, 2, "Exp", STATIC, NULL, NULL));
	for (j=0; j < 10; j++)
	{
	    if (j < 9)
		sprintf (temp, "%d", j+2);
	    else
		sprintf (temp, "Grow");
	    addItem (clevel, newItem (0, j+4, strdup(temp), STATIC, NULL, NULL));
	    sprintf (temp, "Power %d", j+1);
	    addItem (clevel, newItem (15, j+4, strdup (temp), STATIC, NULL, NULL));
	    sprintf (temp, "Level req. for power %d", j+1);
	    addItem (clevel, newItem (40, j+4, strdup (temp), STATIC, NULL, NULL));
	    addItem (clevel, newItem (6, j+4, "", NUMERIC, values+i*123+31+j, NULL));
	    addItem (clevel, newItem (25, j+4, "", CHOICE, values+i*123+52+j, INSTANCE));
	    addItem (clevel, newItem (65, j+4, "", NUMERIC, values+i*123+62+j, NULL));
	}
	addItem (clevel, newItem (0, 15, "Grow up   ", CHOICE, values+i*123+41, CREATURES));
	addItem (clevel, newItem (0, 16, "Grow level", NUMERIC, values+i*123+42, NULL));
	cstat->back=creatures;
	cannoy->back=creatures;
	clevel->back=creatures;
	cliving->back=creatures;
    }

    /* Create rooms menu */
    
    rooms = newMenu ("Room costs and health values");
    addItem (rooms, newItem (2, 2, "Room", STATIC, NULL, NULL));
    addItem (rooms, newItem (22, 2, "Cost", STATIC, NULL, NULL));
    addItem (rooms, newItem (42, 2, "Health", STATIC, NULL, NULL));
    for (i=0; ROOMS[i+1]; i++)
    {
	addItem (rooms, newItem (2, 4+i, ROOMS[i+1], STATIC, NULL, NULL));
	addItem (rooms, newItem (21, 4+i, "", NUMERIC, values+3814+i*3, NULL));
	addItem (rooms, newItem (41, 4+i, "", NUMERIC, values+3815+i*3, NULL));
    }

    /* Create keeper power menus */

    powers1 = newMenu ("Keeper spells (page 1)");
    addItem (powers1, newItem (0, 2, "Spell: Cost:", STATIC, NULL, NULL));
    for (i=0; i < 9; i++)
    {
	sprintf (temp, "%d", i);
	addItem (powers1, newItem (7*i+15, 2, strdup(temp), STATIC, NULL, NULL));
    }
    for (i=0; POWERS[i]; i++)
    {
	addItem (powers1, newItem (0, 3+i, POWERS[i], STATIC, NULL, NULL));
	for (j=0; j < 9; j++)
	    addItem (powers1, newItem (7*j+14, 3+i, "", NUMERIC,
				       values+3862+i*20+j, NULL));
    }

    powers2 = newMenu ("Keeper spells (page 2)");
    addItem (powers2, newItem (0, 2, "Spell: Power:", STATIC, NULL, NULL));
    for (i=0; i < 9; i++)
    {
	sprintf (temp, "%d", i);
	addItem (powers2, newItem (6*i+15, 2, strdup(temp), STATIC, NULL, NULL));
    }
    addItem (powers2, newItem (69, 2, "Time", STATIC, NULL, NULL));
    for (i=0; POWERS[i]; i++)
    {
	addItem (powers2, newItem (0, 3+i, POWERS[i], STATIC, NULL, NULL));
	for (j=0; j < 9; j++)
	    addItem (powers2, newItem (6*j+14, 3+i, "", NUMERIC, 
				       values+3872+i*20+j, NULL));
	addItem (powers2, newItem (70, 3+i, "", NUMERIC, values+3871+i*20, NULL));
    }
    addItem (powers1, newItem (0, 23, "Page 2", MENU, NULL, powers2));
    addItem (powers2, newItem (0, 23, "Page 1", MENU, NULL, powers1));

    /* Create miscellaneous menu */

    misc1 = newMenu ("Miscellaneous options (page 1)");
    addItem (misc1, newItem (0, 2, "Gold per block          ", NUMERIC, values+4242, NULL));
    addItem (misc1, newItem (0, 3, "Gold per pot            ", NUMERIC, values+4244, NULL));
    addItem (misc1, newItem (0, 4, "Gold pile value         ", NUMERIC, values+4246, NULL));
    addItem (misc1, newItem (0, 5, "Gold pile maximum       ", NUMERIC, values+4248, NULL));
    addItem (misc1, newItem (0, 6, "Recovery frequency      ", NUMERIC, values+4250, NULL));
    addItem (misc1, newItem (0, 7, "Fight maximum hate      ", NUMERIC, values+4252, NULL));
    addItem (misc1, newItem (0, 8, "Fight borderline        ", NUMERIC, values+4254, NULL));
    addItem (misc1, newItem (0, 9, "Fight maximum love      ", NUMERIC, values+4256, NULL));
    addItem (misc1, newItem (0, 10, "Food life from hatchery ", NUMERIC, values+4258, NULL));
    addItem (misc1, newItem (0, 11, "Fight hate kill value   ", NUMERIC, values+4260, NULL));
    addItem (misc1, newItem (0, 12, "D/M statue reappear time", NUMERIC, values+4262, NULL));
    addItem (misc1, newItem (0, 13, "D/M object reappear time", NUMERIC, values+4264, NULL));
    addItem (misc1, newItem (0, 14, "Hits per slab           ", NUMERIC, values+4266, NULL));
    addItem (misc1, newItem (0, 15, "Collapse dungeon damage ", NUMERIC, values+4268, NULL));
    addItem (misc1, newItem (0, 16, "Turns per collapse      ", NUMERIC, values+4270, NULL));
    addItem (misc1, newItem (0, 17, "Ghost convert chance    ", NUMERIC, values+4284, NULL));
    addItem (misc1, newItem (0, 18, "Armoury time            ", NUMERIC, values+4286, NULL));
    addItem (misc1, newItem (0, 19, "Workshop time           ", NUMERIC, values+4288, NULL));
    addItem (misc1, newItem (40, 2, "Observatory time          ", NUMERIC, values+4290, NULL));
    addItem (misc1, newItem (40, 3, "Observatory generate      ", NUMERIC, values+4292, NULL));
    addItem (misc1, newItem (40, 4, "Improve area              ", NUMERIC, values+4294, NULL));
    addItem (misc1, newItem (40, 5, "Door open for             ", NUMERIC, values+4296, NULL));
    addItem (misc1, newItem (40, 6, "Boulder reduce health slap", NUMERIC, values+4298, NULL));
    addItem (misc1, newItem (40, 7, "Boulder reduce health wall", NUMERIC, values+4300, NULL));
    addItem (misc1, newItem (40, 8, "Boulder reduce health room", NUMERIC, values+4302, NULL));
    addItem (misc1, newItem (40, 9, "Tile strength             ", NUMERIC, values+4304, NULL));
    addItem (misc1, newItem (40, 10, "Gold tile strength        ", NUMERIC, values+4306, NULL));
    addItem (misc1, newItem (40, 11, "Armageddon count down     ", NUMERIC, values+4308, NULL));
    addItem (misc1, newItem (40, 12, "Armageddon duration       ", NUMERIC, values+4310, NULL));
    addItem (misc1, newItem (40, 13, "Minimum gold              ", NUMERIC, values+4312, NULL));
    addItem (misc1, newItem (40, 14, "Max gold lookup           ", NUMERIC, values+4314, NULL));
    addItem (misc1, newItem (40, 15, "Min gold to record        ", NUMERIC, values+4316, NULL));
    addItem (misc1, newItem (40, 16, "Wait for room time        ", NUMERIC, values+4318, NULL));
    addItem (misc1, newItem (40, 17, "Check expand time         ", NUMERIC, values+4320, NULL));
    addItem (misc1, newItem (40, 18, "Max distance to dig       ", NUMERIC, values+4322, NULL));
    addItem (misc1, newItem (40, 19, "Wait after room area      ", NUMERIC, values+4324, NULL));

    misc2 = newMenu ("Miscellaneous options (page 2)");
    addItem (misc2, newItem (0, 2, "Per imp gold dig size           ", NUMERIC, values+4326, NULL));
    addItem (misc2, newItem (0, 3, "Default generate speed          ", NUMERIC, values+4328, NULL));
    addItem (misc2, newItem (0, 4, "Barrack time                    ", NUMERIC, values+4330, NULL));
    addItem (misc2, newItem (0, 5, "Food generation speed           ", NUMERIC, values+4332, NULL));
    addItem (misc2, newItem (0, 6, "Def. neutral entrance lvl       ", NUMERIC, values+4334, NULL));
    addItem (misc2, newItem (0, 7, "Def. max creatures gen entrance ", NUMERIC, values+4336, NULL));
    addItem (misc2, newItem (0, 8, "Def. imp dig damage             ", NUMERIC, values+4338, NULL));
    addItem (misc2, newItem (0, 9, "Def. imp dig own damage         ", NUMERIC, values+4340, NULL));
    addItem (misc2, newItem (0, 10, "Prison skeleton chance          ", NUMERIC, values+4342, NULL));
    addItem (misc2, newItem (0, 11, "Def. sacrifice score for horny  ", NUMERIC, values+4344, NULL));
    addItem (misc2, newItem (0, 12, "Game turns in flee              ", NUMERIC, values+4346, NULL));
    addItem (misc2, newItem (0, 13, "Train cost frequency            ", NUMERIC, values+4348, NULL));
    addItem (misc2, newItem (0, 14, "Torture convert chance          ", NUMERIC, values+4350, NULL));
    addItem (misc2, newItem (0, 15, "Pay day gap                     ", NUMERIC, values+4352, NULL));
    addItem (misc2, newItem (0, 16, "Chest gold hand                 ", NUMERIC, values+4354, NULL));
    addItem (misc2, newItem (0, 17, "Slab collapse time              ", NUMERIC, values+4356, NULL));
    addItem (misc2, newItem (0, 18, "Dungeon heart health            ", NUMERIC, values+4358, NULL));
    addItem (misc2, newItem (0, 19, "H of E gold grab amount         ", NUMERIC, values+4360, NULL));
    addItem (misc2, newItem (40, 2, "Dungeon heart heal time        ", NUMERIC, values+4362, NULL));
    addItem (misc2, newItem (40, 3, "Dungeon heart heal health      ", NUMERIC, values+4364, NULL));
    addItem (misc2, newItem (40, 4, "Hero door wait time            ", NUMERIC, values+4366, NULL));
    addItem (misc2, newItem (40, 5, "Disease transfer %             ", NUMERIC, values+4368, NULL));
    addItem (misc2, newItem (40, 6, "Disease lose % health          ", NUMERIC, values+4370, NULL));
    addItem (misc2, newItem (40, 7, "Disease lose health time       ", NUMERIC, values+4372, NULL));
    addItem (misc2, newItem (40, 8, "Hold audience time             ", NUMERIC, values+4374, NULL));
    addItem (misc2, newItem (40, 9, "A'g'don t/port your time gap   ", NUMERIC, values+4376, NULL));
    addItem (misc2, newItem (40, 10, "A'g'don t/port enemy time gap  ", NUMERIC, values+4378, NULL));
    addItem (misc2, newItem (40, 11, "Scavenge cost frequency        ", NUMERIC, values+4380, NULL));
    addItem (misc2, newItem (40, 12, "Temple scavenge protection time", NUMERIC, values+4382, NULL));
    addItem (misc2, newItem (40, 13, "Bodies for vampire             ", NUMERIC, values+4384, NULL));
    addItem (misc2, newItem (40, 14, "Body remains for               ", NUMERIC, values+4386, NULL));
    addItem (misc2, newItem (40, 15, "Graveyard convert time         ", NUMERIC, values+4388, NULL));
    addItem (misc2, newItem (40, 16, "Grave min dist for teleport    ", NUMERIC, values+4390, NULL));
    
    addItem (misc1, newItem (0, 23, "Page 2", MENU, NULL, misc2));
    addItem (misc2, newItem (0, 23, "Page 1", MENU, NULL, misc1));

    /* Create spell menu */

    spell = newMenu ("Spell durations");
    addItem (spell, newItem (0, 2, "Spell", STATIC, NULL, NULL));
    addItem (spell, newItem (20, 2, "Duration", STATIC, NULL, NULL));
    addItem (spell, newItem (40, 2, "Spell", STATIC, NULL, NULL));
    addItem (spell, newItem (60, 2, "Duration", STATIC, NULL, NULL));
    for (i=0; i < 28; i++)
    {
	addItem (spell, newItem ((i/14)*40, (i%14)+4, SPELLS[i], STATIC, NULL, NULL));
	addItem (spell, newItem ((i/14)*40+19, (i%14)+4, "", NUMERIC, values+4419+i*2, NULL));
    }
    
    /* Create shot menu */
    
    shots = newMenu ("Keeper Shot data");
    addItem (shots, newItem (0, 2, "Description", STATIC, NULL, NULL));
    addItem (shots, newItem (18, 2, "Health", STATIC, NULL, NULL));
    addItem (shots, newItem (25, 2, "Damage", STATIC, NULL, NULL));
    addItem (shots, newItem (32, 2, "Speed", STATIC, NULL, NULL));
    addItem (shots, newItem (40, 2, "Description", STATIC, NULL, NULL));
    addItem (shots, newItem (58, 2, "Health", STATIC, NULL, NULL));
    addItem (shots, newItem (65, 2, "Damage", STATIC, NULL, NULL));
    addItem (shots, newItem (72, 2, "Speed", STATIC, NULL, NULL));

    for (i=0; i < 29; i++)
    {
	addItem (shots, newItem ((i/15)*40, 4+(i%15), SHOTS[i], STATIC, NULL, NULL));
	addItem (shots, newItem ((i/15)*40+17, 4+(i%15), "", NUMERIC, values+4479+i*4, NULL));
	addItem (shots, newItem ((i/15)*40+24, 4+(i%15), "", NUMERIC, values+4480+i*4, NULL));
	addItem (shots, newItem ((i/15)*40+31, 4+(i%15), "", NUMERIC, values+4481+i*4, NULL));
    }
    
    /* Create traps and doors menu */
    
    trap = newMenu ("Traps and Doors");
    addItem (trap, newItem (0, 2, "Description", STATIC, NULL, NULL));
    addItem (trap, newItem (20, 2, "Man. Lvl ", STATIC, NULL, NULL));
    addItem (trap, newItem (30, 2, "Man. Req", STATIC, NULL, NULL));
    addItem (trap, newItem (40, 2, "Shots", STATIC, NULL, NULL));
    addItem (trap, newItem (50, 2, "Reload", STATIC, NULL, NULL));
    addItem (trap, newItem (60, 2, "Resale value", STATIC, NULL, NULL));
    addItem (trap, newItem (20, 11, "Man. Lvl ", STATIC, NULL, NULL));
    addItem (trap, newItem (30, 11, "Man. Req", STATIC, NULL, NULL));
    addItem (trap, newItem (40, 11, "Resale", STATIC, NULL, NULL));
    addItem (trap, newItem (50, 11, "Health", STATIC, NULL, NULL));
    addItem (trap, newItem (0, 4, "Boulder", STATIC, NULL, NULL));
    addItem (trap, newItem (0, 5, "Alarm", STATIC, NULL, NULL));
    addItem (trap, newItem (0, 6, "Poison gas", STATIC, NULL, NULL));
    addItem (trap, newItem (0, 7, "Lightning", STATIC, NULL, NULL));
    addItem (trap, newItem (0, 8, "Word of power", STATIC, NULL, NULL));
    addItem (trap, newItem (0, 9, "Lava", STATIC, NULL, NULL));
    addItem (trap, newItem (0, 13, "Wooden", STATIC, NULL, NULL));
    addItem (trap, newItem (0, 14, "Braced", STATIC, NULL, NULL));
    addItem (trap, newItem (0, 15, "Steel", STATIC, NULL, NULL));
    addItem (trap, newItem (0, 16, "Magic", STATIC, NULL, NULL));
    
    for (i=0; i < 6; i++)
    {
	addItem (trap, newItem (19, 4+i, "", NUMERIC, values+4600+i*6, NULL));
	addItem (trap, newItem (29, 4+i, "", NUMERIC, values+4602+i*6, NULL));
	addItem (trap, newItem (39, 4+i, "", NUMERIC, values+4603+i*6, NULL));
	addItem (trap, newItem (49, 4+i, "", NUMERIC, values+4604+i*6, NULL));
	addItem (trap, newItem (59, 4+i, "", NUMERIC, values+4605+i*6, NULL));
    }
    for (i=0; i < 4; i++)
    {
	addItem (trap, newItem (19, 13+i, "", NUMERIC, values+4637+i*5, NULL));
	addItem (trap, newItem (29, 13+i, "", NUMERIC, values+4638+i*5, NULL));
	addItem (trap, newItem (39, 13+i, "", NUMERIC, values+4639+i*5, NULL));
	addItem (trap, newItem (49, 13+i, "", NUMERIC, values+4640+i*5, NULL));
    }

    /* Create health/block stats menu */
    
    health = newMenu ("Health and block tables");
    addItem (health, newItem (2, 2, "Health values", STATIC, NULL, NULL));
    addItem (health, newItem (2, 4, "Hunger health loss                ", NUMERIC, values+4661, NULL));
    addItem (health, newItem (2, 5, "Game turns per hunger health loss ", NUMERIC, values+4663, NULL));
    addItem (health, newItem (2, 6, "Food health gain                  ", NUMERIC, values+4665, NULL));
    addItem (health, newItem (2, 7, "Torture health loss               ", NUMERIC, values+4667, NULL));
    addItem (health, newItem (2, 8, "Game turns per torture health loss", NUMERIC, values+4669, NULL));
    addItem (health, newItem (2, 10, "Room", STATIC, NULL, NULL));
    addItem (health, newItem (15, 10, "Health", STATIC, NULL, NULL));
    addItem (health, newItem (2, 12, "Rock        ", NUMERIC, values+4671, NULL));
    addItem (health, newItem (2, 13, "Gold        ", NUMERIC, values+4673, NULL));
    addItem (health, newItem (2, 14, "Path        ", NUMERIC, values+4675, NULL));
    addItem (health, newItem (2, 15, "Floor       ", NUMERIC, values+4677, NULL));
    addItem (health, newItem (2, 16, "Room        ", NUMERIC, values+4679, NULL));
    addItem (health, newItem (2, 17, "Wooden door ", NUMERIC, values+4681, NULL));
    addItem (health, newItem (2, 18, "Braced door ", NUMERIC, values+4683, NULL));
    addItem (health, newItem (2, 19, "Steel door  ", NUMERIC, values+4685, NULL));
    addItem (health, newItem (2, 20, "Magic door  ", NUMERIC, values+4687, NULL));

    /* Create instances menu */

    instance1=newMenu ("Instance data (page 1)");
    instance2=newMenu ("Instance data (page 2)");
    instance3=newMenu ("Instance data (page 3)");
    addItem (instance1, newItem(0, 3, "Name", STATIC, NULL, NULL));
    addItem (instance1, newItem(15,3, "Time", STATIC, NULL, NULL));
    addItem (instance1, newItem(24,2, "Action", STATIC, NULL, NULL));
    addItem (instance1, newItem(24,3, "Time", STATIC, NULL, NULL));
    addItem (instance1, newItem(33,2, "Reset", STATIC, NULL, NULL));
    addItem (instance1, newItem(33,3, "Time", STATIC, NULL, NULL));
    addItem (instance1, newItem(42,2, "FP", STATIC, NULL, NULL));
    addItem (instance1, newItem(42,3, "Time", STATIC, NULL, NULL));
    addItem (instance1, newItem(51,2, "FP Act", STATIC, NULL, NULL));
    addItem (instance1, newItem(51,3, "Time", STATIC, NULL, NULL));
    addItem (instance1, newItem(60,2, "FP Reset", STATIC, NULL, NULL));
    addItem (instance1, newItem(60,3, "Time", STATIC, NULL, NULL));
    addItem (instance1, newItem(69,2, "Force", STATIC, NULL, NULL));
    addItem (instance1, newItem(69,3, "Visibility", STATIC, NULL, NULL));

    addItem (instance2, newItem(0, 3, "Name", STATIC, NULL, NULL));
    addItem (instance2, newItem(15,3, "Time", STATIC, NULL, NULL));
    addItem (instance2, newItem(24,2, "Action", STATIC, NULL, NULL));
    addItem (instance2, newItem(24,3, "Time", STATIC, NULL, NULL));
    addItem (instance2, newItem(33,2, "Reset", STATIC, NULL, NULL));
    addItem (instance2, newItem(33,3, "Time", STATIC, NULL, NULL));
    addItem (instance2, newItem(42,2, "FP", STATIC, NULL, NULL));
    addItem (instance2, newItem(42,3, "Time", STATIC, NULL, NULL));
    addItem (instance2, newItem(51,2, "FP Act", STATIC, NULL, NULL));
    addItem (instance2, newItem(51,3, "Time", STATIC, NULL, NULL));
    addItem (instance2, newItem(60,2, "FP Reset", STATIC, NULL, NULL));
    addItem (instance2, newItem(60,3, "Time", STATIC, NULL, NULL));
    addItem (instance2, newItem(69,2, "Force", STATIC, NULL, NULL));
    addItem (instance2, newItem(69,3, "Visibility", STATIC, NULL, NULL));

    addItem (instance3, newItem(0, 3, "Name", STATIC, NULL, NULL));
    addItem (instance3, newItem(15,3, "Time", STATIC, NULL, NULL));
    addItem (instance3, newItem(24,2, "Action", STATIC, NULL, NULL));
    addItem (instance3, newItem(24,3, "Time", STATIC, NULL, NULL));
    addItem (instance3, newItem(33,2, "Reset", STATIC, NULL, NULL));
    addItem (instance3, newItem(33,3, "Time", STATIC, NULL, NULL));
    addItem (instance3, newItem(42,2, "FP", STATIC, NULL, NULL));
    addItem (instance3, newItem(42,3, "Time", STATIC, NULL, NULL));
    addItem (instance3, newItem(51,2, "FP Act", STATIC, NULL, NULL));
    addItem (instance3, newItem(51,3, "Time", STATIC, NULL, NULL));
    addItem (instance3, newItem(60,2, "FP Reset", STATIC, NULL, NULL));
    addItem (instance3, newItem(60,3, "Time", STATIC, NULL, NULL));
    addItem (instance3, newItem(69,2, "Force", STATIC, NULL, NULL));
    addItem (instance3, newItem(69,3, "Visibility", STATIC, NULL, NULL));
    
    for (i=0; i < 15; i++)
    {
	addItem (instance1, newItem(0, 5+i, INSTANCE[i+1], STATIC, NULL, NULL));
	addItem (instance2, newItem(0, 5+i, INSTANCE[i+16], STATIC, NULL, NULL));
	addItem (instance3, newItem(0, 5+i, INSTANCE[i+31], STATIC, NULL, NULL));
    }
    
    for (i=0; i < 15; i++)
    {
	for (j=0; j < 7; j++)
	{
	    addItem (instance1, newItem (14+9*j, 5+i, "", NUMERIC, values+4697+i*8+j, NULL));
	    addItem (instance2, newItem (14+9*j, 5+i, "", NUMERIC, values+4817+i*8+j, NULL));
	    addItem (instance3, newItem (14+9*j, 5+i, "", NUMERIC, values+4937+i*8+j, NULL));
	}
    }
    
    addItem (instance1, newItem (0, 23, "Page 2", MENU, NULL, instance2));
    addItem (instance2, newItem (0, 23, "Page 1", MENU, NULL, instance1));
    addItem (instance2, newItem (8, 23, "Page 3", MENU, NULL, instance3));
    addItem (instance3, newItem (0, 23, "Page 2", MENU, NULL, instance2));

    /* Create research menu */
    
    research=newMenu ("Research models (?) and costs");
    addItem (research, newItem (0, 2, "Description", STATIC, NULL, NULL));
    addItem (research, newItem (20, 2, "Model", STATIC, NULL, NULL));
    addItem (research, newItem (30, 2, "Amount", STATIC, NULL, NULL));
    addItem (research, newItem (40, 2, "Description", STATIC, NULL, NULL));
    addItem (research, newItem (60, 2, "Model", STATIC, NULL, NULL));
    addItem (research, newItem (70, 2, "Amount", STATIC, NULL, NULL));
    
    addItem (research, newItem (0, 4, "H of E", STATIC, NULL, NULL));
    addItem (research, newItem (0, 5, "Possess", STATIC, NULL, NULL));
    addItem (research, newItem (0, 6, "Slap", STATIC, NULL, NULL));
    addItem (research, newItem (0, 7, "Create imp", STATIC, NULL, NULL));
    addItem (research, newItem (0, 8, "Treasure room", STATIC, NULL, NULL));
    addItem (research, newItem (0, 9, "Hatchery", STATIC, NULL, NULL));
    addItem (research, newItem (0, 10, "Lair", STATIC, NULL, NULL));
    addItem (research, newItem (0, 11, "Research", STATIC, NULL, NULL));
    addItem (research, newItem (0, 12, "Training", STATIC, NULL, NULL));
    addItem (research, newItem (0, 13, "Sight of evil", STATIC, NULL, NULL));
    addItem (research, newItem (0, 14, "Bridge", STATIC, NULL, NULL));
    addItem (research, newItem (0, 15, "Speed", STATIC, NULL, NULL));
    addItem (research, newItem (0, 16, "Must obey", STATIC, NULL, NULL));
    addItem (research, newItem (0, 17, "Guard post", STATIC, NULL, NULL));
    addItem (research, newItem (0, 18, "Call to arms", STATIC, NULL, NULL));
    addItem (research, newItem (0, 19, "Workshop", STATIC, NULL, NULL));

    addItem (research, newItem (40, 4, "Conceal", STATIC, NULL, NULL));
    addItem (research, newItem (40, 5, "Barracks", STATIC, NULL, NULL));
    addItem (research, newItem (40, 6, "Hold audience", STATIC, NULL, NULL));
    addItem (research, newItem (40, 7, "Prison", STATIC, NULL, NULL));
    addItem (research, newItem (40, 8, "Cave in", STATIC, NULL, NULL));
    addItem (research, newItem (40, 9, "Torture", STATIC, NULL, NULL));
    addItem (research, newItem (40, 10, "Heal", STATIC, NULL, NULL));
    addItem (research, newItem (40, 11, "Temple", STATIC, NULL, NULL));
    addItem (research, newItem (40, 12, "Lightning", STATIC, NULL, NULL));
    addItem (research, newItem (40, 13, "Graveyard", STATIC, NULL, NULL));
    addItem (research, newItem (40, 14, "Protect", STATIC, NULL, NULL));
    addItem (research, newItem (40, 15, "Scavenger", STATIC, NULL, NULL));
    addItem (research, newItem (40, 16, "Chicken", STATIC, NULL, NULL));
    addItem (research, newItem (40, 17, "Disease", STATIC, NULL, NULL));
    addItem (research, newItem (40, 18, "Armageddon", STATIC, NULL, NULL));
    addItem (research, newItem (40, 19, "Destroy walls", STATIC, NULL, NULL));

    for (i=0; i < 32; i++)
    {
	addItem (research, newItem ((i>>4)*40+19, (i&15)+4, "", NUMERIC, values+5079+i*2, NULL));
	addItem (research, newItem ((i>>4)*40+29, (i&15)+4, "", NUMERIC, values+5080+i*2, NULL));
    }

    /* Create root menu */
    
    root=newMenu("Dungeon Keeper creatures.txt editor");
    root->back=NULL;
    addItem (root, newItem (2, 2, "Creatures", MENU, NULL, creatures));
    addItem (root, newItem (2, 3, "Rooms", MENU, NULL, rooms));
    addItem (root, newItem (2, 4, "Keeper powers", MENU, NULL, powers1));
    addItem (root, newItem (2, 5, "Miscellaneous", MENU, NULL, misc1));
    addItem (root, newItem (2, 6, "Spell durations", MENU, NULL, spell));
    addItem (root, newItem (2, 7, "Shots", MENU, NULL, shots));
    addItem (root, newItem (2, 8, "Traps and Doors", MENU, NULL, trap));
    addItem (root, newItem (2, 9, "Health / blocks", MENU, NULL, health));
    addItem (root, newItem (2, 10, "Instances", MENU, NULL, instance1));
    addItem (root, newItem (2, 11, "Research", MENU, NULL, research));
    
    creatures->back=root;
    rooms->back=root;
    powers1->back=root;
    powers2->back=root;
    misc1->back=root;
    misc2->back=root;
    trap->back=root;
    health->back=root;
    research->back=root;
    instance1->back=root;
    instance2->back=root;
    instance3->back=root;
    spell->back=root;
    shots->back=root;
    return root;
}

void load (void)
{
    FILE *fp;
    int n=0;
    char *buffer;
    int msize=0;
    int g;
    int inNumber=0;
    int tabLast=0;
    char number[20];
    int nums=0;
    int i, j;
    
    values = malloc (5144*sizeof (long));
    if (!values)
	die ("Out of memory");
    jobnotdo = malloc (31*15*sizeof(long));
    jobanger = malloc (31*8*sizeof(long));
    if (!jobanger || !jobnotdo)
	die ("Out of memory");
    buffer = malloc (65536);
    if (!buffer)
	die ("Out of memory");
    
    fp = fopen ("creature.txt", "rb");
    if (!fp)
	die ("Can't open creature.txt. Aborting.\n");
    while ((g=fgetc (fp))!=EOF)
    {
	if (g < 0)
	    printf ("Eek!\n");
	if (inNumber)
	{
	    if (g >= '0' && g <= '9')
		number[nums++]=(char)g;
	    else
	    {
		number[nums]=0;
		values[n++]=atoi (number);
		inNumber=0;
	    }
	}
	if (!inNumber) /* Can't just do else, because of the end of a number */
	{
	    if (size==msize)
	    {
		msize+=65536;
		txtfile = realloc (txtfile, msize);
		if (!txtfile)
		    die ("Out of memory");
	    }
	    if (((g < '0' || g > '9') && g != '-') || !tabLast)
		txtfile[size++]=(char)g;
	    else
	    {
		inNumber=1;
		nums=1;
		number[0]=(char)g;
		txtfile[size++]=255;
	    }
	}
	if (g==9)
	    tabLast=1;
	else
	    tabLast=0;
    }
    fclose (fp);
    for (i=0; i < 31; i++)
    {
	for (j=0; j < 15; j++)
	{
	    if ((values[i*123+3]>>j)&1)
		jobnotdo[i*15+j]=1;
	    else
		jobnotdo[i*15+j]=0;
	}
	for (j=0; j < 8; j++)
	{
	    if ((values[i*123+4]>>j)&1)
		jobanger[i*8+j]=1;
	    else
		jobanger[i*8+j]=0;
	}
	g=0;
	for (j=0; j < 15; j++)
	    if ((values[i*123+1]>>j)&1)
		g=j+1;
	values[i*123+1]=g;
	g=0;
	for (j=0; j < 15; j++)
	    if ((values[i*123+2]>>j)&1)
		g=j+1;
	values[i*123+2]=g;
    }
}

void save (void)
{
    FILE *fp;
    int i, j, g, n=0;
    
    fp = fopen ("creature.new", "wb");
    if (!fp)
	die ("Can't open output file");
    for (i=0; i < 31; i++)
    {
	values[i*123+3]=0;
	values[i*123+4]=0;
	for (j=0; j < 14; j++)
	    if (jobnotdo[i*15+j])
		values[i*123+3]+=1<<j;
	for (j=0; j < 8; j++)
	    if (jobanger[i*8+j])
		values[i*123+4]+=1<<j;
	if (values[i*123+1])
	    values[i*123+1]=1<<(values[i*123+1]-1);
	else
	    values[i*123+1]=0;
	if (values[i*123+2])
	    values[i*123+2]=1<<(values[i*123+2]-1);
	else
	    values[i*123+2]=0;
    }
    for (i=0; i < size; i++)
    {
	if (txtfile[i] != 255)
	    fputc (txtfile[i], fp);
	else
	    fprintf (fp, "%ld", values[n++]);
    }
    fclose (fp);
    /* Undo the change made to the jobs */
    for (i=0; i < 31; i++)
    {
	g=0;
	for (j=0; j < 15; j++)
	    if ((values[i*123+1]>>j)&1)
		g=j+1;
	values[i*123+1]=g;
	g=0;
	for (j=0; j < 15; j++)
	    if ((values[i*123+2]>>j)&1)
		g=j+1;
	values[i*123+2]=g;
    }
}

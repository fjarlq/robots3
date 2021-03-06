/*
 * R O B O T S
 *
 * The game of robots.
 * History:
 * Original Implementation
 *     Allan Black <allan@cs.strath.ac.uk>
 * Updated for fast robots, moveable heaps, ROBOTOPTS
 * antimatter, v7/sys5/4.2 portability etc.
 *     Graeme Lunt <gal@cs.nott.ac.uk>
 *     Julian Onions <jpo@cs.nott.ac.uk>
 *
 * Provided free as long as you don't make money from it!
 */

#include <stdarg.h>
#include <termios.h>
#include <sys/time.h>
#include "robots.h"

char whoami[MAXSTR];
char my_user_name[MAXSTR];

bool dead = FALSE;
bool last_stand;
bool show_highscore = TRUE;
bool running, adjacent, first_move, bad_move, waiting;
bool moveable_heaps = TRUE;

int my_x, my_y;
int new_x, new_y;
int level = 0;
int free_teleports = 0;
int free_per_level = 1;
int count;
int dots = 0;
int robot_value = MIN_VALUE;
int max_robots = MIN_ROBOTS;
int nrobots_alive;
int scrap_heaps = 1;            /* to allow for first level */

long score = 0;

void interrupt(int signum);

#define TERM_UNINIT 00
#define TERM_CURSES 01
#ifdef VDSUSP
# define TERM_DSUSP 02
void nodsusp(void);
static struct termios origtty;
#endif

int term_state = TERM_UNINIT;   /* cuts out some race conditions */

int main(int argc, char *argv[])
{
    struct passwd *pass;
    char *x;
    int i;

    setbuf(stdout, NULL);
    if (argc > 1) {
        if (argv[1][0] == '-') {
            switch (argv[1][1]) {
            case 's':
                show_highscore = TRUE;
                scoring(FALSE);
                return 0;
            }
        }
    }
    if ((pass = getpwuid(getuid())) == NULL) {
        x = "ANON";
    } else {
        x = pass->pw_name;
    }
    strcpy(my_user_name, x);
    strcpy(whoami, x);
    if ((x = getenv(ROBOTOPTS)) != NULL && *x != '\0')
        get_robot_opts(x);
    seed_rnd();
    signal(SIGQUIT, interrupt);
    signal(SIGINT, interrupt);
    if (initscr() == NULL) {
        fprintf(stderr, "Curses won't initialise - seek a guru\n");
        quit(FALSE);
    }
    term_state |= TERM_CURSES;
    crmode();
    noecho();
#ifdef VDSUSP
    nodsusp();
#endif
    for (;;) {
        count = 0;
        running = FALSE;
        adjacent = FALSE;
        waiting = FALSE;
        last_stand = FALSE;
        if (rnd(free_per_level) < free_teleports) {
            free_per_level++;
            if (free_per_level > MAX_FREE)
                free_per_level = MAX_FREE;
        }
        free_teleports += free_per_level;
        leaveok(stdscr, FALSE);
        draw_screen();
        put_robots();
        do {
            my_x = rndx();
            my_y = rndy();
            move(my_y, my_x);
        } while (inch() != ' ');
        addch(ME);
        for (;;) {
            scorer();
            if (nrobots_alive == 0)
                break;
            command();
            for (i = 1; i <= FASTEST; i++)
                robots(i);
            if (dead)
                munch();
        }
        msg("%d robots are now %d scrap heaps", max_robots, scrap_heaps);
        leaveok(stdscr, FALSE);
        move(my_y, my_x);
        refresh();
        readchar();
        level++;
    }
}

void draw_screen(void)
{
    int x, y;
    clear();
    for (y = 1; y < LINES - 2; y++) {
        mvaddch(y, 0, VERT);
        mvaddch(y, COLS - 1, VERT);
    }
    for (x = 0; x < COLS; x++) {
        mvaddch(0, x, HORIZ);
        mvaddch(LINES - 2, x, HORIZ);
    }
}

char readchar(void)
{
    char c;
    while (read(0, &c, 1) != 1) {
        if (errno != EINTR)
            quit(TRUE);
    }
    return c;
}

void put_dots(void)
{
    int x, y;
    for (x = my_x - dots; x <= my_x + dots; x++) {
        for (y = my_y - dots; y <= my_y + dots; y++) {
            move(y, x);
            if (inch() == ' ')
                addch(DOT);
        }
    }
}

void erase_dots(void)
{
    int x, y;
    for (x = my_x - dots; x <= my_x + dots; x++) {
        for (y = my_y - dots; y <= my_y + dots; y++) {
            move(y, x);
            if (inch() == DOT)
                addch(' ');
        }
    }
}

int xinc(char dir)
{
    switch (dir) {
    case 'h':
    case 'y':
    case 'b':
        return (-1);
    case 'l':
    case 'u':
    case 'n':
        return (1);
    case 'j':
    case 'k':
    default:
        return (0);
    }
}

int yinc(char dir)
{
    switch (dir) {
    case 'k':
    case 'y':
    case 'u':
        return (-1);
    case 'j':
    case 'b':
    case 'n':
        return (1);
    case 'h':
    case 'l':
    default:
        return (0);
    }
}

void munch(void)
{
    scorer();
    msg("MUNCH! You're robot food");
    leaveok(stdscr, FALSE);
    mvaddch(my_y, my_x, MUNCH);
    move(my_y, my_x);
    refresh();
    readchar();
    quit(TRUE);
}

void quit(bool eaten)
{
#ifdef VDSUSP
    if (term_state & TERM_DSUSP) {
        tcsetattr(0, TCSANOW, &origtty);
        term_state &= ~TERM_DSUSP;
    }
#endif
    if (term_state & TERM_CURSES) {
        move(LINES - 1, 0);
        refresh();
        endwin();
        term_state &= ~TERM_CURSES;
    }
    putchar('\n');
    signal(SIGINT, SIG_DFL);
    scoring(eaten);
    exit(0);
}

int rndx(void)
{
    return (rnd(COLS - 2) + 1);
}

int rndy(void)
{
    return (rnd(LINES - 3) + 1);
}

// Returns in the half-open interval [0, max)
// http://stackoverflow.com/a/6852396/393684
long rnd(long upper)
{
    unsigned long num_bins, num_rand, bin_size, defect;
    long x;

    if (upper <= 0)
        return 0;

    // upper <= RAND_MAX < ULONG_MAX, so this is okay.
    num_bins = (unsigned long)upper;
    num_rand = (unsigned long)RAND_MAX + 1;
    bin_size = num_rand / num_bins;
    defect   = num_rand % num_bins;

    do {
        x = random();
    }
    // This is carefully written not to overflow
    while (num_rand - defect <= (unsigned long)x);

    // Truncated division is intentional
    return x / bin_size;
}

void seed_rnd(void)
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == -1) {
        perror("seed_rnd: gettimeofday");
        exit(1);
    }
    srandom((getpid() << 16) ^ tv.tv_sec ^ tv.tv_usec);
}

void msg(char *message, ...)
{
    char msgbuf[1000];
    va_list args;

    va_start(args, message);
    vsprintf(msgbuf, message, args);
    va_end(args);
    mvaddstr(LINES - 1, MSGPOS, msgbuf);
    clrtoeol();
    refresh();
}

void interrupt(int signum)
{
    (void)signum;
    quit(FALSE);
}

/*
 * file locking routines - much nicer under BSD ...
 */

#ifdef USE_FLOCK
# include <sys/file.h>
/* lock a file using the flock sys call */
int lk_open(char *file, int mode)
{
    int fd;

    if ((fd = open(file, mode)) < 0)
        return -1;
    if (flock(fd, LOCK_EX) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

int lk_close(int fd, char *file)
{
    (void)file;                 /* now will you shut up lint???? */
    return close(fd);
}

#else

# define LOCKTIME (60)          /* 1 minute */
# include <sys/stat.h>

/* lock a file by crude means */
int lk_open(char *file, int mode)
{
    char tfile[128], lfile[128];
    struct stat stbuf;
    time_t now;
    int fd;

    sprintf(tfile, "%s.t", file); /* temp file */
    sprintf(lfile, "%s.l", file); /* lock file */

    if (close(creat(tfile, 0)) < 0)     /* make temp file */
        return -1;
    while (link(tfile, lfile) == -1) {  /* now attempt the lock file */
        if (stat(lfile, &stbuf) < 0)
            continue;           /* uhh? -- try again */
        time(&now);
        /* OK - is this file old? */
        if (stbuf.st_mtime + LOCKTIME < now)
            unlink(lfile);      /* ok its old enough - junk it */
        else
            sleep(1);           /* snooze ... */
    }
    unlink(tfile);              /* tmp files done its job */
    if ((fd = open(file, mode)) < 0) {
        unlink(lfile);
        return -1;
    }
    return fd;
}

int lk_close(int fd, char *fname)
{
    char lfile[128];

    sprintf(lfile, "%s.l", fname);        /* recreate the lock file name */
    if (unlink(lfile) == -1)    /* blow it away */
        perror(lfile);
    return close(fd);
}
#endif

#ifdef VDSUSP
void nodsusp(void)
{
    struct termios tty;
    if (tcgetattr(0, &origtty) == -1) {
        perror("tcgetattr");
        quit(FALSE);
        return;
    }
    tty = origtty;
    tty.c_cc[VDSUSP] = _POSIX_VDISABLE;
    if (tcsetattr(0, TCSANOW, &tty) == -1) {
        perror("tcsetattr");
        quit(FALSE);
        return;
    }
    term_state |= TERM_DSUSP;
}
#endif

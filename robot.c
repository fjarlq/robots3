#include "robots.h"

/*
 * robot.c: most of the robot oriented stuff
 */

/* place some robots */
void put_robots(void)
{
    struct robot *r, *end;
    int x, y;

    robot_value += level * (5 + (max_robots / scrap_heaps));
    max_robots += level * 3 + rnd(level * 5);
    if (max_robots > MAX_ROBOTS)
        max_robots = MAX_ROBOTS;
    nrobots_alive = max_robots;
    scrap_heaps = 0;            /* number of scrap heaps created */
    end = &rbt_list[max_robots];
    for (r = rbt_list; r < end; r++) {
        for (;;) {
            x = rndx();
            y = rndy();
            move(y, x);
            if (inch() == ' ')
                break;
        }
        r->x = x;
        r->y = y;
        r->alive = TRUE;
        r->speed = 1 + (rnd(10) > 6);
        if (r->speed == 2)
            addch(FROBOT);
        else
            addch(ROBOT);
    }
}

/* Troops, Troooooops advance! */
void robots(int speed)
{
    struct robot *r, *end;

    end = &rbt_list[max_robots];
    for (r = rbt_list; r < end; r++) {
        if (r->alive && r->speed >= speed) {
            mvaddch(r->y, r->x, ' ');
        }
    }
    for (r = rbt_list; r < end; r++) {
        if (r->alive && r->speed >= speed) {
            r->x += sign(my_x - r->x);
            r->y += sign(my_y - r->y);
            move(r->y, r->x);
            switch (inch()) {
            case ME:           /* robot eat me */
                addch(MUNCH);
                dead = TRUE;
                break;
            case SCRAP:        /* robot walks into scrap heap */
                r->alive = FALSE;
                if (r->speed == 2)
                    score += (MULT * robot_value);
                else
                    score += robot_value;
                nrobots_alive--;
                break;
            case FROBOT:
            case ROBOT:        /* two robots form scrap heap */
                collision(r, end);
                r->alive = FALSE;
                addch(SCRAP);
                if (r->speed == 2)
                    score += (MULT * robot_value);
                else
                    score += robot_value;
                nrobots_alive -= 2;
                scrap_heaps++;
                break;
            case MUNCH:
                break;
            default:
                if (r->speed == 2)
                    addch(FROBOT);
                else
                    addch(ROBOT);
            }
        }
    }
}

/* do two robots collide - if so - which two */
void collision(struct robot *r, struct robot *end)
{
    struct robot *find;

    for (find = rbt_list; find < end; find++) {
        if (find->alive && r != find) {
            if (r->x == find->x && r->y == find->y) {
                find->alive = FALSE;
                if (find->speed == 2)
                    score += (MULT * robot_value);
                else
                    score += robot_value;
                return;
            }
        }
    }
}

/* dismantle those robots ... */
void screwdriver(void)
{
    int test_x, test_y;
    struct robot *end, *find;
    char *m, *moves = "hjklyubn.";

    end = &rbt_list[max_robots];

    for (m = moves; *m; m++) {  /* let's see if there is a robot */
        test_x = my_x + xinc(*m);
        test_y = my_y + yinc(*m);
        move(test_y, test_x);
        switch (inch()) {
        case FROBOT:
        case ROBOT:            /* OK so there is now let's look for it */
            for (find = rbt_list; find < end; find++) {
                if (find->alive) {
                    if (test_x == find->x && test_y == find->y) {
                        find->alive = FALSE;
                        if (find->speed == 2)
                            score += (MULT * robot_value);
                        else
                            score += robot_value;
                    }
                }
            }
            addch(' ');
            nrobots_alive--;
            break;
        }
    }
    free_teleports--;
}

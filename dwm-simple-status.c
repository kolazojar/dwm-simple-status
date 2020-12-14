#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#define INTERVAL 5
#define BUF_SIZE 129
#define NUM_STAT 2
#define BAT_DIR "/sys/class/power_supply/BAT0/"
#define BAT_FILE(NAME) BAT_DIR NAME


const char date_format[] = "%a %b %d %Y %R";

static Display *dpy;

void get_datetime(char *str, size_t size)
{
    time_t now = time(NULL);

    if (now == -1) {
        *str = '\0';
        return;
    }

    struct tm *ptm = localtime(&now);

    if (ptm == NULL) {
        *str = '\0';
        return;
    }

    size_t len = strftime(str, size-1, date_format, ptm);

    if (len == 0) {
        *str = '\0';
    }
}

void read_file(char *path, char *line, size_t size)
{
    FILE *fd;

    fd = fopen(path, "r");
    if (fd == NULL) {
        *line = '\0';
        return;
    }

    if (fgets(line, size-1, fd) == NULL) {
        *line = '\0';
    }
    fclose(fd);
}

void get_battery(char *str, size_t size)
{
    char capacity[BUF_SIZE];
    char status[BUF_SIZE];

    read_file(BAT_FILE("capacity"), capacity, BUF_SIZE-1);
    read_file(BAT_FILE("status"), status, BUF_SIZE-1);

    if (*status == '\0' || *capacity == '\0') {
        *str = '\0';
        return;
    }

    snprintf(str, size, "%s %s", status, capacity);
}

int main(void)
{
    if (!(dpy = XOpenDisplay(NULL))) {
        fputs("dwm-simple-status: cannot open display.", stderr);
        return 1;
    }

    char status[BUF_SIZE*NUM_STAT];

    char datetime[BUF_SIZE];

    char battery[BUF_SIZE];

    for (;;sleep(INTERVAL)) {
        get_datetime(datetime, BUF_SIZE);
        get_battery(battery, BUF_SIZE);

        snprintf(status, BUF_SIZE*NUM_STAT, "B: %s | %s", battery, datetime);

        XStoreName(dpy, DefaultRootWindow(dpy), status);
        XSync(dpy, 0);
    }

    XCloseDisplay(dpy);

    return 0;
}

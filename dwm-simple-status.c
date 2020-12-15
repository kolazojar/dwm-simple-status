#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

#include <X11/Xlib.h>

#define INTERVAL 10
#define DATE_FMT "%a %b %d %Y %R"
#ifdef BATTERY
#define BAT_DIR "/sys/class/power_supply/BAT0/"
#define BAT_FILE(NAME) BAT_DIR NAME
#endif

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

    size_t len = strftime(str, size-1, DATE_FMT, ptm);

    if (len == 0) {
        *str = '\0';
    }
}

#ifdef BATTERY
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

    size_t i;

    for (i = 0; line[i] != '\0'; i++);

    if (i > 0 && line[--i] == '\n') {
        line[i] = '\0';
    }

    fclose(fd);
}

void get_battery(char *str, size_t size)
{
    char capacity[1 << 6];
    char status[1 << 6];

    read_file(BAT_FILE("capacity"), capacity, 1 << 6);
    read_file(BAT_FILE("status"), status, 1 << 6);

    snprintf(str, size, "%s %s%%", status, capacity);
}
#endif

int main(void)
{
    if (!(dpy = XOpenDisplay(NULL))) {
        fputs("dwm-simple-status: cannot open display.", stderr);
        return 1;
    }

    char status[1 << 9];
    char datetime[1 << 7];
#ifdef BATTERY
    char battery[(1 << 7) + 3];
#endif

    for (;;sleep(INTERVAL)) {
#ifdef BATTERY
        get_datetime(datetime, 1 << 7);
        get_battery(battery, (1 << 7) + 2);
        snprintf(status, 1 << 9, " %s | %s", battery, datetime);
#else
        get_datetime(datetime, 1 << 7);
        snprintf(status, 1 << 9, " %s", datetime);
#endif

        XStoreName(dpy, DefaultRootWindow(dpy), status);
        XSync(dpy, 0);
    }

    XCloseDisplay(dpy);

    return 0;
}

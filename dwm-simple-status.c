#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <X11/Xlib.h>

#define INTERVAL 1
#define DATE_FMT "%a %b %d %T"
#define SBUF_SIZE (1 << 4)
#define MBUF_SIZE (1 << 6)
#define LBUF_SIZE (1 << 8)
#define TEMP_DIR "/sys/devices/platform/coretemp.0/hwmon"
#define TEMP_GUESS "/sys/class/hwmon/hwmon0/temp1_input"
#ifdef BATTERY
#define BAT_DIR "/sys/class/power_supply/BAT0/"
#define BAT_FILE(NAME) BAT_DIR NAME
#endif

void read_file(char *path, char *line, size_t size)
{
    FILE *fd;

    fd = fopen(path, "r");

    if (fd == NULL) {
        *line = '\0';
        return;
    }

    if (fgets(line, size, fd) == NULL) {
        *line = '\0';
    }

    fclose(fd);

    size_t i;

    for (i = 0; line[i] != '\0'; i++);

    if (i > 0 && line[--i] == '\n') {
        line[i] = '\0';
    }
}

void get_temp_file(char *str, size_t size)
{
    DIR *d;
    struct dirent *dir;

    d = opendir(TEMP_DIR);

    char not_found = 1;

    if (d != NULL) {
        while (not_found && (dir = readdir(d)) != NULL) {
            if (strstr(dir->d_name, "hwmon") != NULL) {
                not_found = 0;
            }
        }
    }

    if (!not_found) {
        snprintf(str, size, "%s/%s/temp1_input", TEMP_DIR, dir->d_name);
    } else {
        snprintf(str, size, "%s", TEMP_GUESS);
    }
}

void get_temp(char *str, size_t size, char *temp_file)
{
    char temp[SBUF_SIZE];

    read_file(temp_file, temp, SBUF_SIZE);

    if (temp[0] == '\0') {
        *str = '\0';
    } else {
        snprintf(str, size, "%.0f°C", atof(temp)/1000.0);
    }
}

void get_datetime(char *str, size_t size)
{
    time_t now = time(NULL);

    if (!strftime(str, size, DATE_FMT, localtime(&now))) {
        *str = '\0';
    }
}

#ifdef BATTERY
void get_battery(char *str, size_t size)
{
    char capacity[SBUF_SIZE];
    char status[SBUF_SIZE];

    read_file(BAT_FILE("capacity"), capacity, SBUF_SIZE);
    read_file(BAT_FILE("status"), status, SBUF_SIZE);

    if (capacity[0] == '\0' || status[0] == '\0') {
        *str = '\0';
    } else {
        char sym;

        if (!strcmp(status, "Discharging")) {
            sym = '-';
        } else if (!strcmp(status, "Charging")) {
            sym = '+';
        } else if (!strcmp(status, "Full")) {
            sym = '=';
        } else {
            sym = '?';
        }

        snprintf(str, size, "%c%s%%", sym, capacity);
    }
}
#endif

int main(void)
{
    Display *dpy;

    if (!(dpy = XOpenDisplay(NULL))) {
        fputs("dwm-simple-status: cannot open display.", stderr);
        return 1;
    }

    char temp_file[MBUF_SIZE];
    get_temp_file(temp_file, MBUF_SIZE);

    char status[LBUF_SIZE];

    char datetime[MBUF_SIZE];
    char temp[SBUF_SIZE];
#ifdef BATTERY
    char battery[SBUF_SIZE];
#endif

    for (;;sleep(INTERVAL)) {
#ifdef BATTERY
        get_datetime(datetime, MBUF_SIZE);
        get_temp(temp, SBUF_SIZE, temp_file);
        get_battery(battery, SBUF_SIZE);
        snprintf(status, LBUF_SIZE, " %s | %s | %s", temp, battery, datetime);
#else
        get_datetime(datetime, MBUF_SIZE);
        get_temp(temp, SBUF_SIZE, temp_file);
        snprintf(status, LBUF_SIZE, " %s | %s", temp, datetime);
#endif
        XStoreName(dpy, DefaultRootWindow(dpy), status);
        XSync(dpy, 0);
    }

    XCloseDisplay(dpy);

    return 0;
}

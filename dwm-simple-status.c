#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#ifdef LAPTOP
#include <signal.h>
#endif

#include <X11/Xlib.h>

#define INTERVAL 10
#define DATE_FMT "%a %b %d %Y %R"
#define TEMP_DIR "/sys/devices/platform/coretemp.0/hwmon"
#define TEMP_GUESS "/sys/class/hwmon/hwmon0/temp1_input"
#ifdef LAPTOP
#define BAT_DIR "/sys/class/power_supply/BAT0/"
#define BAT_FILE(NAME) BAT_DIR NAME
#define BACKLIGHT_DIR "/sys/class/backlight/intel_backlight/"
#define BACKLIGHT_FILE(NAME) BACKLIGHT_DIR NAME
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

    size_t len = strftime(str, size, DATE_FMT, ptm);

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

#ifdef LAPTOP
void get_battery(char *str, size_t size)
{
    char capacity[1 << 5];
    char status[1 << 5];

    read_file(BAT_FILE("capacity"), capacity, 1 << 5);
    read_file(BAT_FILE("status"), status, 1 << 5);

    if (capacity[0] == '\0' || status[0] == '\0') {
        *str = '\0';
    } else {
        snprintf(str, size, "%s %s%%", status, capacity);
    }
}

void get_backlight(char *str, size_t size)
{
    char current[1 << 5];
    char max[1 << 5];

    read_file(BACKLIGHT_FILE("actual_brightness"), current, 1 << 5);
    read_file(BACKLIGHT_FILE("max_brightness"), max, 1 << 5);

    if (current[0] == '\0' || max[0] == '\0') {
        *str = '\0';
    } else {
        snprintf(str, size, "Backlight %02.0f%%", atof(current)/atof(max)*100);
    }
}
#endif

void get_temp_file(char *str, size_t size)
{
    DIR *d;
    struct dirent *dir;

    d = opendir(TEMP_DIR);

    char not_found = 1;

    if (d != NULL) {
        while (((dir = readdir(d)) != NULL) && (not_found == 1)) {
            if (strstr(dir->d_name, "hwmon") != NULL) {
                not_found = 0;
                break;
            }
        }
    }

    if (not_found == 0) {
        snprintf(str, size, "%s/%s/temp1_input", TEMP_DIR, dir->d_name);
    } else {
        snprintf(str, size, "%s", TEMP_GUESS);
    }
}

void get_temp(char *str, size_t size, char *temp_file)
{
    char temp[1 << 4];

    read_file(temp_file, temp, 1 << 4);

    if (temp[0] == '\0') {
        *str = '\0';
    } else {
        snprintf(str, size, "CPU %02.0f°C", atof(temp)/1000.0);
    }
}

#ifdef LAPTOP
void handler(int sig)
{
    signal(SIGINT, handler);
}
#endif

int main(void)
{
    if (!(dpy = XOpenDisplay(NULL))) {
        fputs("dwm-simple-status: cannot open display.", stderr);
        return 1;
    }

    char temp_file[1 << 8];
    get_temp_file(temp_file, 1 << 8);

    char status[1 << 9];
    char datetime[1 << 6];
    char temp[1 << 4];
#ifdef LAPTOP
    char backlight[1 << 7];
    char battery[1 << 7];
    signal(SIGINT, handler);
#endif

    for (;;sleep(INTERVAL)) {
#ifdef LAPTOP
        get_datetime(datetime, 1 << 6);
        get_temp(temp, 1 << 4, temp_file);
        get_battery(battery, 1 << 7);
        get_backlight(backlight, 1 << 7);
        snprintf(status, 1 << 9, " %s | %s | %s | %s", backlight, temp, battery, datetime);
#else
        get_datetime(datetime, 1 << 6);
        get_temp(temp, 1 << 4, temp_file);
        snprintf(status, 1 << 9, " %s | %s", temp, datetime);
#endif
        XStoreName(dpy, DefaultRootWindow(dpy), status);
        XSync(dpy, 0);
    }

    XCloseDisplay(dpy);

    return 0;
}

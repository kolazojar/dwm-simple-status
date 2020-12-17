#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#ifdef LAPTOP
#include <signal.h>
#endif

#include <X11/Xlib.h>

#define INTERVAL 10
#define DATE_FMT "%a %b %d %Y %R"
#define SBUF_SIZE (1 << 4)
#define MBUF_SIZE (1 << 6)
#define LBUF_SIZE (1 << 8)
#define TEMP_DIR "/sys/devices/platform/coretemp.0/hwmon"
#define TEMP_GUESS "/sys/class/hwmon/hwmon0/temp1_input"

#ifdef LAPTOP
#define BAT_DIR "/sys/class/power_supply/BAT0/"
#define BAT_FILE(NAME) BAT_DIR NAME
#define BACKLIGHT_DIR "/sys/class/backlight/intel_backlight/"
#define BACKLIGHT_FILE(NAME) BACKLIGHT_DIR NAME
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

void get_mem(char *str, size_t size)
{
    struct sysinfo info;

    if (!sysinfo(&info)) {
        double used = 100.0*(1.0 - ((double)info.freeram)/((double)info.totalram));
        snprintf(str, size, "%.0f%%", used);
    } else {
        *str = '\0';
    }
}

void get_disk(char *str, size_t size, char *path)
{
    struct statvfs stats;

    if (!statvfs(path, &stats)) {
        double used = 100.0*(1.0 - ((double)stats.f_bfree)/((double)stats.f_blocks));
        snprintf(str, size, "%.0f%%", used);
    } else {
        *str = '\0';
    }
}

void get_datetime(char *str, size_t size)
{
    time_t now = time(NULL);

    if (!strftime(str, size, DATE_FMT, localtime(&now))) {
        *str = '\0';
    }
}

#ifdef LAPTOP
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

void get_backlight(char *str, size_t size)
{
    char current[SBUF_SIZE];
    char max[SBUF_SIZE];

    read_file(BACKLIGHT_FILE("actual_brightness"), current, SBUF_SIZE);
    read_file(BACKLIGHT_FILE("max_brightness"), max, SBUF_SIZE);

    if (current[0] == '\0' || max[0] == '\0') {
        *str = '\0';
    } else {
        snprintf(str, size, "%.0f%%", atof(current)/atof(max)*100);
    }
}

void handler(int sig)
{
    signal(SIGUSR1, handler);
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
    char mem[SBUF_SIZE];
    char disk1[SBUF_SIZE];
#ifndef LAPTOP
    char disk2[SBUF_SIZE];
#endif
#ifdef LAPTOP
    char backlight[SBUF_SIZE];
    char battery[MBUF_SIZE];

    signal(SIGUSR1, handler);
#endif

    for (;;sleep(INTERVAL)) {
#ifdef LAPTOP
        get_datetime(datetime, MBUF_SIZE);
        get_temp(temp, SBUF_SIZE, temp_file);
        get_battery(battery, MBUF_SIZE);
        get_backlight(backlight, SBUF_SIZE);
        get_mem(mem, SBUF_SIZE);
        get_disk(disk1, SBUF_SIZE, "/");
        snprintf(status, LBUF_SIZE, " Brig %s | Disk %s | Mem %s | Temp %s | Bat %s | %s", backlight, disk1, mem, temp, battery, datetime);
#else
        get_datetime(datetime, MBUF_SIZE);
        get_temp(temp, SBUF_SIZE, temp_file);
        get_mem(mem, SBUF_SIZE);
        get_disk(disk1, SBUF_SIZE, "/");
        get_disk(disk2, SBUF_SIZE, "/home");
        snprintf(status, LBUF_SIZE, " Disk / %s /home %s | Mem %s | Temp %s | %s", disk1, disk2, mem, temp, datetime);
#endif
        XStoreName(dpy, DefaultRootWindow(dpy), status);
        XSync(dpy, 0);
    }

    XCloseDisplay(dpy);

    return 0;
}

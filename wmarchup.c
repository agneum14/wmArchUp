/* Also includes Xlib, Xresources, XPM, stdlib and stdio */
#include <dockapp.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

/* Include the pixmap to use */
#include "archlinux_red.xpm"
#include "archlinux_green.xpm"
#include "archlinux_gray.xpm"

#define TRUE            1
#define FALSE           0
#define MAX             256
#define MAXPATHLEN      4096
#define CHK_INTERVAL    600
#define WMARCHUP_VER    "1.2"
#define VERSION         "wmArchUp version " WMARCHUP_VER

/*
 * Prototypes for local functions
 */
void destroy(void);
void check_for_updates();
void buttonrelease(int button, int state, int x, int y);
void update();
char *get_update_script();

/*
 * Global variables
 */
Pixmap arch_red, arch_red_mask, arch_green, arch_green_mask, arch_gray, arch_gray_mask;
unsigned short  height, width;
unsigned int check_interval = CHK_INTERVAL;
int updates_available = FALSE;
char *script;
char *aur_helper;
char *term;

/*
 * M A I N
 */

int
main(int argc, char **argv)
{
    /* Find bash update script */
    script = get_update_script();

    /* Set callbacks for events */
    DACallbacks eventCallbacks = {
        destroy,            /* destroy */
        NULL,               /* buttonPress */
        &buttonrelease,     /* buttonRelease */
        NULL,               /* motion (mouse) */
        NULL,               /* mouse enters window */
        NULL,               /* mouse leaves window */
        check_for_updates   /* timeout */
    };

    int interval = 0;

    /* Set program options */
    DAProgramOption options[] = {
        {
            "-a",
            "--aur-helper",
            "AUR helper (e.g. yay). Default none.",
            DOString,
            False,
            {&aur_helper}
        },
        {
            "-t",
            "--terminal-emulator",
            "Terminal to launch system update. Default xterm.",
            DOString,
            False,
            {&term}
        },
        {
            "-c",
            "--check-interval",
            "Check interval in minutes. Default 10 minutes.",
            DONatural,
            False,
            {&interval}
        },
    };

    /* provide standard command-line options */
    DAParseArguments(
        argc, argv, /* Where the options come from */
        options, 3, /* Our list with options */
        "This is dockapp watch for available updates "
        "in Arch Linux packages.\n",
        VERSION);

    /* Set the check interval */
    check_interval = interval ? (interval * 60) : CHK_INTERVAL;

    /* Check if correct libdockapp version exists in system */
    DASetExpectedVersion(20050716);

    DAInitialize("", "WMArchUp", 56, 56, argc, argv);

    DAMakePixmapFromData(
        archlinux_red,
        &arch_red,
        &arch_red_mask,
        &height,
        &width);

    DAMakePixmapFromData(
        archlinux_green,
        &arch_green,
        &arch_green_mask,
        &height,
        &width);

    DAMakePixmapFromData(
        archlinux_gray,
        &arch_gray,
        &arch_gray_mask,
        &height,
        &width);

    /* Respond to destroy and timeout events (the ones not NULL in the
     * eventCallbacks variable.
     */
    DASetCallbacks(&eventCallbacks);

    /* Set timeout in msec */
    DASetTimeout(check_interval * 1000);

    /* Show the dockapp window. */
    DAShow();

    /* Check for updates at the begining */
    check_for_updates();

    /* Loop for events */
    DAEventLoop();

    /* not reached */
    exit(EXIT_SUCCESS);
}

void add_arg(char *arg, char *def_arg) {
    size_t LENGTH = strlen(script);
    script[LENGTH] = ' ';
    script[LENGTH+1] = '\0';
    if (arg) strcat(script, arg);
    else strcat(script, def_arg);
}

void
update()
{
    if (updates_available == TRUE) {
        XSelectInput(DAGetDisplay(NULL), DAGetWindow(), NoEventMask);

    add_arg(aur_helper, "pacman");
    add_arg(term, "xterm");
        int ret = system(script);

        if (WEXITSTATUS(ret) == 0) {
            updates_available = FALSE;
            DASetShape(arch_green_mask);
            DASetPixmap(arch_green);
        }

        XSelectInput(DAGetDisplay(NULL), DAGetWindow(),
                     ButtonPressMask | ButtonReleaseMask);
    }
}

void
check_for_updates()
{
    XSelectInput(DAGetDisplay(NULL), DAGetWindow(), NoEventMask);
    char res[MAX];


    DASetShape(arch_gray_mask);
    DASetPixmap(arch_gray);

    /* Read output from command */
    FILE *fp = popen("checkupdates 2>&1 | head -n 1", "r");
    /* check for AUR updates */
    int aur_update = 1;
    if (aur_helper) {
    char command[50];
    strcpy(command, aur_helper);
    strcat(command, " -Qum 2>&1 | head -n 1");
    FILE *fp2 = popen(command, "r");
    if (fgets(res, MAX, fp2) != NULL) aur_update = 0;
    pclose(fp2);
    }
    if (fgets(res, MAX, fp) != NULL || aur_update == 0) {
        updates_available = TRUE;
        DASetShape(arch_red_mask);
        DASetPixmap(arch_red);
    } else {
        updates_available = FALSE;
        DASetShape(arch_green_mask);
        DASetPixmap(arch_green);
    }

    pclose(fp);

    XSelectInput(DAGetDisplay(NULL), DAGetWindow(),
                 ButtonPressMask | ButtonReleaseMask);
}

void
destroy(void)
{
    return;
}

/* A mouse button was pressed and released.
 * See if it was button 1 left click or button 3 right click. */
void
buttonrelease(int button, int state, int x, int y)
{
    if (button == 1) {
        update();
    } else if (button == 3) {
        check_for_updates();
    }
}

char *
get_update_script()
{
    int length;
    char *p;
    char *script_name = "arch_update.sh";

    char *fullpath = malloc(MAXPATHLEN + strlen(script_name));
    if (fullpath == NULL) {
        perror("Can't allocate memory.");
        exit(1);
    }

    /*
     * /proc/self is a symbolic link to the process-ID subdir of /proc, e.g.
     * /proc/4323 when the pid of the process of this program is 4323.
     * Inside /proc/<pid> there is a symbolic link to the executable that is
     * running as this <pid>.  This symbolic link is called "exe". So if we
     * read the path where the symlink /proc/self/exe points to we have the
     * full path of the executable.
     */

    length = readlink("/proc/self/exe", fullpath, MAXPATHLEN);

    /*
     * Catch some errors:
     */
    if (length < 0) {
        perror("resolving symlink /proc/self/exe.");
        exit(1);
    }
    if (length >= MAXPATHLEN) {
        fprintf(stderr, "Path too long.\n");
        exit(1);
    }

    fullpath[length] = '\0';
    if ((p = strrchr(fullpath, '/'))) {
        *(p + 1) = '\0';
    }
    strcat(fullpath, script_name);

    return fullpath;
}

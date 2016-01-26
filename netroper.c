#include "common.h"


#include <errno.h>
#include <ftw.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "fail.h"


#define MAX_VERBOSITY 3
#define NR_DIR "/tmp/netroper/"
#define WPA_CTRL NR_DIR "wpa_supplicant.sock"


int verbosity;


struct args {
    enum net_action {
        ACT_MANUAL_CONNECT,
    } action;
    const char* iface;
} handle_args(int argc, char** argv)
{
    int c;
    opterr = 0;
    while ((c = getopt(argc, argv, "+hv")) != -1) {
        if (c == 'h') {
            print(
                "Usage:  netroper [OPTIONS] action\n"
                "\n"
                "Options:\n"
                "  -h   Print this help text\n"
                "  -v   Increase verbosity (up to a maximum of "
                    str(MAX_VERBOSITY) ")\n"
                "Actions:\n"
                "  connect IFACE   Connect to a new network on IFACE\n"
            );
            exit(E_USAGE);
        } else if (c == 'v') {
            if (verbosity < MAX_VERBOSITY)
                ++verbosity;
        } else if (c == '?') {
            fatal(E_USAGE, "Invalid option \"-%c\"", optopt);
        }
    }

    struct args args;
    const char* const action = argv[optind];
    if (action == NULL)
        fatal(E_USAGE, "No action specified (try `netroper -h`)");
    if (strcmp(action, "connect") == 0) {
        if (argv[optind + 1] == NULL)
            fatal(E_USAGE, "No interface specified");
        args.action = ACT_MANUAL_CONNECT;
        args.iface = argv[optind + 1];
    } else {
        fatal(E_USAGE, "Invalid action");
    }
    return args;
}


int _rmdir_recursive_fn(const char* fpath, const struct stat* sb, int typeflag,
        struct FTW* ftwbuf)
{
    // Don't check file types. It's less racey to just try both calls.
    if (unlink(fpath) == 0)
        return 0; // continue traversal
    else if (rmdir(fpath) == 0)
        return 0; // continue traversal
    else
        fatal_e(E_RARE, "Can't remove \"%s\"", fpath);
    return 1;
}


void rmdir_recursive(const char* const path)
{
    int ret = nftw(path, _rmdir_recursive_fn, 16, FTW_DEPTH | FTW_PHYS);
    if (!(ret == 0 || errno == ENOENT))
        fatal_e(E_RARE, "Can't recursively remove " NR_DIR);
}


void net_manual_connect(const char* const iface)
{
    v1("Scanning...");

    rmdir_recursive(NR_DIR);

    if (mkdir(NR_DIR, 0700) != 0)
        fatal_e(E_COMMON, "Can't create " NR_DIR);

    pid_t p = fork();
    if (p == 0) {
        // child
        execlp(
            "wpa_supplicant", "wpa_supplicant",
            "-c", "/dev/null",
            "-d",
            "-g", WPA_CTRL,
            "-i", iface,
            "-P", NR_DIR "wpa_supplicant.pid",
            "-t",
            NULL
        );
        fatal_e(E_RARE, "Can't run wpa_supplicant");
    }

    /*struct wpa_ctrl* wc = wpa_ctrl_open(WPA_CTRL);*/
    /*if (wc == NULL)*/
        /*fatal(E_RARE, "Can't open wpa_supplicant control socket");*/

    while (waitpid(p, NULL, 0) != p) { /* spin */ }
}


int main(int argc, char** argv)
{
    struct args args = handle_args(argc, argv);

    if (args.action == ACT_MANUAL_CONNECT)
        net_manual_connect(args.iface);
    else
        fatal(E_RARE, "BUG");

    return 0;
}

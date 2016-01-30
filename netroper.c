#include "common.h"


#include <errno.h>
#include <ftw.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "fail.h"
#include "wpa_ctrl.h"


#define MAX_VERBOSITY 3
#define NR_DIR "/tmp/netroper/"
#define WPA_CTRL NR_DIR "wpa_supplicant.sock"


int verbosity;

pid_t child = 0;


void kill_child()
{
    if (child != 0)
        kill(child, SIGTERM);
}


void handle_sig(int signum)
{
    (void)signum;

    char err[] = "!!! Killing wpa_supplicant and exiting !!!\n";
    write(STDERR_FILENO, err, sizeof(err));

    kill_child();
}


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

    {
        struct sigaction sa = {
            .sa_handler = handle_sig,
            .sa_flags = 0,
        };
        if (sigemptyset(&sa.sa_mask) != 0)
            fatal_e(E_RARE, "BUG");
        if (sigaddset(&sa.sa_mask, SIGINT) != 0)
            fatal_e(E_RARE, "BUG");
        if (sigaddset(&sa.sa_mask, SIGTERM) != 0)
            fatal_e(E_RARE, "BUG");
        if (sigaction(SIGINT, &sa, NULL) != 0)
            fatal_e(E_RARE, "BUG");
        if (sigaction(SIGTERM, &sa, NULL) != 0)
            fatal_e(E_RARE, "BUG");

        if (atexit(kill_child) != 0)
            fatal(E_RARE, "BUG");
    }

    child = fork();
    if (child == 0) {
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

    v1("Connecting to wpa_supplicant");
    struct wpa_ctrl* wc_cmd;
    do {
        wc_cmd = wpa_ctrl_open(WPA_CTRL);
    } while (wc_cmd == NULL);
    struct wpa_ctrl* wc_ev = wpa_ctrl_open(WPA_CTRL);
    if (wc_ev == NULL)
        fatal(E_RARE, "Can't open wpa_supplicant control sockets");
    if (wpa_ctrl_attach(wc_ev) != 0)
        fatal(E_RARE, "Can't register as event monitor");

    {
        v1("Sending PING");
        char reply[4096];
        size_t reply_len;
        wpa_ctrl_request(wc_cmd, "PING", sizeof("PING"), reply, &reply_len,
            NULL);
        v1("Received \"%s\"", reply);
    }

    v1("Disconnecting from wpa_supplicant");
    if (wpa_ctrl_detach(wc_ev) != 0)
        warning("Can't unregister event monitor");
    wpa_ctrl_close(wc_ev);
    wpa_ctrl_close(wc_cmd);

    v1("Waiting for wpa_supplicant to exit");
    while (waitpid(child, NULL, 0) != child) { /* spin */ }
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

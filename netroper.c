#include "common.h"


#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "fail.h"


#define MAX_VERBOSITY 3


int verbosity;


struct args {
    enum net_action {
        ACT_SCAN,
    } action;
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
                "  -h    Print this help text\n"
                "  -v    Increase verbosity (up to a maximum of "
                    str(MAX_VERBOSITY) ")\n"
                "Actions:\n"
                "  scan  Scan for networks and print the results\n"
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
    if (strcmp(action, "scan") == 0)
        args.action = ACT_SCAN;
    else
        fatal(E_USAGE, "Invalid action");
    return args;
}


void net_scan()
{
    v1("Scanning...");
}


int main(int argc, char** argv)
{
    struct args args = handle_args(argc, argv);

    if (args.action == ACT_SCAN)
        net_scan();
    else
        fatal(E_RARE, "BUG");

    return 0;
}

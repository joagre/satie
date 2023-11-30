#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <libgen.h>
#include "log.h"
#include "loader.h"
#include "util.h"

#define SUCCESS 0
#define PARAMETER_ERROR 1
#define DEFAULT_CHECK_AFTER 100
#define DEFAULT_LOAD_PATH "./"
#define DEFAULT_TIME_SLICE 25

void usage(const char* name);

int main(int argc, char* argv[]) {
    uint16_t check_after = DEFAULT_CHECK_AFTER;
    char* load_path = DEFAULT_LOAD_PATH;
    uint32_t time_slice = DEFAULT_TIME_SLICE;
    satie_error_t satie_error;

    // Parse command line options
    struct option long_options[] = {
        {"time-slice", required_argument, 0, 't'},
        {"check-after", required_argument, 0, 'c'},
        {"load-path", required_argument, 0, 'l'},
        {0, 0, 0, 0}
    };
    int opt;
    int option_index = 0;
    while ((opt = getopt_long(argc, argv, "t:c:l:i:", long_options,
                              &option_index)) != -1) {
        switch (opt) {
        case 't': {
            time_slice = string_to_long(optarg, &satie_error);
            if (satie_error.failed) {
                usage(basename(argv[0]));
            }
            break;
        }
        case 'c': {
            check_after = string_to_long(optarg, &satie_error);
            if (satie_error.failed) {
                usage(basename(argv[0]));
            }
            break;
        }
        case 'l':
            load_path = optarg;
            break;
        default:
            usage(basename(argv[0]));
        }
    }

    if (argc < 3) {
        usage(basename(argv[0]));
    }

    // Parse positional arguments
    const char* module_name = argv[optind];
    uint32_t label = string_to_long(argv[optind + 1], &satie_error);
    if (satie_error.failed) {
        usage(basename(argv[0]));
    }
    long parameters[argc - optind];
    for (int j = 0, i = optind + 2; i < argc; i++) {
        parameters[j++] = string_to_long(argv[i], &satie_error);
        if (satie_error.failed) {
            usage(basename(argv[0]));
        }
    }

    SATIE_LOG(LOG_LEVEL_DEBUG, "check_after = %d", check_after);
    SATIE_LOG(LOG_LEVEL_DEBUG, "load_path = %s", load_path);
    SATIE_LOG(LOG_LEVEL_DEBUG, "time_slice = %d", time_slice);
    SATIE_LOG(LOG_LEVEL_DEBUG, "module_name = %s", module_name);
    SATIE_LOG(LOG_LEVEL_DEBUG, "label = %d", label);
    for (int i = 0; i < argc - optind - 2; i++) {
        SATIE_LOG(LOG_LEVEL_DEBUG, "parameter = %d", parameters[i]);
    }

    // Prepare loader
    loader_t loader;
    loader_init(&loader, load_path);
    loader_load_module(&loader, module_name, &satie_error);
    loader_load_module(&loader, "ackermannr", &satie_error);

    satie_print_error(&satie_error);

    return SUCCESS;
}

void usage(const char* name) {
    fprintf(stderr,
            "Usage: %s [options] <module> <label> [<parameter> ...]\n"
            "Options:\n"
            "  -c <instructions>, --check-after=<instructions>\n"
            "    Check time slice timeout each number of <instructions> (%d)\n"
            "  -h, --help\n"
            "    Print this message and exit\n"
            "  -i, interpreter-mode <mode>\n"
            "    Start interpreter in 'stack' or 'register' <mode>\n"
            "  -l <directory>, --load-path=<directory>\n"
            "    Load POSM files from <directory> (%s)\n"
            "  -t <milli-seconds>, --time-slice=<milli-seconds>\n"
            "    <milli-seconds> spent by each job before context switch (%d) "
            "ms)\n",
            name, DEFAULT_CHECK_AFTER, DEFAULT_LOAD_PATH, DEFAULT_TIME_SLICE);
    exit(PARAMETER_ERROR);
}
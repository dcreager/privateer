/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <libcork/cli.h>
#include <libcork/core.h>
#include <libcork/ds.h>
#include <yaml.h>

#include "privateer/registry.h"

#define ri_check_exit(call) \
    do { \
        if ((call) != 0) { \
            fprintf(stderr, "%s\n", cork_error_message()); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)

#define rp_check_exit(call) \
    do { \
        if ((call) == NULL) { \
            fprintf(stderr, "%s\n", cork_error_message()); \
            exit(EXIT_FAILURE); \
        } \
    } while (0)


/*-----------------------------------------------------------------------
 * --registry option
 */

typedef cork_array(const char *)  path_array;
static path_array  registry_paths;

static void
registry_paths_init(void)
{
    cork_array_init(&registry_paths);
}

static void
registry_paths_done(void)
{
    cork_array_done(&registry_paths);
}

static void
registry_paths_add(const char *path)
{
    cork_array_append(&registry_paths, path);
}

static void
registry_add_paths(struct pvt_registry *reg)
{
    size_t  i;
    for (i = 0; i < cork_array_size(&registry_paths); i++) {
        const char  *path = cork_array_at(&registry_paths, i);
        ri_check_exit(pvt_registry_add_directory(reg, path, ".yaml"));
    }
}

#define REGISTRY_SHORTOPTS  "r:"

#define REGISTRY_LONGOPTS \
    { "registry", required_argument, NULL, 'r' }

#define REGISTRY_HELP \
"  --registry <registry path>\n" \
"    The location of a local Privateer registry.  You can provide this\n" \
"    option multiple times to search in multiple registry directories.\n" \

static bool
registry_options(int ch)
{
    switch (ch) {
        case 'r':
            registry_paths_add(optarg);
            return true;

        default:
            return false;
    }
}


/*-----------------------------------------------------------------------
 * get command
 */

#define GET_SHORT_DESC \
    "Print out information about a Privateer plugin"

#define GET_USAGE_SUFFIX \
    "[options] <plugin name>..."

#define GET_HELP_TEXT \
"Loads in the YAML descriptor file for a particular plugin in a Privateer\n" \
"registry, then prints out information about that plugin.\n" \
"\n" \
"Options:\n" \
REGISTRY_HELP \

static int
get_options(int argc, char **argv);

static void
get_run(int argc, char **argv);

static struct cork_command  get =
    cork_leaf_command("get",
                      GET_SHORT_DESC,
                      GET_USAGE_SUFFIX,
                      GET_HELP_TEXT,
                      get_options, get_run);

#define GET_OPTS "+" REGISTRY_SHORTOPTS

static struct option  get_opts[] = {
    REGISTRY_LONGOPTS,
    { NULL, 0, NULL, 0 }
};

static int
get_options(int argc, char **argv)
{
    int  ch;
    registry_paths_init();
    while ((ch = getopt_long(argc, argv, GET_OPTS, get_opts, NULL)) != -1) {
        if (registry_options(ch)) {
            /* cool */
        } else {
            cork_command_show_help(&get, NULL);
            exit(EXIT_FAILURE);
        }
    }
    return optind;
}

static void
get_run(int argc, char **argv)
{
    size_t  i;
    struct pvt_registry  *reg;

    /* Open the input file. */
    if (argc == 0) {
        cork_command_show_help(&get, "Missing plugin name.");
        exit(EXIT_FAILURE);
    }

    reg = pvt_registry_new();
    registry_add_paths(reg);

    for (i = 0; i < argc; i++) {
        struct pvt_plugin_descriptor  *desc;

        if (i > 0) {
            printf("\n");
        }

        desc = pvt_registry_get_descriptor(reg, argv[i]);
        if (desc == NULL) {
            printf("%s\n", cork_error_message());
        } else {
            printf("Plugin:       %s\n"
                   "Library:      %s\n"
                   "Loader:       %s\n",
                   desc->name,
                   desc->library_path == NULL? "[default]": desc->library_path,
                   desc->loader_name);

            if (desc->dependencies != NULL) {
                size_t  j;
                for (j = 0; j < desc->dependency_count; j++) {
                    if (j == 0) {
                        printf("Dependencies: %s", desc->dependencies[j]);
                    } else {
                        printf(", %s", desc->dependencies[j]);
                    }
                }
                printf("\n");
            }
        }
    }

    pvt_registry_free(reg);
    registry_paths_done();
    exit(EXIT_SUCCESS);
}


/*-----------------------------------------------------------------------
 * load command
 */

#define LOAD_SHORT_DESC \
    "Load in Privateer plugins"

#define LOAD_USAGE_SUFFIX \
    "[options] [<plugin name>...]"

#define LOAD_HELP_TEXT \
"Loads in plugins from a Privateer registry.  If no plugin name is given,\n" \
"then all of the plugins in the registry are loaded.  Otherwise, only the\n" \
"specified plugins, and their dependencies, are loaded.\n" \
"\n" \
"Options:\n" \
REGISTRY_HELP \

static int
load_options(int argc, char **argv);

static void
load_run(int argc, char **argv);

static struct cork_command  load =
    cork_leaf_command("load",
                      LOAD_SHORT_DESC,
                      LOAD_USAGE_SUFFIX,
                      LOAD_HELP_TEXT,
                      load_options, load_run);

#define LOAD_OPTS "+" REGISTRY_SHORTOPTS

static struct option  load_opts[] = {
    REGISTRY_LONGOPTS,
    { NULL, 0, NULL, 0 }
};

static int
load_options(int argc, char **argv)
{
    int  ch;
    registry_paths_init();
    while ((ch = getopt_long(argc, argv, LOAD_OPTS, load_opts, NULL)) != -1) {
        if (registry_options(ch)) {
            /* cool */
        } else {
            cork_command_show_help(&load, NULL);
            exit(EXIT_FAILURE);
        }
    }
    return optind;
}

static void
load_run(int argc, char **argv)
{
    struct pvt_registry  *reg;

    reg = pvt_registry_new();
    registry_add_paths(reg);

    if (argc == 0) {
        ri_check_exit(pvt_registry_load_all(reg, NULL));
    } else {
        size_t  i;
        for (i = 0; i < argc; i++) {
            ri_check_exit(pvt_registry_load_one(reg, argv[i], NULL));
        }
    }

    pvt_registry_free(reg);
    registry_paths_done();
    exit(EXIT_SUCCESS);
}


/*-----------------------------------------------------------------------
 * Root command
 */

static struct cork_command  *root_subcommands[] = {
    &get, &load, NULL
};

static struct cork_command  root =
    cork_command_set("privateer", NULL, NULL, root_subcommands);

/*-----------------------------------------------------------------------
 * Driver
 */

int
main(int argc, char **argv)
{
    return cork_command_main(&root, argc, argv);
}

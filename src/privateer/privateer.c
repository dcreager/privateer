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
        int  rc = pvt_registry_add_directory(reg, path, ".yaml");
        if (CORK_UNLIKELY(rc != 0)) {
            fprintf(stderr, "%s\n", cork_error_message());
            exit(EXIT_FAILURE);
        }
    }
}


/*-----------------------------------------------------------------------
 * privateer command
 */

#define SHORT_DESC \
    "Print out the contents of a Privateer plugin"

#define USAGE_SUFFIX \
    "[options] <plugin name>..."

#define HELP_TEXT \
"Loads in the YAML descriptor file for a particular plugin in a Privateer\n" \
"registry, then prints out information about that plugin.\n" \
"\n" \
"Options:\n" \
"  --registry <registry path>\n" \
"    The location of a local Privateer registry.  You can provide this\n" \
"    option multiple times to search in multiple registry directories.\n" \

static int
privateer_options(int argc, char **argv);

static void
privateer(int argc, char **argv);

static struct cork_command  root =
    cork_leaf_command("privateer",
                      SHORT_DESC,
                      USAGE_SUFFIX,
                      HELP_TEXT,
                      privateer_options, privateer);

static struct option  run_opts[] = {
    { "registry", required_argument, NULL, 'r' },
    { NULL, 0, NULL, 0 }
};

static int
privateer_options(int argc, char **argv)
{
    int  ch;
    registry_paths_init();
    while ((ch = getopt_long(argc, argv, "+r:", run_opts, NULL)) != -1) {
        switch (ch) {
            case 'r':
                registry_paths_add(optarg);
                break;

            default:
                cork_command_show_help(&root, NULL);
                exit(EXIT_FAILURE);
        }
    }
    return optind;
}

static void
privateer(int argc, char **argv)
{
    size_t  i;
    struct pvt_registry  *reg;

    /* Open the input file. */
    if (argc == 0) {
        cork_command_show_help(&root, "Missing plugin name.");
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
                   "Descriptor:   %s\n"
                   "Library:      %s\n"
                   "Loader:       %s\n",
                   desc->name,
                   desc->descriptor_path,
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
 * Driver
 */

int
main(int argc, char **argv)
{
    return cork_command_main(&root, argc, argv);
}

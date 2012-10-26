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

static size_t
registry_path_count(void)
{
    return cork_array_size(&registry_paths);
}

static const char **
registry_paths_get(void)
{
    return &cork_array_at(&registry_paths, 0);
}


/*-----------------------------------------------------------------------
 * privateer command
 */

#define SHORT_DESC \
    "Print out the contents of a Privateer configuration file"

#define USAGE_SUFFIX \
    "[options] <entity name> <section name>"

#define HELP_TEXT \
"Loads in the YAML configuration file for a particular entity and section\n" \
"in a Privateer registry, then prints out a normalized version of that\n" \
"YAML.\n" \
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
    struct pvt_registry  *reg;
    yaml_document_t  doc;
    yaml_emitter_t  emit;

    /* Open the input file. */
    if (argc == 0) {
        cork_command_show_help(&root, "Missing entity and section names.");
        exit(EXIT_FAILURE);
    } else if (argc == 1) {
        cork_command_show_help(&root, "Missing section name.");
        exit(EXIT_FAILURE);
    } else if (argc > 2) {
        cork_command_show_help(&root, "Too many names provided.");
        exit(EXIT_FAILURE);
    }

    yaml_emitter_initialize(&emit);
    yaml_emitter_set_output_file(&emit, stdout);
    yaml_emitter_set_break(&emit, YAML_LN_BREAK);
    yaml_emitter_set_canonical(&emit, true);
    yaml_emitter_set_encoding(&emit, YAML_UTF8_ENCODING);
    yaml_emitter_set_unicode(&emit, false);

    reg = pvt_registry_new(registry_path_count(), registry_paths_get());
    if (pvt_registry_load_section(reg, argv[0], argv[1], &doc) != 0) {
        fprintf(stderr, "%s\n", cork_error_message());
        exit(EXIT_FAILURE);
    }

    if (yaml_emitter_dump(&emit, &doc) == 0) {
        fprintf(stderr, "%s\n", emit.problem);
        exit(EXIT_FAILURE);
    }

    yaml_document_delete(&doc);
    yaml_emitter_delete(&emit);
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

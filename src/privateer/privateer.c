/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <clogger.h>
#include <libcork/cli.h>
#include <libcork/core.h>
#include <libcork/ds.h>
#include <yaml.h>

#include "privateer.h"
#include "privateer-tests.h"

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

static const char **
registry_paths_get(void)
{
    return &cork_array_at(&registry_paths, 0);
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
 * list command
 */

#define LIST_SHORT_DESC \
    "List the plugins in a Privateer registry"

#define LIST_USAGE_SUFFIX \
    "[options]"

#define LIST_HELP_TEXT \
"Lists the names of all of the plugins in a Privateer registry.\n" \
"\n" \
"Options:\n" \
REGISTRY_HELP \

static int
list_options(int argc, char **argv);

static void
list_run(int argc, char **argv);

static struct cork_command  list =
    cork_leaf_command("list",
                      LIST_SHORT_DESC,
                      LIST_USAGE_SUFFIX,
                      LIST_HELP_TEXT,
                      list_options, list_run);

#define LIST_OPTS "+" REGISTRY_SHORTOPTS

static struct option  list_opts[] = {
    REGISTRY_LONGOPTS,
    { NULL, 0, NULL, 0 }
};

static int
list_options(int argc, char **argv)
{
    int  ch;
    registry_paths_init();
    while ((ch = getopt_long(argc, argv, LIST_OPTS, list_opts, NULL)) != -1) {
        if (registry_options(ch)) {
            /* cool */
        } else {
            cork_command_show_help(&list, NULL);
            exit(EXIT_FAILURE);
        }
    }
    return optind;
}

static void
list_run(int argc, char **argv)
{
    struct pvt_ctx  *ctx;
    size_t  i;

    /* Open the input file. */
    if (argc > 0) {
        cork_command_show_help(&list, "Too many arguments provided.");
        exit(EXIT_FAILURE);
    }

    ctx = pvt_ctx_new();
    ri_check_exit(pvt_ctx_find_plugins(ctx, registry_paths_get(), NULL));
    for (i = 0; i < pvt_ctx_plugin_count(ctx); i++) {
        const struct pvt_plugin  *plugin = pvt_ctx_plugin_at(ctx, i);
        printf("%s\n", pvt_plugin_name(plugin));
    }
    pvt_ctx_free(ctx);
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

#define LOAD_OPTS "+" REGISTRY_SHORTOPTS "d"

static bool  dump_plugins = false;

static struct option  load_opts[] = {
    REGISTRY_LONGOPTS,
    { "dump", no_argument, NULL, 'd' },
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
        } else if (ch == 'd') {
            dump_plugins = true;
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
    struct pvt_ctx  *ctx;

    ctx = pvt_ctx_new();

    if (argc == 0) {
        ri_check_exit(pvt_ctx_find_plugins
                      (ctx, registry_paths_get(), NULL));
    } else {
        ri_check_exit(pvt_ctx_find_plugins
                      (ctx, registry_paths_get(), (const char **) argv));
    }

    ri_check_exit(pvt_ctx_load_plugins(ctx));

    if (dump_plugins) {
        size_t  i;

        printf("Plugins\n");
        for (i = 0; i < pvt_ctx_plugin_count(ctx); i++) {
            const struct pvt_plugin  *plugin = pvt_ctx_plugin_at(ctx, i);
            printf("  %s (%s)\n",
                   pvt_plugin_name(plugin),
                   pvt_plugin_library_name(plugin));
        }

        for (i = 0; i < pvt_ctx_extension_point_count(ctx); i++) {
            size_t  j;
            const struct pvt_extension_point  *exts =
                pvt_ctx_extension_point_at(ctx, i);
            printf("\n%s\n", pvt_extension_point_name(exts));
            for (j = 0; j < pvt_extension_point_count(exts); j++) {
                const struct pvt_extension  *ext =
                    pvt_extension_point_at(exts, j);
                printf("  %s\n", pvt_extension_name(ext));
            }
        }
    }

    pvt_ctx_free(ctx);
    registry_paths_done();
    exit(EXIT_SUCCESS);
}


/*-----------------------------------------------------------------------
 * Root command
 */

static struct cork_command  *root_subcommands[] = {
    &list, &load, NULL
};

static struct cork_command  root =
    cork_command_set("privateer", NULL, NULL, root_subcommands);


/*-----------------------------------------------------------------------
 * Driver
 */

/* This is a hack to force the linker to link this program with
 * libprivateer-tests.  (For some of our test cases, we're going to try to load
 * symbols from that library at runtime, via dlopen and dlsym, and we don't want
 * the linker to outthink us.) */
static const struct pvt_plugin_registration  *dummy;

int
main(int argc, char **argv)
{
    clog_set_default_format("[%L] %m");
    ri_check_exit(clog_setup_logging());
    dummy = pvt_static_plugin(pvt_alpha);
    return cork_command_main(&root, argc, argv);
}

/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <dlfcn.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#include <clogger.h>
#include <libcork/core.h>
#include <libcork/ds.h>
#include <libcork/os.h>
#include <libcork/helpers/errors.h>
#include <yaml.h>

#include "privateer.h"


struct pvt_extension {
    const char  *name;
    struct pvt_plugin  *plugin;
    struct pvt_extension_point  *extension_point;
    void  *user_data;
    cork_free_f  free_user_data;
};

struct pvt_extension_point {
    const char  *name;
    struct cork_hash_table  *by_name;
    cork_array(struct pvt_extension *)  by_index;
    struct pvt_ctx  *ctx;
};

struct pvt_plugin {
    struct pvt_ctx  *ctx;
    const char  *name;
    const char  *library_name;
    const char  *raw_library_name;
    void  *library;
    const struct pvt_plugin_registration  *reg;
    cork_array(struct pvt_extension *)  extensions;
    bool  loading;
    bool  loaded;
};

struct pvt_ctx {
    struct pvt_extension_point  *root_extension_points;
    struct pvt_extension_point  *plugins;
    cork_array(struct pvt_extension_point *)  all_extension_points;
};


#define CLOG_CHANNEL  "extensions"

/*-----------------------------------------------------------------------
 * Extensions
 */

static struct pvt_extension *
pvt_extension_new(struct pvt_extension_point *extension_point,
                  struct pvt_plugin *plugin, const char *name,
                  void *user_data, cork_free_f free_user_data)
{
    struct pvt_extension  *ext = cork_new(struct pvt_extension);
    ext->name = cork_strdup(name);
    ext->plugin = plugin;
    ext->extension_point = extension_point;
    ext->user_data = user_data;
    ext->free_user_data = free_user_data;
    if (plugin != NULL) {
        cork_array_append(&plugin->extensions, ext);
    }
    cork_array_append(&extension_point->by_index, ext);
    return ext;
}

static void
pvt_extension_free(struct pvt_extension *ext)
{
    cork_free_user_data(ext);
    cork_strfree(ext->name);
    free(ext);
}

const char *
pvt_extension_name(const struct pvt_extension *ext)
{
    return ext->name;
}

struct pvt_extension_point *
pvt_extension_extension_point(const struct pvt_extension *ext)
{
    return ext->extension_point;
}

void *
pvt_extension_user_data(const struct pvt_extension *ext)
{
    return ext->user_data;
}


/*-----------------------------------------------------------------------
 * Extension points
 */

struct pvt_extension_point *
pvt_extension_point_new(struct pvt_ctx *ctx, const char *name)
{
    struct pvt_extension_point  *exts = cork_new(struct pvt_extension_point);
    clog_debug("Create new extension point: %s", name);
    exts->name = cork_strdup(name);
    exts->by_name = cork_string_hash_table_new(0);
    cork_array_init(&exts->by_index);
    exts->ctx = ctx;
    cork_array_append(&ctx->all_extension_points, exts);
    return exts;
}

void
pvt_extension_point_free(struct pvt_extension_point *exts)
{
    size_t  i;
    for (i = 0; i < cork_array_size(&exts->by_index); i++) {
        struct pvt_extension  *ext = cork_array_at(&exts->by_index, i);
        pvt_extension_free(ext);
    }
    cork_array_done(&exts->by_index);
    cork_hash_table_free(exts->by_name);
    cork_strfree(exts->name);
    free(exts);
}

const char *
pvt_extension_point_name(const struct pvt_extension_point *exts)
{
    return exts->name;
}

struct pvt_ctx *
pvt_extension_point_context(const struct pvt_extension_point *exts)
{
    return exts->ctx;
}

struct pvt_extension *
pvt_extension_point_add(struct pvt_extension_point *exts,
                        struct pvt_plugin *plugin, const char *name,
                        void *user_data, cork_free_f free_user_data)
{
    bool  is_new;
    struct cork_hash_table_entry  *entry;
    entry = cork_hash_table_get_or_create
        (exts->by_name, (void *) name, &is_new);
    if (CORK_LIKELY(is_new)) {
        struct pvt_extension  *ext;
        clog_debug("Register %s \"%s\"", exts->name, name);
        ext = pvt_extension_new(exts, plugin, name, user_data, free_user_data);
        entry->key = (void *) ext->name;
        entry->value = ext;
        return ext;
    } else {
        pvt_redefined("Already have a %s named \"%s\"", exts->name, name);
        return NULL;
    }
}

void *
pvt_extension_point_get(const struct pvt_extension_point *exts,
                        const char *name)
{
    struct pvt_extension  *ext =
        cork_hash_table_get(exts->by_name, (void *) name);
    if (CORK_UNLIKELY(ext == NULL)) {
        return NULL;
    } else {
        return ext->user_data;
    }
}

void *
pvt_extension_point_require(const struct pvt_extension_point *exts,
                            const char *name)
{
    struct pvt_extension  *ext =
        cork_hash_table_get(exts->by_name, (void *) name);
    if (CORK_UNLIKELY(ext == NULL)) {
        pvt_undefined("No %s named \"%s\"", exts->name, name);
        return NULL;
    } else {
        return ext->user_data;
    }
}

size_t
pvt_extension_point_count(const struct pvt_extension_point *exts)
{
    return cork_array_size(&exts->by_index);
}

const struct pvt_extension *
pvt_extension_point_at(const struct pvt_extension_point *exts, size_t index)
{
    return cork_array_at(&exts->by_index, index);
}


#undef CLOG_CHANNEL
#define CLOG_CHANNEL  "plugins"

/*-----------------------------------------------------------------------
 * Plugins
 */

static struct pvt_plugin *
pvt_plugin_new_static(struct pvt_ctx *ctx,
                      const struct pvt_plugin_registration *reg)
{
    struct pvt_plugin  *plugin = cork_new(struct pvt_plugin);
    plugin->ctx = ctx;
    plugin->name = cork_strdup(reg->plugin_name);
    cork_array_init(&plugin->extensions);
    plugin->raw_library_name = NULL;
    plugin->library_name = "[default]";
    plugin->library = NULL;
    plugin->reg = reg;
    plugin->loading = false;
    plugin->loaded = false;
    return plugin;
}

static struct pvt_plugin *
pvt_plugin_new_dynamic(struct pvt_ctx *ctx,
                       const char *name, const char *library_name)
{
    struct pvt_plugin  *plugin = cork_new(struct pvt_plugin);
    plugin->ctx = ctx;
    plugin->name = cork_strdup(name);
    if (library_name == NULL) {
        plugin->raw_library_name = NULL;
        plugin->library_name = "[default]";
    } else {
        plugin->raw_library_name = cork_strdup(library_name);
        plugin->library_name = plugin->raw_library_name;
    }
    cork_array_init(&plugin->extensions);
    plugin->library = NULL;
    plugin->reg = NULL;
    plugin->loading = false;
    plugin->loaded = false;
    return plugin;
}

static void
pvt_plugin_free(struct pvt_plugin *plugin)
{
    if (plugin->library != NULL) {
        dlclose(plugin->library);
    }
    cork_strfree(plugin->name);
    if (plugin->raw_library_name != NULL) {
        cork_strfree(plugin->raw_library_name);
    }
    cork_array_done(&plugin->extensions);
    free(plugin);
}

const char *
pvt_plugin_name(const struct pvt_plugin *plugin)
{
    return plugin->name;
}

const char *
pvt_plugin_library_name(const struct pvt_plugin *plugin)
{
    return plugin->library_name;
}

bool
pvt_plugin_loaded(const struct pvt_plugin *plugin)
{
    return plugin->loaded;
}

struct pvt_ctx *
pvt_plugin_context(const struct pvt_plugin *plugin)
{
    return plugin->ctx;
}

size_t
pvt_plugin_extension_count(const struct pvt_plugin *plugin)
{
    return cork_array_size(&plugin->extensions);
}

const struct pvt_extension *
pvt_plugin_extension_at(const struct pvt_plugin *plugin, size_t index)
{
    return cork_array_at(&plugin->extensions, index);
}

static int
pvt_plugin_load_library(struct pvt_plugin *plugin)
{
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    const char  *library_name = plugin->raw_library_name;
    const char  *lib_error;

    if (CORK_UNLIKELY(plugin->reg != NULL)) {
        return 0;
    }

    /* If the library name doesn't contain a slash, assume that it's a "short"
     * library name.  Turn it into a full library filename (still not including
     * a path, though), by adding this platform's prefix and suffix. */
    if (library_name != NULL && strchr(library_name, '/') == NULL) {
        cork_buffer_printf
            (&buf, PVT_LIBRARY_PREFIX "%s" PVT_LIBRARY_SUFFIX, library_name);
        library_name = buf.buf;
    }

    /* Load in the library that contains this registration */
    clog_debug("Load library %s for plugin %s",
               library_name == NULL? "[default]": library_name,
               plugin->name);
    plugin->library = dlopen(library_name, RTLD_LAZY | RTLD_LOCAL);
    if (plugin->library == NULL) {
        lib_error = dlerror();
        pvt_bad_library
            ("Cannot open library %s: %s", plugin->library_name, lib_error);
        cork_buffer_done(&buf);
        return -1;
    }

    /* Find the registration struct for this plugin */
    clog_debug("Load registration for plugin %s", plugin->name);
    cork_buffer_printf(&buf, "%s__plugin", plugin->name);
    (void) dlerror();
    plugin->reg = dlsym(plugin->library, buf.buf);
    lib_error = dlerror();
    cork_buffer_done(&buf);
    if (CORK_UNLIKELY(lib_error != NULL)) {
        pvt_bad_library
            ("Cannot find registration for plugin %s in library %s: %s",
             plugin->name, plugin->library_name, lib_error);
        return -1;
    }

    /* Verify that the symbol points to a valid registration struct. */
    if (CORK_UNLIKELY(plugin->reg->magic != PVT_PLUGIN_REGISTRATION_MAGIC)) {
        pvt_bad_library
            ("%s does not contain a valid plugin registration for plugin %s",
             plugin->library_name, plugin->name);
        return -1;
    }

    return 0;
}

int
pvt_plugin_load(struct pvt_plugin *plugin)
{
    if (CORK_UNLIKELY(plugin->reg == NULL)) {
        rii_check(pvt_plugin_load_library(plugin));
    }

    if (CORK_UNLIKELY(plugin->loaded)) {
        /* Plugin is already loaded */
        return 0;
    }

    if (CORK_UNLIKELY(plugin->loading)) {
        /* Circular dependency! */
        pvt_circular_dependency
            ("Circular dependency when loading plugin %s", plugin->name);
        return -1;
    }

    /* If we ever have to change the plugin registration struct, we'll bump the
     * current version number so that we can tell apart registrations from
     * libraries compiled with previous versions.  This switch statement should
     * be able to handle all of them that we currently know about. */

    switch (plugin->reg->version) {
        case 1:
            clog_debug("Load plugin %s", plugin->name);
            plugin->loading = true;
            plugin->reg->reg(plugin->ctx, plugin);
            if (CORK_UNLIKELY(cork_error_occurred())) {
                return -1;
            } else {
                plugin->loaded = true;
                return 0;
            }

        default:
            pvt_bad_library
                ("Cannot handle plugin version %u for %s",
                 plugin->reg->version, plugin->name);
            return -1;
    }
}


/*-----------------------------------------------------------------------
 * Context
 */

struct pvt_ctx *
pvt_ctx_new(void)
{
    struct pvt_ctx  *ctx = cork_new(struct pvt_ctx);
    cork_array_init(&ctx->all_extension_points);
    ctx->root_extension_points =
        pvt_extension_point_new(ctx, "extension point");
    ctx->plugins = pvt_ctx_root_extension_point(ctx, "plugin");
    return ctx;
}

void
pvt_ctx_free(struct pvt_ctx *ctx)
{
    pvt_extension_point_free(ctx->root_extension_points);
    cork_array_done(&ctx->all_extension_points);
    free(ctx);
}

struct pvt_plugin *
pvt_ctx_add_static_plugin(struct pvt_ctx *ctx,
                          const struct pvt_plugin_registration *reg)
{
    struct pvt_plugin  *plugin;
    plugin = pvt_extension_point_get(ctx->plugins, reg->plugin_name);
    if (CORK_LIKELY(plugin == NULL)) {
        plugin = pvt_plugin_new_static(ctx, reg);
        clog_debug("Add static plugin %s", reg->plugin_name);
        pvt_extension_point_add
            (ctx->plugins, plugin, reg->plugin_name,
             plugin, (cork_free_f) pvt_plugin_free);
        return plugin;
    } else {
        if (plugin->reg == NULL) {
            /* It's okay to register a static plugin with the same name as a
             * dynamic plugin, if that dynamic plugin hasn't been loaded yet. */
            clog_debug("Static plugin %s shadows dynamic plugin from %s",
                       reg->plugin_name, plugin->library_name);
            plugin->reg = reg;
            return plugin;
        } else if (plugin->reg == reg) {
            /* It's okay to register the exact same static plugin multiple
             * times. */
            clog_debug("Static plugin %s already added", reg->plugin_name);
            return plugin;
        } else {
            /* But if you try to register two distinct static plugins with the
             * same name, that's an error. */
            pvt_redefined
                ("Plugin %s already exists in library %s",
                 reg->plugin_name, plugin->library_name);
            return NULL;
        }
    }
}

struct pvt_plugin *
pvt_ctx_add_dynamic_plugin(struct pvt_ctx *ctx, const char *plugin_name,
                           const char *library_name)
{
    struct pvt_plugin  *plugin;
    plugin = pvt_extension_point_get(ctx->plugins, plugin_name);
    if (CORK_LIKELY(plugin == NULL)) {
        plugin = pvt_plugin_new_dynamic(ctx, plugin_name, library_name);
        clog_debug
            ("Add dynamic plugin %s from %s",
             plugin_name, library_name == NULL? "[default]": library_name);
        pvt_extension_point_add
            (ctx->plugins, plugin, plugin_name,
             plugin, (cork_free_f) pvt_plugin_free);
        return plugin;
    } else {
        pvt_redefined
            ("Plugin %s already exists in library %s",
             plugin_name, plugin->library_name);
        return NULL;
    }
}

int
pvt_ctx_load_plugins(struct pvt_ctx *ctx)
{
    size_t  i;
    size_t  count;
    count = pvt_extension_point_count(ctx->plugins);
    for (i = 0; i < count; i++) {
        const struct pvt_extension  *ext =
            pvt_extension_point_at(ctx->plugins, i);
        struct pvt_plugin  *plugin = ext->user_data;
        rii_check(pvt_plugin_load(plugin));
    }
    return 0;
}

int
pvt_ctx_require_plugin(struct pvt_ctx *ctx, const char *plugin_name)
{
    struct pvt_plugin  *plugin;
    rip_check(plugin = pvt_extension_point_require(ctx->plugins, plugin_name));
    return pvt_plugin_load(plugin);
}


const struct pvt_plugin *
pvt_ctx_get_plugin(const struct pvt_ctx *ctx, const char *plugin_name)
{
    return pvt_extension_point_require(ctx->plugins, plugin_name);
}

size_t
pvt_ctx_plugin_count(const struct pvt_ctx *ctx)
{
    return pvt_extension_point_count(ctx->plugins);
}

const struct pvt_plugin *
pvt_ctx_plugin_at(const struct pvt_ctx *ctx, size_t index)
{
    const struct pvt_extension  *ext =
        pvt_extension_point_at(ctx->plugins, index);
    return ext->user_data;
}


struct pvt_extension_point *
pvt_ctx_root_extension_point(struct pvt_ctx *ctx, const char *name)
{
    struct pvt_extension_point  *exts =
        pvt_extension_point_get(ctx->root_extension_points, name);
    if (CORK_UNLIKELY(exts == NULL)) {
        exts = pvt_extension_point_new(ctx, name);
        rpp_check(pvt_extension_point_add
                  (ctx->root_extension_points, NULL, name,
                   exts, (cork_free_f) pvt_extension_point_free));
    }
    return exts;
}

size_t
pvt_ctx_extension_point_count(const struct pvt_ctx *ctx)
{
    return cork_array_size(&ctx->all_extension_points);
}

const struct pvt_extension_point *
pvt_ctx_extension_point_at(const struct pvt_ctx *ctx, size_t index)
{
    return cork_array_at(&ctx->all_extension_points, index);
}


/*-----------------------------------------------------------------------
 * YAML helpers
 */

static int
pvt_yaml_read_file(void *vdata, unsigned char *buf, size_t max_size,
                   size_t *actual_size)
{
    int  fd = (intptr_t) vdata;
    ssize_t  bytes_read = read(fd, buf, max_size);
    if (bytes_read == -1) {
        cork_system_error_set();
        return 0;
    } else {
        *actual_size = bytes_read;
        return 1;
    }
}

static int
pvt_load_yaml_file(const char *filename, yaml_document_t *dest)
{
    int  fd;
    int  rc;
    yaml_parser_t  parser;

    fd = open(filename, O_RDONLY);
    if (fd == -1) {
        cork_system_error_set();
        return -1;
    }

    if (CORK_UNLIKELY(yaml_parser_initialize(&parser) == 0)) {
        pvt_bad_config("Cannot allocate YAML parser");
        close(fd);
        return -1;
    }

    yaml_parser_set_input(&parser, pvt_yaml_read_file, (void *) (intptr_t) fd);
    rc = yaml_parser_load(&parser, dest);
    close(fd);

    if (CORK_UNLIKELY(rc == 0)) {
        pvt_bad_config("%s", parser.problem);
        yaml_parser_delete(&parser);
        return -1;
    }

    yaml_parser_delete(&parser);
    return 0;
}


/*-----------------------------------------------------------------------
 * Finding plugins
 */

static int
pvt_load_plugin_conf_file(struct pvt_ctx *ctx, const char *path,
                          const char **plugins)
{
    yaml_document_t  doc;
    yaml_node_t  *node;
    yaml_node_pair_t  *pair;

    clog_debug("Load plugin list from %s", path);
    rii_check(pvt_load_yaml_file(path, &doc));

    node = yaml_document_get_node(&doc, 1);
    if (CORK_UNLIKELY(node->type != YAML_MAPPING_NODE)) {
        pvt_bad_config("Expected a mapping value for plugin list");
        goto error;
    }

    for (pair = node->data.mapping.pairs.start;
         pair < node->data.mapping.pairs.top; pair++) {
        yaml_node_t  *plugin_node = yaml_document_get_node(&doc, pair->key);
        yaml_node_t  *library_node = yaml_document_get_node(&doc, pair->value);
        const char  *plugin_name;
        const char  *library_name;

        if (CORK_UNLIKELY(plugin_node->type != YAML_SCALAR_NODE)) {
            pvt_bad_config("Expected a scalar value for plugin name");
            goto error;
        }

        if (CORK_UNLIKELY(library_node->type != YAML_SCALAR_NODE)) {
            pvt_bad_config("Expected a scalar value for library name");
            goto error;
        }

        plugin_name = (const char *) plugin_node->data.scalar.value;
        library_name = (const char *) library_node->data.scalar.value;

        /* Treat the empty string or "~" specially for the library name */
        if (library_name[0] == '\0' ||
            (library_name[0] == '~' && library_name[1] == '\0')) {
            library_name = NULL;
        }

        if (plugins == NULL) {
            ep_check(pvt_ctx_add_dynamic_plugin
                     (ctx, plugin_name, library_name));
        } else {
            const char  **curr;
            for (curr = plugins; *curr != NULL; curr++) {
                if (strcmp(*curr, plugin_name) == 0) {
                    ep_check(pvt_ctx_add_dynamic_plugin
                             (ctx, plugin_name, library_name));
                    break;
                }
            }
            if (*curr == NULL) {
                clog_debug("Skip plugin %s from library %s",
                           plugin_name, library_name);
            }
        }
    }

    yaml_document_delete(&doc);
    return 0;

error:
    yaml_document_delete(&doc);
    return -1;
}


struct pvt_walker {
    struct pvt_ctx  *ctx;
    struct cork_dir_walker  parent;
    size_t  extension_length;
    const char  *extension;
    const char  **plugins;
};

static int
pvt_plugin_walk__file(struct cork_dir_walker *vwalker, const char *full_path,
                      const char *rel_path, const char *base_name)
{
    struct pvt_walker  *walker =
        cork_container_of(vwalker, struct pvt_walker, parent);

    /* If the user specified an extension, make sure the filename ends with that
     * extension. */
    if (walker->extension != NULL) {
        size_t  base_name_length = strlen(base_name);
        if (walker->extension_length > base_name_length) {
            /* Base name can't possibly end with the desired extension, since
             * it's too short. */
            clog_debug("Skip non-plugin file %s", full_path);
            return 0;
        }

        if (memcmp(base_name + base_name_length - walker->extension_length,
                   walker->extension, walker->extension_length) != 0) {
            /* Base name doesn't end with desired extension. */
            clog_debug("Skip non-plugin file %s", full_path);
            return 0;
        }
    }

    /* Then load the file. */
    return pvt_load_plugin_conf_file
        (walker->ctx, full_path, walker->plugins);
}

static int
pvt_plugin_walk__enter(struct cork_dir_walker *vwalker, const char *full_path,
                       const char *rel_path, const char *base_name)
{
    return 0;
}

static int
pvt_plugin_walk__leave(struct cork_dir_walker *vwalker, const char *full_path,
                       const char *rel_path, const char *base_name)
{
    return 0;
}

static int
pvt_plugin_load_conf_directory(struct pvt_ctx *ctx, const char *path,
                               const char **plugins)
{
    struct pvt_walker  walker;
    clog_debug("Look for plugins in %s", path);
    walker.parent.file = pvt_plugin_walk__file;
    walker.parent.enter_directory = pvt_plugin_walk__enter;
    walker.parent.leave_directory = pvt_plugin_walk__leave;
    walker.ctx = ctx;
    walker.extension_length = 5;
    walker.extension = ".yaml";
    walker.plugins = plugins;
    return cork_walk_directory(path, &walker.parent);
}


int
pvt_ctx_find_plugins(struct pvt_ctx *ctx, const char **registry_paths,
                     const char **plugins)
{
    const char  **path;

    if (registry_paths == NULL || *registry_paths == NULL) {
        pvt_bad_config("No registry paths given");
        return -1;
    }

    for (path = registry_paths; *path != NULL; path++) {
        rii_check(pvt_plugin_load_conf_directory(ctx, *path, plugins));
    }

    return 0;
}

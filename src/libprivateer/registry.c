/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
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

#include "privateer/config.h"
#include "privateer/error.h"
#include "privateer/registry.h"

#define CLOG_CHANNEL  "pvt-registry"


/*-----------------------------------------------------------------------
 * YAML helpers
 */

static int
pvt_registry_read(void *vdata, unsigned char *buf, size_t max_size,
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
pvt_registry_load_yaml_file(const char *filename, yaml_document_t *dest)
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
        pvt_yaml_error("Cannot allocate YAML parser");
        close(fd);
        return -1;
    }

    yaml_parser_set_input(&parser, pvt_registry_read, (void *) (intptr_t) fd);
    rc = yaml_parser_load(&parser, dest);
    close(fd);

    if (CORK_UNLIKELY(rc == 0)) {
        pvt_yaml_error("%s", parser.problem);
        yaml_parser_delete(&parser);
        return -1;
    }

    yaml_parser_delete(&parser);
    return 0;
}


/*-----------------------------------------------------------------------
 * Plugin plugins
 */

static struct pvt_plugin_descriptor *
pvt_plugin_descriptor_new(const char *descriptor_path, const char *name,
                          const char *library_path, const char *loader_name)
{
    struct pvt_plugin_descriptor  *desc =
        cork_new(struct pvt_plugin_descriptor);
    clog_debug("Read plugin descriptor %s from %s", name, descriptor_path);
    desc->descriptor_path = cork_strdup(descriptor_path);
    desc->name = cork_strdup(name);
    if (library_path == NULL) {
        desc->library_path = NULL;
    } else {
        desc->library_path = cork_strdup(library_path);
    }
    desc->loader_name = cork_strdup(loader_name);
    desc->dependency_count = 0;
    desc->dependencies = NULL;
    return desc;
}

static struct pvt_plugin_descriptor *
pvt_plugin_descriptor_from_yaml(const char *path, yaml_document_t *doc)
{
    size_t  i;
    struct pvt_plugin_descriptor  *desc;
    yaml_node_t  *root;
    const char  *name = NULL;
    const char  *library = NULL;
    const char  *loader = NULL;
    yaml_node_t  *dependencies = NULL;
    yaml_node_pair_t  *pair;

#define verify_type(node, expected, msg) \
    do { \
        if (CORK_UNLIKELY((node)->type != (expected))) { \
            pvt_yaml_error("Error in %s: " msg, path); \
            return NULL; \
        } \
    } while (0)

#define verify_exists(name) \
    do { \
        if (CORK_UNLIKELY((name) == NULL)) { \
            pvt_yaml_error \
                ("Error in %s: Missing " #name " in plugin descriptor", path); \
            return NULL; \
        } \
    } while (0)

    root = yaml_document_get_root_node(doc);
    verify_type(root, YAML_MAPPING_NODE,
                "Plugin descriptor must be a map");

    /* Pull out the named fields that we care about */
    for (pair = root->data.mapping.pairs.start;
         pair < root->data.mapping.pairs.top; pair++) {
        yaml_node_t  *key = yaml_document_get_node(doc, pair->key);
        yaml_node_t  *value;
        const char  *key_name;
        verify_type(key, YAML_SCALAR_NODE, "Map key must be a scalar");
        key_name = (const char *) key->data.scalar.value;

        if (strcmp(key_name, "name") == 0) {
            value = yaml_document_get_node(doc, pair->value);
            verify_type(value, YAML_SCALAR_NODE,
                        "Plugin's name must be a scalar");
            name = (const char *) value->data.scalar.value;
        } else if (strcmp(key_name, "library") == 0) {
            value = yaml_document_get_node(doc, pair->value);
            verify_type(value, YAML_SCALAR_NODE,
                        "Plugin's library path must be a scalar");
            library = (const char *) value->data.scalar.value;
        } else if (strcmp(key_name, "loader") == 0) {
            value = yaml_document_get_node(doc, pair->value);
            verify_type(value, YAML_SCALAR_NODE,
                        "Plugin's loader function name must be a scalar");
            loader = (const char *) value->data.scalar.value;
        } else if (strcmp(key_name, "dependencies") == 0) {
            dependencies = yaml_document_get_node(doc, pair->value);
            verify_type(dependencies, YAML_SEQUENCE_NODE,
                        "Plugin's dependency list must be a sequence");
        }
    }

    /* Verify that the required fields exist */
    verify_exists(name);
    verify_exists(loader);

    /* If there's a dependencies field, make sure it's an array of strings */
    if (dependencies != NULL) {
        yaml_node_item_t  *item;
        for (item = dependencies->data.sequence.items.start;
             item < dependencies->data.sequence.items.top; item++) {
            yaml_node_t  *element = yaml_document_get_node(doc, *item);
            verify_type(element, YAML_SCALAR_NODE,
                        "Plugin dependency must be a scalar");
        }
    }

    /* Okay, we've verified everything.  Now construct that bad boy. */
    desc = pvt_plugin_descriptor_new(path, name, library, loader);

    if (dependencies != NULL) {
        desc->dependency_count =
            dependencies->data.sequence.items.top -
            dependencies->data.sequence.items.start;
        desc->dependencies =
            cork_calloc(desc->dependency_count, sizeof(const char *));
        for (i = 0; i < desc->dependency_count; i++) {
            yaml_node_item_t  *item;
            yaml_node_t  *element;
            const char  *dependency;
            item = dependencies->data.sequence.items.start + i;
            element = yaml_document_get_node(doc, *item);
            dependency = (const char *) element->data.scalar.value;
            desc->dependencies[i] = cork_strdup(dependency);
        }
    }

    return desc;
}

static void
pvt_plugin_descriptor_free(struct pvt_plugin_descriptor *desc)
{
    cork_strfree(desc->descriptor_path);
    cork_strfree(desc->name);
    if (desc->library_path != NULL) {
        cork_strfree(desc->library_path);
    }
    cork_strfree(desc->loader_name);
    if (desc->dependencies != NULL) {
        size_t  i;
        for (i = 0; i < desc->dependency_count; i++) {
            cork_strfree(desc->dependencies[i]);
        }
        free(desc->dependencies);
    }
    free(desc);
}


/*-----------------------------------------------------------------------
 * Plugins
 */

struct pvt_plugin {
    struct pvt_plugin_descriptor  *desc;
    void  *library;
    struct pvt_loader  *loader;
    bool  loaded;
};

static struct pvt_plugin *
pvt_plugin_new(struct pvt_plugin_descriptor *desc)
{
    struct pvt_plugin  *plugin;
    struct cork_buffer  buf = CORK_BUFFER_INIT();
    const char  *raw_library_name = desc->library_path;
    const char  *library_name = raw_library_name;
    const char  *lib_error;
    void  *library;
    void  *loader;

    clog_debug("Loading %s for plugin %s", raw_library_name, desc->name);

    /* If the library name doesn't contain a slash, assume that it's a "short"
     * library name.  Turn it into a full library filename (still not including
     * a path, though), by adding this platform's prefix and suffix. */
    if (library_name != NULL && strchr(library_name, '/') == NULL) {
        cork_buffer_printf
            (&buf, "%s%s%s",
             PVT_LIBRARY_PREFIX, library_name, PVT_LIBRARY_SUFFIX);
        library_name = buf.buf;
    }

    /* Load in the library that contains this registration */
    library = dlopen(library_name, RTLD_LAZY | RTLD_LOCAL);
    if (library == NULL) {
        lib_error = dlerror();
        if (raw_library_name == NULL) {
            raw_library_name = "[default]";
        }
        pvt_bad_library("Cannot open library %s for plugin %s\n%s",
                        raw_library_name, desc->name, lib_error);
        pvt_plugin_descriptor_free(desc);
        cork_buffer_done(&buf);
        return NULL;
    }

    /* Extract the registration struct */
    loader = dlsym(library, desc->loader_name);
    lib_error = dlerror();
    if (lib_error != NULL) {
        if (raw_library_name == NULL) {
            raw_library_name = "[default]";
        }
        pvt_bad_library("Cannot load symbol %s from %s for plugin %s\n%s",
                        raw_library_name, desc->loader_name,
                        desc->name, lib_error);
        dlclose(library);
        pvt_plugin_descriptor_free(desc);
        cork_buffer_done(&buf);
        return NULL;
    } else if (loader == NULL) {
        pvt_bad_library("No symbol named %s in %s for plugin %s",
                        raw_library_name, desc->loader_name, desc->name);
        dlclose(library);
        pvt_plugin_descriptor_free(desc);
        cork_buffer_done(&buf);
        return NULL;
    }

    plugin = cork_new(struct pvt_plugin);
    plugin->desc = desc;
    plugin->library = library;
    plugin->loader = loader;
    plugin->loaded = false;
    cork_buffer_done(&buf);
    return plugin;
}

static void
pvt_plugin_free(struct pvt_plugin *plugin)
{
    pvt_plugin_descriptor_free(plugin->desc);
    if (plugin->library != NULL) {
        dlclose(plugin->library);
    }
    free(plugin);
}


/*-----------------------------------------------------------------------
 * Registry maintenance
 */

struct pvt_registry {
    struct cork_hash_table  plugins;
};

static cork_hash
string_hasher(const void *vk)
{
    const char  *k = vk;
    size_t  len = strlen(k);
    return cork_hash_buffer(0, k, len);
}

static bool
string_comparator(const void *vk1, const void *vk2)
{
    const char  *k1 = vk1;
    const char  *k2 = vk2;
    return strcmp(k1, k2) == 0;
}


struct pvt_registry *
pvt_registry_new(void)
{
    struct pvt_registry  *self = cork_new(struct pvt_registry);
    cork_hash_table_init(&self->plugins, 0, string_hasher, string_comparator);
    return self;
}

void
pvt_registry_free(struct pvt_registry *self)
{
    struct cork_hash_table_iterator  iter;
    struct cork_hash_table_entry  *entry;
    cork_hash_table_iterator_init(&self->plugins, &iter);
    while ((entry = cork_hash_table_iterator_next(&iter)) != NULL) {
        pvt_plugin_free(entry->value);
    }
    cork_hash_table_done(&self->plugins);
    free(self);
}


struct pvt_plugin_descriptor *
pvt_registry_get_descriptor(struct pvt_registry *self, const char *name)
{
    struct pvt_plugin  *plugin =
        cork_hash_table_get(&self->plugins, (void *) name);
    if (plugin == NULL) {
        pvt_undefined("No plugin named \"%s\"", name);
        return NULL;
    } else {
        return plugin->desc;
    }
}


static struct pvt_plugin_descriptor *
pvt_registry_load_plugin(struct pvt_registry *self, const char *path)
{
    struct pvt_plugin_descriptor  *desc;
    yaml_document_t  doc;
    clog_debug("Loading plugin from %s", path);
    rpi_check(pvt_registry_load_yaml_file(path, &doc));
    desc = pvt_plugin_descriptor_from_yaml(path, &doc);
    yaml_document_delete(&doc);
    return desc;
}

int
pvt_registry_add_plugin(struct pvt_registry *self,
                        struct pvt_plugin_descriptor *desc)
{
    bool  is_new;
    struct cork_hash_table_entry  *entry =
        cork_hash_table_get_or_create
        (&self->plugins, (void *) desc->name, &is_new);

    if (is_new) {
        rip_check(entry->value = pvt_plugin_new(desc));
        return 0;
    } else {
        struct pvt_plugin_descriptor  *old = entry->value;
        pvt_redefined
            ("Plugin %s defined in %s and %s",
             desc->name, old->descriptor_path, desc->descriptor_path);
        pvt_plugin_descriptor_free(desc);
        return -1;
    }

}

int
pvt_registry_add_file(struct pvt_registry *self, const char *path)
{
    struct pvt_plugin_descriptor  *desc;
    rip_check(desc = pvt_registry_load_plugin(self, path));
    return pvt_registry_add_plugin(self, desc);
}

struct pvt_walker {
    struct cork_dir_walker  parent;
    struct pvt_registry  *reg;
    size_t  extension_length;
    const char  *extension;
};

static int
pvt_registry_walk__file(struct cork_dir_walker *vwalker, const char *full_path,
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
            clog_debug("Skipping non-plugin file %s", full_path);
            return 0;
        }

        if (memcmp(base_name + base_name_length - walker->extension_length,
                   walker->extension, walker->extension_length) != 0) {
            /* Base name doesn't end with desired extension. */
            clog_debug("Skipping non-plugin file %s", full_path);
            return 0;
        }
    }

    /* Then load the file. */
    return pvt_registry_add_file(walker->reg, full_path);
}

static int
pvt_registry_walk__enter(struct cork_dir_walker *vwalker, const char *full_path,
                         const char *rel_path, const char *base_name)
{
    return 0;
}

static int
pvt_registry_walk__leave(struct cork_dir_walker *vwalker, const char *full_path,
                         const char *rel_path, const char *base_name)
{
    return 0;
}

int
pvt_registry_add_directory(struct pvt_registry *self, const char *path,
                           const char *extension)
{
    struct pvt_walker  walker;
    walker.parent.file = pvt_registry_walk__file;
    walker.parent.enter_directory = pvt_registry_walk__enter;
    walker.parent.leave_directory = pvt_registry_walk__leave;
    walker.reg = self;
    if (extension == NULL) {
        walker.extension_length = 0;
        walker.extension = NULL;
    } else {
        walker.extension_length = strlen(extension);
        walker.extension = extension;
    }
    return cork_walk_directory(path, &walker.parent);
}


/*-----------------------------------------------------------------------
 * Loading everything
 */

static cork_hash
constant_hasher(const void *vk)
{
    return (cork_hash) (uintptr_t) vk;
}

static bool
constant_comparator(const void *vk1, const void *vk2)
{
    return vk1 == vk2;
}

static int
pvt_registry_load_one_plugin(struct pvt_registry *self,
                             struct cork_hash_table *loading,
                             struct pvt_plugin *plugin, void *ud)
{
    size_t  i;

    /* Have we already loaded this plugin? */
    if (plugin->loaded) {
        return 0;
    }

    /* If we've started loading this plugin, but haven't finished, then we've
     * got a circular chain of dependencies. */
    if (cork_hash_table_get(loading, plugin) != NULL) {
        pvt_circular_dependency
            ("Circular dependency when loading %s", plugin->desc->name);
        return -1;
    }

    /* Mark that we've started loading this plugin. */
    cork_hash_table_put(loading, plugin, plugin, NULL, NULL, NULL);

    /* Load in any dependencies first. */
    for (i = 0; i < plugin->desc->dependency_count; i++) {
        const char  *dep_name = plugin->desc->dependencies[i];
        struct pvt_plugin  *dep_plugin  =
            cork_hash_table_get(&self->plugins, (void *) dep_name);
        if (dep_plugin == NULL) {
            pvt_undefined("Missing dependency %s for plugin %s",
                          dep_name, plugin->desc->name);
            return -1;
        } else {
            rii_check(pvt_registry_load_one_plugin
                      (self, loading, dep_plugin, ud));
        }
    }

    /* Then call the plugin's loader function, and mark that we finished loading
     * the plugin. */
    rii_check(plugin->loader->load(self, plugin->desc, ud));
    plugin->loaded = true;
    return 0;
}

int
pvt_registry_load_one(struct pvt_registry *self, const char *name, void *ud)
{
    struct cork_hash_table  loading;
    struct pvt_plugin  *plugin;

    plugin = cork_hash_table_get(&self->plugins, (void *) name);
    if (plugin == NULL) {
        pvt_undefined("Unknown plugin %s", plugin->desc->name);
        return -1;
    }

    cork_hash_table_init(&loading, 0, constant_hasher, constant_comparator);
    ei_check(pvt_registry_load_one_plugin(self, &loading, plugin, ud));
    cork_hash_table_done(&loading);
    return 0;

error:
    cork_hash_table_done(&loading);
    return -1;
}

int
pvt_registry_load_all(struct pvt_registry *self, void *ud)
{
    struct cork_hash_table  loading;
    struct cork_hash_table_iterator  iter;
    struct cork_hash_table_entry  *entry;
    cork_hash_table_init(&loading, 0, constant_hasher, constant_comparator);
    cork_hash_table_iterator_init(&self->plugins, &iter);
    while ((entry = cork_hash_table_iterator_next(&iter)) != NULL) {
        struct pvt_plugin  *plugin = entry->value;
        ei_check(pvt_registry_load_one_plugin(self, &loading, plugin, ud));
    }
    cork_hash_table_done(&loading);
    return 0;

error:
    cork_hash_table_done(&loading);
    return -1;
}

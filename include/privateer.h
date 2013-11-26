/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef PRIVATEER_H
#define PRIVATEER_H

#include <libcork/core.h>

#include <privateer/config.h>


/* forward declarations */

struct pvt_ctx;
struct pvt_extension;
struct pvt_extension_point;
struct pvt_plugin;
struct pvt_plugin_registration;


/*-----------------------------------------------------------------------
 * Error handling
 */

/* hash of "privateer.h" */
#define PVT_ERROR  0xd3d52b87

enum pvt_error {
    PVT_BAD_CONFIG,
    PVT_BAD_LIBRARY,
    PVT_CIRCULAR_DEPENDENCY,
    PVT_REDEFINED,
    PVT_UNDEFINED
};


#define pvt_set_error(code, ...) (cork_error_set(PVT_ERROR, code, __VA_ARGS__))
#define pvt_bad_config(...) \
    pvt_set_error(PVT_BAD_CONFIG, __VA_ARGS__)
#define pvt_bad_library(...) \
    pvt_set_error(PVT_BAD_LIBRARY, __VA_ARGS__)
#define pvt_circular_dependency(...) \
    pvt_set_error(PVT_CIRCULAR_DEPENDENCY, __VA_ARGS__)
#define pvt_redefined(...) \
    pvt_set_error(PVT_REDEFINED, __VA_ARGS__)
#define pvt_undefined(...) \
    pvt_set_error(PVT_UNDEFINED, __VA_ARGS__)


/*-----------------------------------------------------------------------
 * Extensions
 */

const char *
pvt_extension_name(const struct pvt_extension *ext);

struct pvt_extension_point *
pvt_extension_extension_point(const struct pvt_extension *ext);

struct pvt_plugin *
pvt_extension_plugin(const struct pvt_extension *ext);

void *
pvt_extension_user_data(const struct pvt_extension *ext);


/*-----------------------------------------------------------------------
 * Extension points
 */

struct pvt_extension_point *
pvt_extension_point_new(struct pvt_ctx *reg, const char *name);

void
pvt_extension_point_free(struct pvt_extension_point *exts);

const char *
pvt_extension_point_name(const struct pvt_extension_point *exts);

struct pvt_ctx *
pvt_extension_point_context(const struct pvt_extension_point *exts);

struct pvt_extension *
pvt_extension_point_add(struct pvt_extension_point *exts,
                        struct pvt_plugin *plugin, const char *name,
                        void *user_data, cork_free_f free_user_data);

void *
pvt_extension_point_get(const struct pvt_extension_point *exts,
                        const char *name);

void *
pvt_extension_point_require(const struct pvt_extension_point *exts,
                            const char *name);

size_t
pvt_extension_point_count(const struct pvt_extension_point *exts);

const struct pvt_extension *
pvt_extension_point_at(const struct pvt_extension_point *exts, size_t index);


/*-----------------------------------------------------------------------
 * Contexts
 */

struct pvt_ctx;

struct pvt_ctx *
pvt_ctx_new(void);

void
pvt_ctx_free(struct pvt_ctx *ctx);


struct pvt_extension_point *
pvt_ctx_root_extension_point(struct pvt_ctx *ctx, const char *name);

size_t
pvt_ctx_extension_point_count(const struct pvt_ctx *ctx);

const struct pvt_extension_point *
pvt_ctx_extension_point_at(const struct pvt_ctx *ctx, size_t index);


/*-----------------------------------------------------------------------
 * Plugins
 */

const char *
pvt_plugin_name(const struct pvt_plugin *plugin);

const char *
pvt_plugin_library_name(const struct pvt_plugin *plugin);

int
pvt_plugin_load(struct pvt_plugin *plugin);

bool
pvt_plugin_loaded(const struct pvt_plugin *plugin);

struct pvt_ctx *
pvt_plugin_context(const struct pvt_plugin *plugin);

size_t
pvt_plugin_extension_count(const struct pvt_plugin *plugin);

const struct pvt_extension *
pvt_plugin_extension_at(const struct pvt_plugin *plugin, size_t index);


struct pvt_plugin *
pvt_ctx_add_static_plugin(struct pvt_ctx *ctx,
                          const struct pvt_plugin_registration *plugin);

struct pvt_plugin *
pvt_ctx_add_dynamic_plugin(struct pvt_ctx *ctx, const char *plugin_name,
                           const char *library_name);

const struct pvt_plugin *
pvt_ctx_get_plugin(const struct pvt_ctx *ctx, const char *plugin_name);

size_t
pvt_ctx_plugin_count(const struct pvt_ctx *ctx);

const struct pvt_plugin *
pvt_ctx_plugin_at(const struct pvt_ctx *ctx, size_t index);

/* registry_paths must be a NULL-terminated array.  plugin_names must be NULL or
 * a NULL-terminated array. */
int
pvt_ctx_find_plugins(struct pvt_ctx *ctx, const char **registry_paths,
                     const char **plugin_names);

int
pvt_ctx_load_plugins(struct pvt_ctx *ctx);

int
pvt_ctx_require_plugin(struct pvt_ctx *ctx, const char *plugin_name);



/*-----------------------------------------------------------------------
 * Plugin registrations
 */

/* hash of "pvt_plugin" */
#define PVT_PLUGIN_REGISTRATION_MAGIC  0x18168469
#define PVT_PLUGIN_REGISTRATION_CURRENT_VERSION  1

typedef void
(*pvt_plugin_register_f)(struct pvt_ctx *ctx,
                         struct pvt_plugin *plugin);

/* The first two fields of this struct must always be set to
 * PVT_PLUGIN_REGISTRATION_MAGIC and _CURRENT_VERSION, respectively.  (The
 * pvt_define_plugin macro takes care of that for you.)
 *
 * The magic number gives us some basic error-checking to ensure that the symbol
 * referred to in each registration file really does refer to a plugin
 * registration, and the version lets us make changes to the struct without
 * breaking ABI compatibility.  (Our internal functions for processing
 * registrations will always be able to process all existing version numbers, or
 * raise an error if a registration version is no longer viable.) */

struct pvt_plugin_registration {
    cork_hash  magic;
    unsigned int  version;
    const char  *plugin_name;
    pvt_plugin_register_f  reg;
};


/*-----------------------------------------------------------------------
 * Helper macros
 */

#define pvt_static_plugin(c_name)  (&c_name##__plugin)

#define pvt_declare_plugin(c_name) \
extern const struct pvt_plugin_registration  c_name##__plugin; \

#define pvt_define_plugin(c_name) \
static void \
c_name##__plugin__register(struct pvt_ctx *ctx, \
                           struct pvt_plugin *plugin); \
\
const struct pvt_plugin_registration  c_name##__plugin = { \
    PVT_PLUGIN_REGISTRATION_MAGIC, \
    PVT_PLUGIN_REGISTRATION_CURRENT_VERSION, \
    #c_name, \
    c_name##__plugin__register \
}; \
\
static void \
c_name##__plugin__register(struct pvt_ctx *ctx, \
                           struct pvt_plugin *plugin)

#define pvt_require_plugin(dep_name) \
    do { \
        int  rc; \
        struct pvt_plugin  *dep = pvt_ctx_add_static_plugin \
            (ctx, pvt_static_plugin(dep_name)); \
        if (CORK_UNLIKELY(dep == NULL)) { \
            return; \
        } \
        rc = pvt_plugin_load(dep); \
        if (CORK_UNLIKELY(rc != 0)) { \
            return; \
        } \
    } while (0)


#endif /* PRIVATEER_H */

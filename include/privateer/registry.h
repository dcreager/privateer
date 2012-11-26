/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PRIVATEER_REGISTRY_H
#define PRIVATEER_REGISTRY_H

#include <libcork/core.h>


struct pvt_plugin_descriptor {
    const char  *descriptor_path;
    const char  *name;
    const char  *library_path;
    const char  *loader_name;
    size_t  dependency_count;
    const char  **dependencies;
};


struct pvt_registry;

struct pvt_registry *
pvt_registry_new(void);

void
pvt_registry_free(struct pvt_registry *reg);

int
pvt_registry_add_plugin(struct pvt_registry *reg,
                        struct pvt_plugin_descriptor *desc);

int
pvt_registry_add_file(struct pvt_registry *reg, const char *path);

int
pvt_registry_add_directory(struct pvt_registry *reg, const char *path,
                           const char *extension);

struct pvt_plugin_descriptor *
pvt_registry_get_descriptor(struct pvt_registry *reg, const char *name);


/* This has to be a struct, not a bare function pointer, since C doesn't require
 * function pointers to be compatible with (void *). */
struct pvt_loader {
    int
    (*load)(struct pvt_registry *reg, struct pvt_plugin_descriptor *desc,
            void *ud);
};


#define pvt_define_loader(name) \
static int \
name##__loader(struct pvt_registry *reg, struct pvt_plugin_descriptor *desc, \
               void *ud); \
\
struct pvt_loader  name = { name##__loader }; \
\
static int \
name##__loader(struct pvt_registry *reg, struct pvt_plugin_descriptor *desc, \
               void *ud)


#endif /* PRIVATEER_REGISTRY_H */

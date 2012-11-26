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


#endif /* PRIVATEER_REGISTRY_H */

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

#include <yaml.h>


struct pvt_registry;

/* registry_paths must be a NULL-terminated array. */
struct pvt_registry *
pvt_registry_new(size_t registry_path_count, const char **registry_paths);

void
pvt_registry_free(struct pvt_registry *reg);

int
pvt_registry_load_section(struct pvt_registry *reg, const char *entity_name,
                          const char *section_name, yaml_document_t *dest);


#endif /* PRIVATEER_REGISTRY_H */

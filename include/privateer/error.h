/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#ifndef PRIVATEER_ERROR_H
#define PRIVATEER_ERROR_H

#include <libcork/core.h>

/* hash of "privateer.h" */
#define PVT_ERROR  0xd3d52b87

enum pvt_error {
    PVT_REDEFINED,
    PVT_UNDEFINED,
    PVT_YAML_ERROR
};


#define pvt_set_error(code, ...) (cork_error_set(PVT_ERROR, code, __VA_ARGS__))
#define pvt_redefined(...) \
    pvt_set_error(PVT_REDEFINED, __VA_ARGS__)
#define pvt_undefined(...) \
    pvt_set_error(PVT_UNDEFINED, __VA_ARGS__)
#define pvt_yaml_error(...) \
    pvt_set_error(PVT_YAML_ERROR, __VA_ARGS__)


#endif /* PRIVATEER_ERROR_H */

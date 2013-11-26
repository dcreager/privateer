/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdio.h>

#include <clogger.h>

#include "privateer.h"
#include "privateer-tests.h"

#define CLOG_CHANNEL  "circular"

pvt_define_plugin(pvt_circular)
{
    pvt_require_plugin(pvt_gamma);
    pvt_require_plugin(pvt_circular);
    clog_debug("Load circular");
}

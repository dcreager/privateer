/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#include <stdio.h>

#include <clogger.h>

#include "privateer.h"
#include "privateer-tests.h"

#define CLOG_CHANNEL  "beta"

pvt_define_plugin(pvt_beta)
{
    pvt_require_plugin(pvt_alpha);
    clog_debug("Load beta");
}

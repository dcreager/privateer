/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2011-2013, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the COPYING file in this distribution for license details.
 * ----------------------------------------------------------------------
 */

#ifndef TESTS_HELPERS_H
#define TESTS_HELPERS_H

#include <stdlib.h>
#include <string.h>

#include <libcork/core.h>

#if !defined(PRINT_EXPECTED_FAILURES)
#define PRINT_EXPECTED_FAILURES  1
#endif

#if PRINT_EXPECTED_FAILURES
#define print_expected_failure() \
    printf("[expected: %s]\n", cork_error_message());
#else
#define print_expected_failure()  /* do nothing */
#endif


#define DESCRIBE_TEST \
    fprintf(stderr, "--- %s\n", __func__);


#define fail_if_error(call) \
    do { \
        call; \
        if (cork_error_occurred()) { \
            fail("%s", cork_error_message()); \
        } \
    } while (0)

#define fail_unless_error(call, ...) \
    do { \
        call; \
        if (!cork_error_occurred()) { \
            fail(__VA_ARGS__); \
        } else { \
            print_expected_failure(); \
        } \
        cork_error_clear(); \
    } while (0)

#define fail_unless_equal(what, format, expected, actual) \
    (fail_unless((expected) == (actual), \
                 "%s not equal (expected " format \
                 ", got " format ")", \
                 (what), (expected), (actual)))

#define fail_unless_streq(what, expected, actual) \
    (fail_unless(strcmp((expected) == NULL? "": (expected), \
                        (actual) == NULL? "": (actual)) == 0, \
                 "%s not equal (expected \"%s\", got \"%s\")", \
                 (char *) (what), (char *) (expected), (char *) (actual)))


static void
initialize_tests(void)
{
    clog_set_default_format("[%L] %m");
    if (clog_setup_logging() != 0) {
        fprintf(stderr, "%s\n", cork_error_message());
        exit(EXIT_FAILURE);
    }
}

static void
finalize_tests(void)
{
}


#endif /* TESTS_HELPERS_H */
/* -*- coding: utf-8 -*-
 * ----------------------------------------------------------------------
 * Copyright © 2012, RedJack, LLC.
 * All rights reserved.
 *
 * Please see the LICENSE.txt file in this distribution for license
 * details.
 * ----------------------------------------------------------------------
 */

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <clogger.h>
#include <libcork/core.h>
#include <yaml.h>

#include "privateer/error.h"
#include "privateer/registry.h"

#define CLOG_CHANNEL  "pvt-registry"


struct pvt_registry {
    size_t  path_count;
    const char  **paths;
};


struct pvt_registry *
pvt_registry_new(size_t registry_path_count, const char **registry_paths)
{
    struct pvt_registry  *self = cork_new(struct pvt_registry);
    size_t  i;
    self->path_count = registry_path_count;
    self->paths = cork_calloc(registry_path_count, sizeof(const char *));
    for (i = 0; i < registry_path_count; i++) {
        self->paths[i] = cork_strdup(registry_paths[i]);
    }
    return self;
}

void
pvt_registry_free(struct pvt_registry *self)
{
    size_t  i;
    for (i = 0; i < self->path_count; i++) {
        cork_strfree(self->paths[i]);
    }
    free(self->paths);
    free(self);
}


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
pvt_registry_load_file(const char *filename, yaml_document_t *dest)
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
        pvt_yaml_error(parser.problem);
        yaml_parser_delete(&parser);
        return -1;
    }

    yaml_parser_delete(&parser);
    return 0;
}


#define REG_MISS  -2

static int
pvt_registry_try_one_path(struct pvt_registry *self, const char *registry_path,
                          const char *entity_name, const char *section_name,
                          yaml_document_t *dest)
{
    int  rc;
    size_t  len;
    struct cork_buffer  path = CORK_BUFFER_INIT();
    struct stat  info;

    /* Remove any trailing slashes from the library path */
    len = strlen(registry_path);
    while (registry_path[len-1] == '/') {
        len--;
    }
    cork_buffer_set(&path, registry_path, len);

    /* Build the rest of the path to the registration file */
    cork_buffer_append_printf(&path, "/%s/%s.yaml", entity_name, section_name);

    /* Check to see if the file exists.  That will be a special error. */
    rc = stat(path.buf, &info);
    if (CORK_UNLIKELY(rc == -1)) {
        if (CORK_LIKELY(errno == ENOENT)) {
            cork_buffer_done(&path);
            return REG_MISS;
        } else {
            cork_system_error_set();
            cork_buffer_done(&path);
            return -1;
        }
    }

    /* Try to load in the file */
    clog_debug("Loading %s:%s from %s", entity_name, section_name,
               (char *) path.buf);
    rc = pvt_registry_load_file(path.buf, dest);
    cork_buffer_done(&path);
    return rc;
}

int
pvt_registry_load_section(struct pvt_registry *self, const char *entity_name,
                          const char *section_name, yaml_document_t *dest)
{
    size_t  i;
    for (i = 0; i < self->path_count; i++) {
        int  rc = pvt_registry_try_one_path
            (self, self->paths[i], entity_name, section_name, dest);

        /* REG_MISS is a special code indicating that the registration doesn't
         * exist in this path.  -1 indicates that the registration exists, but
         * there was an error loading it.  0 indicates success, as usual. */
        if (CORK_LIKELY(rc != REG_MISS)) {
            return rc;
        }
    }

    /* If we fall through, then none of the paths contained the desired object. */
    pvt_undefined("Cannot find %s:%s in registry", entity_name, section_name);
    return -1;
}

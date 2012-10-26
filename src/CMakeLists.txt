# -*- coding: utf-8 -*-
# ----------------------------------------------------------------------
# Copyright © 2011, RedJack, LLC.
# All rights reserved.
#
# Please see the LICENSE.txt file in this distribution for license
# details.
# ----------------------------------------------------------------------

include_directories(../include)

#-----------------------------------------------------------------------
# Build the library

set(LIBPRIVATEER_SRC
    libprivateer/registry.c
)

add_library(libprivateer SHARED ${LIBPRIVATEER_SRC})
set_target_properties(libprivateer PROPERTIES
    OUTPUT_NAME privateer
    SOVERSION 0.0.0)
target_link_libraries(libprivateer
    ${YAML_LIBRARIES}
    ${CORK_LIBRARIES}
    ${CLOG_LIBRARIES}
)

install(TARGETS libprivateer DESTINATION lib)


set(PRIVATEER_SRC
    privateer/privateer.c
)

add_executable(privateer ${PRIVATEER_SRC})
target_link_libraries(privateer libprivateer)
install(TARGETS privateer DESTINATION bin)

#-----------------------------------------------------------------------
# Generate the pkg-config file

set(prefix ${CMAKE_INSTALL_PREFIX})
configure_file(privateer.pc.in privateer.pc @ONLY)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/privateer.pc
        DESTINATION lib/pkgconfig)

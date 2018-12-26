
find_path(LIBUNWIND_INCLUDE_DIR 
    NAMES libunwind.h
    HINTS /usr/include/ /usr/local/include/
)

find_library(LIBUNWIND_LIBRARIES
    NAMES unwind-generic
    HINTS /usr/lib/ /usr/lib64/ /usr/local/lib64/ /usr/local/lib/
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Libunwind DEFAULT_MSG 
    LIBUNWIND_INCLUDE_DIR LIBUNWIND_LIBRARIES
)

if(NOT TARGET Libunwind::libunwind)
    add_library(Libunwind::libunwind UNKNOWN IMPORTED)
    set_target_properties(Libunwind::libunwind PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES ${LIBUNWIND_INCLUDE_DIR}
        IMPORTED_LOCATION ${LIBUNWIND_LIBRARIES}
        IMPORTED_LINK_INTERFACE_LANGUAGES "C"
    )
endif()

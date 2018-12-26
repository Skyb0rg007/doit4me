
find_program(ADDR2LINE_COMMAND
    NAMES addr2line
    HINTS /usr/bin/
)

find_program(AWK_COMMAND
    NAMES awk gawk
    HINTS /usr/bin/
)

if(NOT ADDR2LINE_COMMAND)
    message(FATAL_ERROR "addr2line command not found!")
endif()
if(NOT AWK_COMMAND)
    message(FATAL_ERROR "awk command not found!")
endif()

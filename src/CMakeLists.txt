# libraries

# netUtil library
add_library(${TARGET_NET_UTIL} STATIC)
target_sources(${TARGET_NET_UTIL}
        PRIVATE netUtil.c
        PUBLIC util.c
        )
target_include_directories(${TARGET_NET_UTIL} PUBLIC ../include)

# services library
add_library(${TARGET_SERVICES} STATIC)
target_sources(${TARGET_SERVICES} PRIVATE services.c)
target_link_libraries(${TARGET_SERVICES}
        PUBLIC  ${TARGET_NET_UTIL}
                ${TARGET_DBMS}
        )

# dbms library code
add_subdirectory(dbms)

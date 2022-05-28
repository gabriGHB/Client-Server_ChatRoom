# dbms library

add_library(${TARGET_DBMS} STATIC)
target_sources(${TARGET_DBMS}
        PRIVATE     dbms.c
                    dbmsUtil.c
                    ../util.c
        )
target_include_directories(${TARGET_DBMS} PRIVATE ../../include)

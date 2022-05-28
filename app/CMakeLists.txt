# server app

# server app
add_executable(${TARGET_SERVER})
target_sources(${TARGET_SERVER} PRIVATE server.c)
target_link_libraries(${TARGET_SERVER}
        PRIVATE pthread
                ${TARGET_SERVICES}
        )

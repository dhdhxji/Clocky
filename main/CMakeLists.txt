idf_component_register(SRCS "main.cpp" "file_server/file_server.c"
                    INCLUDE_DIRS "" "file_server"
                    EMBED_FILES "file_server/favicon.ico" "file_server/upload_script.html")

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
idf_component_register(
    SRCS cfg.cpp
    REQUIRES color_processor
    EMBED_FILES serpent.lua
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
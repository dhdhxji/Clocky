idf_component_register(
    SRCS cfg.cpp
    INCLUDE_DIRS "."
    REQUIRES color_processor
    EMBED_TXTFILES serpent.lua
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_17)
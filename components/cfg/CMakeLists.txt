idf_component_register(
    SRCS cfg.cpp
    INCLUDE_DIRS "."
)

include("${PROJECT_DIR}/util/CPM.cmake")

CPMAddPackage("gh:nlohmann/json@3.10.5")
target_link_libraries(${COMPONENT_LIB} nlohmann_json)
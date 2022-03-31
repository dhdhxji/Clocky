idf_component_register(
    SRCS stub.c
)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mlongcalls -mtext-section-literals")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mlongcalls -mtext-section-literals")

include("${PROJECT_DIR}/util/CPM.cmake")

CPMAddPackage("gh:dhdhxji/ColProc#dev")

target_link_libraries(${COMPONENT_LIB} PUBLIC colproc)
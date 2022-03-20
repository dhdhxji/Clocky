idf_component_register(
    SRCS stub.c
)

include("CPM.cmake")
CPMAddPackage("gh:dhdhxji/ColProc#dev")

target_link_libraries(${COMPONENT_LIB} PUBLIC colproc)
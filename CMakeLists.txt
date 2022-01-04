cmake_minimum_required(VERSION 3.16)

set(PROJECT_NAME fbrpc_binding)
project(${PROJECT_NAME})

add_definitions(-DNOMINMAX -D_UNICODE -D_CRT_SECURE_NO_WARNINGS -DSPDLOG_HEADER_ONLY)

set(FBRPC_PATH ${CMAKE_SOURCE_DIR}/../fbrpc)

find_package(flatbuffers CONFIG REQUIRED)
find_path(FLAT_BUFFER_PATH flatbuffers/flatbuffers.h NO_CACHE)
set(FLATC_PATH "${FLAT_BUFFER_PATH}/../tools/flatbuffers/flatc")

set(TARGET_OUTPUT_DIR ${CMAKE_SOURCE_DIR}/lib)

set(PROTO_ROOT ${FBRPC_PATH}/tests/proto)
set(PROTO_GEN_ROOT ${CMAKE_SOURCE_DIR}/src/generated)
set(TS_GEN_ROOT ${CMAKE_SOURCE_DIR}/lib)
set(BINDING_GEN_ROOT ${CMAKE_SOURCE_DIR}/src/generated)

file(REMOVE_RECURSE ${PROTO_GEN_ROOT} ${TS_GEN_ROOT} ${BINDING_GEN_ROOT})

macro(generateBindings)
    file(REMOVE_RECURSE ${PROTO_GEN_ROOT})
    file(GLOB_RECURSE PROTO_FILES ${PROTO_ROOT}/*.fbs)

    file(REMOVE_RECURSE ${PROTO_GEN_ROOT})
    file(GLOB_RECURSE PROTO_FILES ${PROTO_ROOT}/*.fbs)

    foreach(PROTO_FILE ${PROTO_FILES})
        get_filename_component(PROTO_NAME ${PROTO_FILE} NAME)
        string(REGEX REPLACE "[.]fbs$" "_generated.h" PROTO_GEN_FILE_NAME ${PROTO_NAME})
        set(PROTO_GEN_FILE ${PROTO_GEN_ROOT}/${PROTO_GEN_FILE_NAME})
        list(APPEND PROTO_GEN_FILES ${PROTO_GEN_FILE})

        add_custom_command(
            OUTPUT ${PROTO_GEN_FILE}
            COMMAND ${FLATC_PATH} -c -o ${PROTO_GEN_ROOT} -I ${PROTO_ROOT} ${PROTO_FILE}
            DEPENDS ${PROTO_FILE}
            COMMENT "[flatc] generating ${PROTO_FILE} to ${PROTO_GEN_FILE} ..."
            VERBATIM
        )

        string(REGEX REPLACE "[.]fbs$" "Binding_generated.h" BINDING_GEN_FILE_NAME ${PROTO_NAME})
        set(BINDING_GEN_FILE ${PROTO_GEN_ROOT}/${BINDING_GEN_FILE_NAME})
        list(APPEND BINDING_GEN_FILES ${BINDING_GEN_FILE})
    endforeach()

    list(APPEND BINDING_GEN_FILES ${PROTO_GEN_ROOT}/FlatBufferRpcBinding_generated.h)

    add_custom_command(
        OUTPUT ${BINDING_GEN_FILES}
        COMMAND ${FBRPC_PATH}/bin/fbrpc_generator -n ${PROTO_ROOT} ${BINDING_GEN_ROOT}
        DEPENDS ${PROTO_FILES}
        COMMENT "[fbrpc_generator] generating fbrpc services binding ..."
        VERBATIM
    )
endmacro()

macro(generateTsBinding)
    foreach(PROTO_FILE ${PROTO_FILES})
        add_custom_command(
            TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND ${FLATC_PATH} --ts --gen-object-api -o ${TS_GEN_ROOT} -I ${PROTO_ROOT} ${PROTO_FILE}
            DEPENDS ${PROTO_FILE}
            COMMENT "[flatc] generating ${PROTO_FILE} for ts ..."
            VERBATIM
        )
    endforeach()
    foreach(PROTO_FILE ${PROTO_FILES})
        add_custom_command(
            TARGET ${PROJECT_NAME}
            PRE_BUILD
            COMMAND ${FBRPC_PATH}/bin/fbrpc_generator -t ${PROTO_ROOT} ${TS_GEN_ROOT}
            DEPENDS ${PROTO_FILE}
            COMMENT "[fbrpc_generator] generating ${PROTO_FILE} for ts ..."
            VERBATIM
        )
    endforeach()
endmacro()

generateBindings()

file(GLOB_RECURSE SOURCE_FILES src/*.h src/*.cpp)
add_library(${PROJECT_NAME} SHARED ${SOURCE_FILES} ${PROTO_GEN_FILES} ${BINDING_GEN_FILES})

generateTsBinding()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_17)

execute_process(COMMAND node -p "require('node-addon-api').include"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE NODE_ADDON_API_DIR
)

string(REPLACE "\n" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})
string(REPLACE "\"" "" NODE_ADDON_API_DIR ${NODE_ADDON_API_DIR})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_JS_INC})
target_include_directories(${PROJECT_NAME} PRIVATE ${NODE_ADDON_API_DIR})

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "" SUFFIX ".node")
target_link_options(${PROJECT_NAME} PRIVATE "/delayload:node.exe")
target_link_libraries(${PROJECT_NAME} PRIVATE delayimp.lib ${CMAKE_JS_LIB})
target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_SOURCE_DIR}/../spdlog/include/)

set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY_DEBUG ${TARGET_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELEASE ${TARGET_OUTPUT_DIR}
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${TARGET_OUTPUT_DIR}
    PDB_OUTPUT_DIRECTORY_DEBUG ${TARGET_OUTPUT_DIR}
    PDB_OUTPUT_DIRECTORY_RELEASE ${TARGET_OUTPUT_DIR}
    PDB_OUTPUT_DIRECTORY_RELWITHDEBINFO ${TARGET_OUTPUT_DIR}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${FBRPC_PATH})
target_link_libraries(${PROJECT_NAME} PRIVATE ${FBRPC_PATH}/lib/libfbrpc.lib)

target_link_libraries(${PROJECT_NAME} PRIVATE flatbuffers::flatbuffers)
target_link_libraries(${PROJECT_NAME} PRIVATE Ws2_32.lib)
cmake_minimum_required(VERSION 3.20)
project(RayTraceRemastered CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(HL2SDK $ENV{HL2SDKCS2})
set(MMS $ENV{MMSOURCE_DEV})
set(PROTO_DIR $ENV{CSGO_PROTO})

foreach(v HL2SDK MMS PROTO_DIR)
    if(NOT ${v} OR NOT EXISTS "${${v}}")
        message(FATAL_ERROR "${v} env var not set or path missing: '${${v}}'")
    endif()
endforeach()

if(DEFINED ENV{PROTOC} AND EXISTS "$ENV{PROTOC}")
    set(PROTOC_EXE "$ENV{PROTOC}")
else()
    find_program(PROTOC_EXE protoc REQUIRED)
endif()

set(PROTO_FILES
    network_connection.proto
    networkbasetypes.proto
    source2_steam_stats.proto
    netmessages.proto
    valveextensions.proto
    usercmd.proto
    cs_usercmd.proto
)

set(GENERATED_DIR "${CMAKE_BINARY_DIR}/generated")
file(MAKE_DIRECTORY "${GENERATED_DIR}")

set(PROTO_PATHS
    "${PROTO_DIR}"
    "${HL2SDK}/common"
    "${HL2SDK}/game/shared"
    "${HL2SDK}/game/shared/cs"
    "${HL2SDK}/thirdparty/protobuf-3.21.8/src"
)

set(PROTOC_PATH_ARGS "")
foreach(proto_path ${PROTO_PATHS})
    list(APPEND PROTOC_PATH_ARGS "--proto_path=${proto_path}")
endforeach()

set(PROTO_SRCS "")
foreach(p ${PROTO_FILES})
    string(REGEX REPLACE "\\.proto$" ".pb.cc" cc ${p})
    string(REGEX REPLACE "\\.proto$" ".pb.h" hh ${p})
    list(APPEND PROTO_SRCS "${GENERATED_DIR}/${cc}" "${GENERATED_DIR}/${hh}")
    set(proto_input "")
    foreach(proto_path ${PROTO_PATHS})
        if(EXISTS "${proto_path}/${p}")
            set(proto_input "${proto_path}/${p}")
            break()
        endif()
    endforeach()
    if(NOT proto_input)
        message(FATAL_ERROR "Missing proto input '${p}' in PROTO_PATHS")
    endif()
    list(APPEND PROTO_INPUTS "${proto_input}")
endforeach()

add_custom_command(
    OUTPUT ${PROTO_SRCS}
    COMMAND "${PROTOC_EXE}" --cpp_out=${GENERATED_DIR} ${PROTOC_PATH_ARGS} ${PROTO_FILES}
    COMMAND /bin/sh -c "perl -0pi -e 's/ final :/ :/g' '${GENERATED_DIR}'/*.pb.h || true"
    DEPENDS ${PROTO_INPUTS}
    WORKING_DIRECTORY "${PROTO_DIR}"
    COMMENT "protoc cs2 protos"
    VERBATIM
)

set(SDK_SOURCES
    ${HL2SDK}/public/tier0/memoverride.cpp
    ${HL2SDK}/tier1/keyvalues3.cpp
    ${HL2SDK}/tier1/convar.cpp
)

add_library(
    RayTrace SHARED
    src/plugin.cpp
    src/common/sig_scan.cpp
    ${PROTO_SRCS}
    ${SDK_SOURCES}
)

target_include_directories(
    RayTrace
    PRIVATE
    src
    src/common
    ${GENERATED_DIR}
    ${HL2SDK}/thirdparty/protobuf-3.21.8/src
    ${CMAKE_SOURCE_DIR}/third_party
    ${MMS}/core
    ${MMS}/core/sourcemm
        ${MMS}/third_party/khook/include
    ${HL2SDK}/public
    ${HL2SDK}/public/engine
    ${HL2SDK}/public/game/server
    ${HL2SDK}/public/game/shared
    ${HL2SDK}/public/mathlib
    ${HL2SDK}/public/tier0
    ${HL2SDK}/public/entity2
    ${HL2SDK}/game/shared
    ${HL2SDK}/common
)

target_compile_definitions(
    RayTrace
    PRIVATE
    META_IS_SOURCE2
    PLATFORM_64BITS
    X64BITS
    GOOGLE_PROTOBUF_NO_RTTI
)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(HL2SDK_LIBDIR ${HL2SDK}/lib/linux64)
    target_link_libraries(
        RayTrace
        PRIVATE
        ${HL2SDK_LIBDIR}/libtier0.so
        ${HL2SDK_LIBDIR}/mathlib.a
        ${HL2SDK_LIBDIR}/interfaces.a
        ${HL2SDK_LIBDIR}/release/libprotobuf.a
        -Wl,-Bstatic
        m
        -Wl,-Bdynamic
        pthread
        dl
    )
    target_compile_definitions(
        RayTrace
        PRIVATE
        _GNU_SOURCE
        POSIX
        _LINUX
        LINUX
        COMPILER_GCC
        _GLIBCXX_USE_CXX11_ABI=0
        stricmp=strcasecmp
        _stricmp=strcasecmp
        _strnicmp=strncasecmp
        strnicmp=strncasecmp
        _snprintf=snprintf
        _vsnprintf=vsnprintf
        _alloca=alloca
        strcmpi=strcasecmp
    )
    target_compile_options(
        RayTrace
        PRIVATE
        -fPIC
        -fno-rtti
        -fvisibility=hidden
        -idirafter ${HL2SDK}/public/tier1
        -Wno-non-virtual-dtor
        -Wno-overloaded-virtual
        -Wno-register
        -Wno-narrowing
        -Wno-deprecated-copy
    )
    set_target_properties(RayTrace PROPERTIES SUFFIX ".so" PREFIX "")
else()
    message(FATAL_ERROR "Windows build not wired up yet (linux only for now)")
endif()

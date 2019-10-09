cmake_minimum_required(VERSION 3.15)
project(bluekyu_framework
    VERSION 0.1.0
    DESCRIPTION "Framework"
    LANGUAGES CXX
)

# === configure ====================================================================================
string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)

option(${PROJECT_NAME}_ENABLE_RTTI "Enable Run-Time Type Information" OFF)
option(${PROJECT_NAME}_USE_STATIC_CRT "Use static CRT" OFF)
option(${PROJECT_NAME}_BUILD_STATIC "Build static library" ON)
option(${PROJECT_NAME}_BUILD_DEV "Build for development" OFF)

option(${PROJECT_NAME}_BUILD_TEST "Build test programs" OFF)
option(${PROJECT_NAME}_BUILD_RXCPP "Build rxcpp programs" OFF)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")
include(GNUInstallDirs)
include(git-utils)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)    # Project Grouping

# version
set(${PROJECT_NAME}_SEMANTIC_VERSION "${${PROJECT_NAME}_VERSION}")
message("[${PROJECT_NAME}] build version: ${${PROJECT_NAME}_SEMANTIC_VERSION}")

if(EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # git commit
    get_git_head_hash("${PROJECT_SOURCE_DIR}" ${PROJECT_NAME}_GIT_COMMIT)
    message("[${PROJECT_NAME}] git commit: ${${PROJECT_NAME}_GIT_COMMIT}")
else()
    set(${PROJECT_NAME}_GIT_COMMIT "")
endif()

# === project specific packages ===
find_package(spdlog CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS filesystem)
find_package(gainput REQUIRED)

# fix fmt in RELWITHDEBINFO
if(TARGET fmt::fmt)
    get_target_property(fmt_IMPORTED_CONFIGURATIONS fmt::fmt IMPORTED_CONFIGURATIONS)
    if(NOT RELWITHDEBINFO IN_LIST fmt_IMPORTED_CONFIGURATIONS)
        set_target_properties(fmt::fmt PROPERTIES MAP_IMPORTED_CONFIG_RELWITHDEBINFO RELEASE)
    endif()
endif()

# configure
set(${PROJECT_NAME}_MACRO_CMAKE_FILE "${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}-macro.cmake")
include(${${PROJECT_NAME}_MACRO_CMAKE_FILE})

if(${PROJECT_NAME}_BUILD_STATIC)
    set(${PROJECT_NAME}_LIB_TYPE STATIC)
else()
    set(${PROJECT_NAME}_LIB_TYPE SHARED)
endif()

set(${PROJECT_NAME}_GENERATED_DIR "${PROJECT_BINARY_DIR}/generated")
# ==================================================================================================

# ==================================================================================================
function(bluekyu_framework_generate_version_hpp)
    set(dest_file "${${PROJECT_NAME}_GENERATED_DIR}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/version.hpp")
    configure_file("${PROJECT_SOURCE_DIR}/src/version.hpp.in" "${dest_file}" @ONLY)
    target_sources(${PROJECT_NAME} PRIVATE "${dest_file}")
endfunction()
# ==================================================================================================

# === target =======================================================================================
include("${PROJECT_SOURCE_DIR}/files.cmake")
add_library(${PROJECT_NAME} ${${PROJECT_NAME}_LIB_TYPE} ${${PROJECT_NAME}_sources} ${${PROJECT_NAME}_headers})
bluekyu_framework_generate_version_hpp()

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_14)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /MP /wd4251 /wd4275 /utf-8 /permissive-
        $<$<NOT:$<BOOL:${${PROJECT_NAME}_ENABLE_RTTI}>>:/GR->

        # note: windows.cmake in vcpkg
        $<$<CONFIG:Release>:/Oi /Gy /Z7>
    )
    set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS_RELWITHDEBINFO    " /INCREMENTAL:NO /OPT:REF /OPT:ICF ")
    set_property(TARGET ${PROJECT_NAME} APPEND_STRING PROPERTY LINK_FLAGS_RELEASE           " /DEBUG /INCREMENTAL:NO /OPT:REF /OPT:ICF ")
    set_target_properties(${PROJECT_NAME} PROPERTIES MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>$<$<NOT:$<BOOL:${${PROJECT_NAME}_USE_STATIC_CRT}>>:DLL>")
else()
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall
        $<$<NOT:$<BOOL:${${PROJECT_NAME}_ENABLE_RTTI}>>:-fno-rtti>
    )
endif()

target_compile_definitions(${PROJECT_NAME}
    PUBLIC
    $<$<NOT:$<BOOL:${${PROJECT_NAME}_BUILD_STATIC}>>:${PROJECT_NAME_UPPER}_SHARED=1>

    PRIVATE
    ${PROJECT_NAME_UPPER}_BUILD_DEV=$<IF:$<BOOL:${${PROJECT_NAME}_BUILD_DEV}>,1,0>
)

target_include_directories(${PROJECT_NAME}
    PUBLIC $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/${CMAKE_INSTALL_INCLUDEDIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>

    PRIVATE $<BUILD_INTERFACE:${${PROJECT_NAME}_GENERATED_DIR}/${CMAKE_INSTALL_INCLUDEDIR}>
    "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    spdlog::spdlog
    Boost::filesystem
    gainput::gainput
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    FOLDER "${PROJECT_NAME}"
    DEFINE_SYMBOL "${PROJECT_NAME_UPPER}_BUILD"
    DEBUG_POSTFIX ${${PROJECT_NAME}_DEBUG_POSTFIX}
    RELWITHDEBINFO_POSTFIX ${${PROJECT_NAME}_RELWITHDEBINFO_POSTFIX}
    POSITION_INDEPENDENT_CODE ON
    VERSION ${PROJECT_VERSION}
)

# === configure package ===
set(${PROJECT_NAME}_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
if(CMAKE_LIBRARY_ARCHITECTURE)
    set(${PROJECT_NAME}_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}/${CMAKE_LIBRARY_ARCHITECTURE}")
endif()

set(${PROJECT_NAME}_VERSION_CONFIG_FILE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake")
set(${PROJECT_NAME}_CONFIG_FILE "${PROJECT_BINARY_DIR}/${PROJECT_NAME}-config.cmake")
set(${PROJECT_NAME}_TARGET_NAMESPACE "${PROJECT_NAME}::")
set(${PROJECT_NAME}_TARGET_EXPORT_NAME "${PROJECT_NAME}-targets")
set(${PROJECT_NAME}_PACKAGE_CMAKE_INSTALL_DIR "${${PROJECT_NAME}_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

include(CMakePackageConfigHelpers)
write_basic_package_version_file(${${PROJECT_NAME}_VERSION_CONFIG_FILE}
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
)

configure_package_config_file("${PROJECT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in"
    ${${PROJECT_NAME}_CONFIG_FILE}
    INSTALL_DESTINATION ${${PROJECT_NAME}_PACKAGE_CMAKE_INSTALL_DIR}
)

add_library(${${PROJECT_NAME}_TARGET_NAMESPACE}${PROJECT_NAME} ALIAS ${PROJECT_NAME})
# ==================================================================================================

# === install ======================================================================================
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
    EXPORT ${${PROJECT_NAME}_TARGET_EXPORT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${${PROJECT_NAME}_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${${PROJECT_NAME}_INSTALL_LIBDIR}"
)
export(EXPORT ${${PROJECT_NAME}_TARGET_EXPORT_NAME}
    NAMESPACE ${${PROJECT_NAME}_TARGET_NAMESPACE}
    FILE "${PROJECT_BINARY_DIR}/${${PROJECT_NAME}_TARGET_EXPORT_NAME}.cmake"
)

install(DIRECTORY "${PROJECT_SOURCE_DIR}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")
install(DIRECTORY "${${PROJECT_NAME}_GENERATED_DIR}/${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}" DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

install(FILES ${${PROJECT_NAME}_CONFIG_FILE} ${${PROJECT_NAME}_VERSION_CONFIG_FILE} ${${PROJECT_NAME}_MACRO_CMAKE_FILE}
    DESTINATION ${${PROJECT_NAME}_PACKAGE_CMAKE_INSTALL_DIR}
)
install(EXPORT ${${PROJECT_NAME}_TARGET_EXPORT_NAME}
    NAMESPACE ${${PROJECT_NAME}_TARGET_NAMESPACE}
    DESTINATION ${${PROJECT_NAME}_PACKAGE_CMAKE_INSTALL_DIR}
)

if(MSVC AND NOT ${PROJECT_NAME}_BUILD_STATIC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
# ==================================================================================================

# === subdirectory =================================================================================
if(${PROJECT_NAME}_BUILD_TEST)
    if(CMAKE_SYSTEM_NAME STREQUAL Windows)
        add_subdirectory("tests/desktop")
    endif()
endif()

if(${PROJECT_NAME}_BUILD_RXCPP)
    add_subdirectory("tests/rxcpp")
endif()
# ==================================================================================================

cmake_minimum_required(VERSION 3.15)
project(bluekyu_framework_test_rxcpp
    VERSION 0.1.0
    DESCRIPTION "Test sample for rxcpp"
    LANGUAGES CXX
)

# === configure ====================================================================================
string(TOUPPER "${PROJECT_NAME}" PROJECT_NAME_UPPER)

option(${PROJECT_NAME}_ENABLE_RTTI "Enable Run-Time Type Information" OFF)
option(${PROJECT_NAME}_USE_STATIC_CRT "Use static CRT" OFF)
option(${PROJECT_NAME}_BUILD_DEV "Build for development" ${bluekyu_framework_BUILD_DEV})

include(GNUInstallDirs)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)    # Project Grouping

# version
set(${PROJECT_NAME}_SEMANTIC_VERSION "${${PROJECT_NAME}_VERSION}")
message("[${PROJECT_NAME}] build version: ${${PROJECT_NAME}_SEMANTIC_VERSION}")

# === project specific packages ===
find_package(spdlog CONFIG REQUIRED)

# configure
if(${PROJECT_NAME}_BUILD_DEV)
    set(${PROJECT_NAME}_WIN32 "")
else()
    set(${PROJECT_NAME}_WIN32 "WIN32")
endif()

set(${PROJECT_NAME}_DEBUG_POSTFIX "-debug" CACHE INTERNAL "Postfix string used for the name of plugin of Debug mode")
set(${PROJECT_NAME}_RELWITHDEBINFO_POSTFIX "-reldeb" CACHE INTERNAL "Postfix string used for the name of plugin of RelWithDebInfo mode")
# ==================================================================================================

# === target =======================================================================================
include("${PROJECT_SOURCE_DIR}/files.cmake")
add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_WIN32} ${${PROJECT_NAME}_sources} ${${PROJECT_NAME}_headers})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_14)
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /MP /wd4251 /wd4275 /utf-8 /permissive-
        $<$<NOT:$<BOOL:${${PROJECT_NAME}_ENABLE_RTTI}>>:/GR->

        $<$<BOOL:${${PROJECT_NAME}_USE_STATIC_CRT}>:$<IF:$<CONFIG:Debug>,/MTd,/MT>>

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
    PRIVATE
    ${PROJECT_NAME_UPPER}_BUILD_DEV=$<IF:$<BOOL:${${PROJECT_NAME}_BUILD_DEV}>,1,0>
)

target_include_directories(${PROJECT_NAME}
    PRIVATE "${PROJECT_SOURCE_DIR}/src"
)

target_link_libraries(${PROJECT_NAME}
    PRIVATE
    spdlog::spdlog
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    FOLDER "${PROJECT_NAME}"
    DEFINE_SYMBOL "${PROJECT_NAME_UPPER}_BUILD"
    DEBUG_POSTFIX ${${PROJECT_NAME}_DEBUG_POSTFIX}
    RELWITHDEBINFO_POSTFIX ${${PROJECT_NAME}_RELWITHDEBINFO_POSTFIX}
    VERSION ${PROJECT_VERSION}
)

# === configure package ===
set(${PROJECT_NAME}_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}")
if(CMAKE_LIBRARY_ARCHITECTURE)
    set(${PROJECT_NAME}_INSTALL_LIBDIR "${CMAKE_INSTALL_LIBDIR}/${CMAKE_LIBRARY_ARCHITECTURE}")
endif()
# ==================================================================================================

# === install ======================================================================================
set(CMAKE_INSTALL_DEFAULT_COMPONENT_NAME ${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${${PROJECT_NAME}_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${${PROJECT_NAME}_INSTALL_LIBDIR}"
)

if(MSVC)
    install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}> DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
# ==================================================================================================

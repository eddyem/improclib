cmake_minimum_required(VERSION 3.20)
set(PROJ improc)
set(MINOR_VERSION "1")
set(MID_VERSION "0")
set(MAJOR_VERSION "0")
set(VERSION "${MAJOR_VERSION}.${MID_VERSION}.${MINOR_VERSION}")

project(${PROJ} VERSION ${VERSION} LANGUAGES C)

message("VERSION: ${VERSION}")

# list of options
option(DEBUG "Compile in debug mode" OFF)
option(EXAMPLES "Compile also all examples" ON)

# default flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -W -Wextra -std=gnu99")
set(CMAKE_COLOR_MAKEFILE ON)

# here is one of two variants: all .c in directory or .c files in list
aux_source_directory(${CMAKE_CURRENT_SOURCE_DIR} SOURCES)

# cmake -DDEBUG=yes -> debugging
if(DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Og -g3 -ggdb -fno-builtin-strlen -Werror")
    add_definitions(-DEBUG)
    set(CMAKE_BUILD_TYPE DEBUG)
    set(CMAKE_VERBOSE_MAKEFILE "ON")
else()
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -march=native -fdata-sections -ffunction-sections")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections")
    set(CMAKE_BUILD_TYPE RELEASE)
endif()

message("Build type: ${CMAKE_BUILD_TYPE}")

###### pkgconfig ######
# pkg-config modules (for pkg-check-modules)
set(MODULES usefull_macros)
# find packages:
find_package(PkgConfig REQUIRED)
pkg_check_modules(${PROJ} REQUIRED ${MODULES})

# external modules like OpenMP:
include(FindOpenMP)
if(OPENMP_FOUND)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
    add_definitions(-DOMP_FOUND)
endif()

###### additional flags ######
#list(APPEND ${PROJ}_LIBRARIES "-lfftw3_threads")

# library
add_library(${PROJ} SHARED ${SOURCES})
# library header files
set(LIBHEADER "improclib.h")
# -I
include_directories(${${PROJ}_INCLUDE_DIRS})
# -L
link_directories(${${PROJ}_LIBRARY_DIRS})
# -D
add_definitions(-D_XOPEN_SOURCE=1234 -D_DEFAULT_SOURCE -D_GNU_SOURCE -DPACKAGE_VERSION=\"${VERSION}\"
    -DMINOR_VERSION=\"${MINOR_VERSION}\" -DMID_VERSION=\"${MID_VERSION}\"
    -DMAJOR_VERSION=\"${MAJOR_VESION}\")

# -l
target_link_libraries(${PROJ} ${${PROJ}_LIBRARIES} -lm)

set(PCFILE "${CMAKE_BINARY_DIR}/${PROJ}.pc")
configure_file("${PROJ}.pc.in" ${PCFILE} @ONLY)

set_target_properties(${PROJ} PROPERTIES VERSION ${VERSION})
set_target_properties(${PROJ} PROPERTIES PUBLIC_HEADER ${LIBHEADER})

# Installation of the program
include(GNUInstallDirs)
install(TARGETS ${PROJ} LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
install(FILES ${PCFILE} DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)

# EXAMPLES
if(EXAMPLES)
    add_subdirectory(examples)
endif()

###############################################################################
#    HPCC SYSTEMS software Copyright (C) 2014 HPCC Systems.
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.
################################################################################
#
# Description:
# ------------
#           This file is the top level handling for
#           cmake based compilation and build process.
#
#   To build for Linux:
#   1.  Check out source hpcc-systems/wssql
#       run `git clone https://github.com/hpcc-systems/wssql.git --recursive`
#    a. Make sure that you run `git submodule update --init --recursive` in the
#         repository if you haven't already
#   2.  Setup Antlr and Antlr3c.
#    a. "antlr-${ANTLR_VER}-complete.jar" and "antlr-runtime-${ANTLR_VER}.jar"
#       are required.
#    b. Build process looks for these files here: /usr/local/ANTLR/${ANTLR_VER}
#       where ANTLR_VER = 3.4
#    c. "antlr-${ANTLR_VER}-complete.jar" is a build time dep
#    e. "antlr-runtime-${ANTLR_VER}.jar" is a runtime dep(extract its contents
#       in /usr/local/ANTLR/${ANTLR_VER})
#    f. If building a distributable package, include ANTLR license file:
#       https://raw2.github.com/antlr/antlr4/master/LICENSE.txt
#   3.  Create build directory - preferably as a sibling of the source directory
#   4.  cd to the build directory
#   5a. To create makefiles to build native release version for local machine,
#       run `cmake ~/wssql`
#   5b. To create makefiles to build native debug version, run
#       `cmake -DCMAKE_BUILD_TYPE=Debug ~/wssql`
#   6.  To build the makefiles just created above, run `make`
#
################################################################################

project(hpccsystems-wssql)

cmake_minimum_required(VERSION 3.3.2)

set(CMAKE_PROJECT_NAME "hpccsystems-wssql")

set(HPCC_WSSQL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})

if("${HPCC_SOURCE_DIR}" STREQUAL "")
    set(HPCC_SOURCE_DIR ${HPCC_WSSQL_SOURCE_DIR}/HPCC-Platform)
endif()

set(CMAKE_MODULE_PATH "${HPCC_WSSQL_SOURCE_DIR}/cmake_modules")

include(${HPCC_WSSQL_SOURCE_DIR}/version.cmake)
include(${HPCC_SOURCE_DIR}/version.cmake)
include(${HPCC_WSSQL_SOURCE_DIR}/platform-version-prereq.cmake)
include(${CMAKE_MODULE_PATH}/optionDefaults.cmake)
include(${CMAKE_MODULE_PATH}/commonSetup.cmake)


if(NOT MAKE_DOCS_ONLY)
    if("${HPCC_MATURITY}" STREQUAL "release")
        set(hpccstagever "${HPCC_SEQUENCE}")
    else()
        set(hpccstagever "${HPCC_MATURITY}${HPCC_SEQUENCE}")
    endif()

    set(VENDOR_NAME "HPCC SYSTEMS hpccsystems.com")
    set(PROJECT_DESCRIPTION "This package provides a read-only SQL interface into HPCC Systems. Requires build ${HPCC_VERSION} or above")

    ## Platform Build Options
    option(USE_EXTERNALS "Use the externals directory" OFF)
    option(USE_MYSQL "Configure use of MYSQL" ON)
    option(USE_SVN "Configure use of SVN" OFF)
    option(USE_NATIVE_LIBRARIES "Search standard OS locations for thirdparty libraries" ON)

    if(USE_EXTERNALS)
        if("${EXTERNALS_DIRECTORY}" STREQUAL "")
            set(EXTERNALS_DIRECTORY ${CMAKE_SOURCE_DIR}/../externals)
        endif()
    endif()

    message(STATUS "Begin setup of HPCC-Platform build dependency")
    set(USE_RINSIDE OFF CACHE BOOL "Use some expat option")
    add_subdirectory(${HPCC_SOURCE_DIR} oss EXCLUDE_FROM_ALL)
    message(STATUS "End setup of HPCC-Platform build dependency")

    HPCC_ADD_SUBDIRECTORY(esp)
    HPCC_ADD_SUBDIRECTORY(initfiles)

    ###
    ## CPack install and packaging setup.
    ###
    set(VER_SEPARATOR "-")
    if("${stagever}" MATCHES "^rc[0-9]+.*$")
        set(VER_SEPARATOR "~")
    endif()

    set(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
    set(PACKAGE_FILE_NAME_PREFIX ${CMAKE_PROJECT_NAME})

    set(CPACK_PACKAGE_VERSION_MAJOR ${majorver})
    set(CPACK_PACKAGE_VERSION_MINOR ${minorver})
    set(CPACK_PACKAGE_VERSION_PATCH ${point}${VER_SEPARATOR}${stagever})

    set(CPACK_RPM_PACKAGE_VERSION "${version}")
    set(CPACK_RPM_PACKAGE_RELEASE "${stagever}")
    set(CPACK_RPM_EXCLUDE_FROM_AUTO_FILELIST
        ${INSTALL_DIR}
        ${LIB_PATH}
        ${COMPONENTFILES_PATH}
        ${COMPONENTFILES_PATH}/configxml
        ${COMPONENTFILES_PATH}/configxml/@temp
        )

    set(CPACK_RPM_PACKAGE_VENDOR "HPCC Systems")
    set(CPACK_PACKAGE_VENDOR "HPCC Systems")
    set(CPACK_PACKAGE_CONTACT "HPCCSystems <ossdevelopment@lexisnexis.com>")
    set(CPACK_SOURCE_GENERATOR TGZ)

    if(${ARCH64BIT} EQUAL 1)
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
    else(${ARCH64BIT} EQUAL 1)
        set(CPACK_RPM_PACKAGE_ARCHITECTURE "i386")
    endif(${ARCH64BIT} EQUAL 1)

    set(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CPACK_RPM_PACKAGE_ARCHITECTURE}")

    if("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set(CPACK_STRIP_FILES TRUE)
    endif()

    if(UNIX)
        EXECUTE_PROCESS(
            COMMAND ${HPCC_SOURCE_DIR}/cmake_modules/distrocheck.sh
            OUTPUT_VARIABLE packageManagement
            ERROR_VARIABLE  packageManagement
            )
        EXECUTE_PROCESS(
            COMMAND ${HPCC_SOURCE_DIR}/cmake_modules/getpackagerevisionarch.sh
            OUTPUT_VARIABLE packageRevisionArch
            ERROR_VARIABLE  packageRevisionArch
            )
        EXECUTE_PROCESS(
            COMMAND ${HPCC_SOURCE_DIR}/cmake_modules/getpackagerevisionarch.sh --noarch
            OUTPUT_VARIABLE packageRevision
            ERROR_VARIABLE  packageRevision
            )

        message(STATUS "Auto Detecting Packaging type")
        message(STATUS "distro uses ${packageManagement}, revision is ${packageRevisionArch}")

        if("${packageManagement}" STREQUAL "DEB")
            set(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME_PREFIX}-${version}-${stagever}${packageRevisionArch}")
        elseif("${packageManagement}" STREQUAL "RPM")
            set(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME_PREFIX}-${version}-${stagever}.${packageRevisionArch}")
            set(CPACK_RPM_SPEC_MORE_DEFINE
                "%define _use_internal_dependency_generator 0
                %define __getdeps()while read file; do /usr/lib/rpm/rpmdeps -%{1} ${file} | %{__grep} -v libantlr3c.so ; done | /bin/sort -u
                %define __find_provides /bin/sh -c '%{__getdeps P}'
                %define __find_requires /bin/sh -c '%{__getdeps R}'")
        else()
            set(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME_PREFIX}_${version}-${stagever}${CPACK_SYSTEM_NAME}")
        endif()
        message(STATUS "Current release version is ${CPACK_PACKAGE_FILE_NAME}")

        set(CPACK_SOURCE_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME_PREFIX}-${version}")
        set(CPACK_SOURCE_GENERATOR TGZ)
        set(CPACK_SOURCE_IGNORE_FILES
            "~$"
            "\\\\.cvsignore$"
            "^${PROJECT_SOURCE_DIR}.*/CVS/"
            "^${PROJECT_SOURCE_DIR}.*/.svn/"
            "^${PROJECT_SOURCE_DIR}.*/.git/"
            "^${PROJECT_SOURCE_DIR}/ln/"
            "^${PROJECT_SOURCE_DIR}/externals/"
            "^${PROJECT_SOURCE_DIR}.*/*.mk$"
            "^${PROJECT_SOURCE_DIR}/makefile$"
            "^${PROJECT_SOURCE_DIR}/make.common$"
            "^${PROJECT_SOURCE_DIR}/make.post$"
            "^${PROJECT_SOURCE_DIR}/build$"
            "^${PROJECT_SOURCE_DIR}/buildall$"
            "^${PROJECT_SOURCE_DIR}/lastbuilds$"
            "^${PROJECT_SOURCE_DIR}/imerge$"
            "^${PROJECT_SOURCE_DIR}/tmerge$"
            "^${PROJECT_SOURCE_DIR}/tmerge.bat$"
            "^${PROJECT_SOURCE_DIR}/tag$"
            "^${PROJECT_SOURCE_DIR}/tag_build$"
            "^${PROJECT_SOURCE_DIR}/old_tag$"
            "^${PROJECT_SOURCE_DIR}/ecl/regress/"
            "^${PROJECT_SOURCE_DIR}/testing/"
            )

        ###
        ## Run file configuration to set build tag along with install lines for generated
        ## config files.
        ###
        set(BUILD_TAG "${version}-${stagever}")
        if(USE_GIT_DESCRIBE OR CHECK_GIT_TAG)
            FETCH_GIT_TAG(${CMAKE_SOURCE_DIR} ${projname}_${version} GIT_BUILD_TAG)
            message(STATUS "Git tag is '${GIT_BUILD_TAG}'")
            if(NOT "${GIT_BUILD_TAG}" STREQUAL "${BUILD_TAG}")
                if(CHECK_GIT_TAG)
                    message(FATAL_ERROR "Git tag '${GIT_BUILD_TAG}' does not match source version '${BUILD_TAG}'")
                else()
                    if(NOT "${GIT_BUILD_TAG}" STREQUAL "")# probably means being built from a tarball...
                        set(BUILD_TAG "${BUILD_TAG}[${GIT_BUILD_TAG}]")
                    endif()
                endif()
            endif()
        endif()
        message(STATUS "Build tag is '${BUILD_TAG}'")

        if(NOT "${BASE_BUILD_TAG}" STREQUAL "")
            set(BASE_BUILD_TAG "${BUILD_TAG}")
        endif()
        message(STATUS "Base build tag is '${BASE_BUILD_TAG}'")

        configure_file(${HPCC_WSSQL_SOURCE_DIR}/wssql-build-config.h.cmake "wssql-build-config.h")
        configure_file(${HPCC_SOURCE_DIR}/build-config.h.cmake "build-config.h")

        if(UNIX)
            set(CPACK_GENERATOR "${packageManagement}")
            message(STATUS "Package generator: ${packageManagement}")
            if("${packageManagement}" STREQUAL "DEB")
                set(CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/initfiles/bash/sbin/deb/postinst;${CMAKE_CURRENT_BINARY_DIR}/initfiles/sbin/prerm;${CMAKE_CURRENT_BINARY_DIR}/initfiles/bash/sbin/deb/postrm")
            elseif("${packageManagement}" STREQUAL "RPM")
                set(CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/initfiles/bash/sbin/deb/postinst")
                set(CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/initfiles/sbin/prerm")
                set(CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/initfiles/bash/sbin/deb/postrm")
            else()
                message(WARNING "Unsupported package ${packageManagement}.")
            endif()
        endif(UNIX)

        if("${packageManagement}" STREQUAL "DEB")
            set(HPCC_VERSION "${HPCC_MAJOR}.${HPCC_MINOR}.${HPCC_POINT}~")
        else()
            set(HPCC_VERSION "${HPCC_MAJOR}.${HPCC_MINOR}.${HPCC_POINT}")
        endif()

        math(EXPR minor_ceiling "${HPCC_MINOR} + 1")
        set(HPCC_VERSION_CEILING "${HPCC_MAJOR}.${minor_ceiling}.0")

        message(STATUS "Building against HPCC Platform version: ${HPCC_MAJOR}.${HPCC_MINOR}.${HPCC_POINT}-${HPCC_MATURITY}${HPCC_SEQUENCE}")
        message(STATUS "Minimum buildtime HPCC Platform version required: ${HPCC_MAJOR_REQ}.${HPCC_MINOR_REQ}")
        message(STATUS "Maximum runtime HPCC Platform version allowed:  ${HPCC_VERSION_CEILING}")

        if(NOT "${HPCC_SOURCE_DIR}" STREQUAL "${HPCC_WSSQL_SOURCE_DIR}/HPCC-Platform")
            if(${HPCC_MAJOR_REQ} GREATER ${HPCC_MAJOR} OR (${HPCC_MAJOR_REQ} EQUAL ${HPCC_MAJOR} AND ${HPCC_MINOR_REQ} GREATER ${HPCC_MINOR}))
                message(FATAL_ERROR "Project ${project} requires HPCC Platform version ${HPCC_MAJOR_REQ}.${HPCC_MINOR_REQ} or greater.")
            endif()
        endif()

        if(EXISTS ${HPCC_WSSQL_SOURCE_DIR}/cmake_modules/dependencies/${packageRevision}.cmake.in)
            configure_file(${HPCC_WSSQL_SOURCE_DIR}/cmake_modules/dependencies/${packageRevision}.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/dependencies/${packageRevision}.cmake)
            include(${CMAKE_CURRENT_BINARY_DIR}/dependencies/${packageRevision}.cmake)
        else()
            message(WARNING "Dependency file for ${packageRevision} not found, Using deps template.")
            include(${HPCC_WSSQL_SOURCE_DIR}/cmake_modules/dependencies/template.cmake)
        endif()

        if(UNIX)
            set(CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
        endif(UNIX)

        include(CPack)
    endif()

    if(MAKE_DOCS)
        message(STATUS "Making documents")
        add_subdirectory(docs)
    endif()
else()
    message(STATUS "Only making documents")
    add_subdirectory(docs)
endif()

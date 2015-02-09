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
#########################################################
# Description:
# ------------
#           This file is the top level handling for
#           cmake based compilation and build process.
#
#   To build for Linux:
#   1. Check out source hpcc-systems/wssql
#   2. Check out pre-req source hpcc-systems/HPCC-Platform (check ~/wssql/platform-version-prereq.cmake for minimum version required)
#   3. Setup Antlr and Antlr3c.
#    a. "antlr-${ANTLR_VER}-complete.jar" and "antlr-runtime-${ANTLR_VER}.jar" are required.
#    b. Build process looks for these files here: /usr/local/ANTLR/${ANTLR_VER} where ANTLR_VER = 3.4
#    c. "antlr-${ANTLR_VER}-complete.jar" is a build time dep
#    e. "antlr-runtime-${ANTLR_VER}.jar" is a runtime dep (extract its contents in /usr/local/ANTLR/${ANTLR_VER})
#    f. If building a distributable package, make sure to include ANTLR's license file: https://raw2.github.com/antlr/antlr4/master/LICENSE.txt in the ANTLR folder
#   4. Create a build directory - preferably at the same level as the two source dirs "wssql and HPCC-Platform"
#   5. cd to the build directory
#   6a.To create makefiles to build native release version for local machine, run
#       cmake ~/wssql
#   6b.To create makefiles to build native debug version, run
#       cmake -DCMAKE_BUILD_TYPE=Debug ~/wssql
#   7. To build the makefiles just created above, run
#       make
#
#########################################################

PROJECT (hpccsystems-wssql)
cmake_minimum_required (VERSION 2.8.3)

set ( CMAKE_PROJECT_NAME "hpccsystems-wssql")

set (HPCC_WSSQL_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set (HPCC_SOURCE_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/..)

IF ("${HPCC_SOURCE_DIR}" STREQUAL "")
  SET (HPCC_SOURCE_DIR ${HPCC_SOURCE_ROOT_DIR}/HPCC-Platform)
ENDIF()

SET (CMAKE_MODULE_PATH "${HPCC_WSSQL_SOURCE_DIR}/cmake_modules")

INCLUDE(${HPCC_WSSQL_SOURCE_DIR}/version.cmake)
INCLUDE(${HPCC_WSSQL_SOURCE_DIR}/platform-version-prereq.cmake)
INCLUDE(${HPCC_SOURCE_DIR}/version.cmake)

INCLUDE(${CMAKE_MODULE_PATH}/optionDefaults.cmake)
INCLUDE(${CMAKE_MODULE_PATH}/commonSetup.cmake)


IF ( NOT MAKE_DOCS_ONLY )
    IF ( "${HPCC_MATURITY}" STREQUAL "release" )
        set(hpccstagever "${HPCC_SEQUENCE}")
    ELSE()
        set(hpccstagever "${HPCC_MATURITY}${HPCC_SEQUENCE}")
    ENDIF()
    #SET(HPCC_VERSION "${HPCC_MAJOR}.${HPCC_MINOR}.${HPCC_POINT}-${hpccstagever}")
    #Due to unstandard HPCC versioning, we will only request versions up to the point value.
    SET(HPCC_VERSION "${HPCC_MAJOR}.${HPCC_MINOR}.${HPCC_POINT}")

    MESSAGE("-- Minimum HPCC Platform version required: ${HPCC_MAJOR_REQ}.${HPCC_MINOR_REQ}")
    MESSAGE("--- Building against HPCC Platform version: ${HPCC_MAJOR}.${HPCC_MINOR}.${HPCC_POINT}-${HPCC_MATURITY}${HPCC_SEQUENCE}")

    IF (${HPCC_MAJOR_REQ} GREATER ${HPCC_MAJOR} OR ( ${HPCC_MAJOR_REQ} EQUAL ${HPCC_MAJOR} AND ${HPCC_MINOR_REQ} GREATER ${HPCC_MINOR} ))
        MESSAGE(FATAL_ERROR "Project ${project} requires HPCC Platform version ${HPCC_MAJOR_REQ}.${HPCC_MINOR_REQ} or greater." )
    ENDIF()

    SET ( VENDOR_NAME "HPCC SYSTEMS hpccsystems.com")
    SET ( PROJECT_DESCRIPTION "This package provides a read-only SQL interface into HPCC Systems. Requires build ${HPCC_VERSION} or above")

    ## Platform Build Options
    OPTION(USE_EXTERNALS "Use the externals directory" OFF)
    OPTION(USE_MYSQL "Configure use of MYSQL" ON)
    SET (USE_SYBASE 0)  # option(USE_SYBASE "Configure use of SYBASE" OFF)
    SET (USE_HYPERTABLE 0)  # option(USE_HYPERTABLE "Configure building of HyperTable plugin" OFF)
    OPTION(USE_SVN "Configure use of SVN" OFF)
    OPTION(USE_NATIVE_LIBRARIES "Search standard OS locations for thirdparty libraries" ON)

    IF (USE_EXTERNALS)
      IF ("${EXTERNALS_DIRECTORY}" STREQUAL "")
        SET (EXTERNALS_DIRECTORY ${HPCC_SOURCE_ROOT_DIR}/externals)
      ENDIF()
    ENDIF()

    MESSAGE("*** START SETTING UP HPCC PLATFORM ***")

    SET(USE_RINSIDE OFF CACHE BOOL "Use some expat option")
    ADD_SUBDIRECTORY(${HPCC_SOURCE_DIR} oss EXCLUDE_FROM_ALL)

    MESSAGE("*** END SETTING UP HPCC PLATFORM ***")

    HPCC_ADD_SUBDIRECTORY (esp)
    HPCC_ADD_SUBDIRECTORY (initfiles)

    ###
    ## CPack install and packaging setup.
    ###

    SET(CPACK_PACKAGE_NAME ${CMAKE_PROJECT_NAME})
    SET(PACKAGE_FILE_NAME_PREFIX ${CMAKE_PROJECT_NAME})

    SET(CPACK_PACKAGE_VERSION_MAJOR ${majorver})
    SET(CPACK_PACKAGE_VERSION_MINOR ${minorver})
    SET(CPACK_PACKAGE_VERSION_PATCH ${point}-${stagever})

    SET(CPACK_RPM_PACKAGE_VERSION "${version}")
    SET(CPACK_RPM_PACKAGE_RELEASE "${stagever}")

    SET(CPACK_RPM_PACKAGE_VENDOR "HPCC Systems")
    SET(CPACK_PACKAGE_VENDOR "HPCC Systems")
    SET( CPACK_PACKAGE_CONTACT "HPCCSystems <ossdevelopment@lexisnexis.com>" )
    SET( CPACK_SOURCE_GENERATOR TGZ )

    IF    ( ${ARCH64BIT} EQUAL 1 )
        SET ( CPACK_RPM_PACKAGE_ARCHITECTURE "x86_64")
    ELSE  ( ${ARCH64BIT} EQUAL 1 )
        SET ( CPACK_RPM_PACKAGE_ARCHITECTURE "i386")
    ENDIF ( ${ARCH64BIT} EQUAL 1 )

    SET(CPACK_SYSTEM_NAME "${CMAKE_SYSTEM_NAME}-${CPACK_RPM_PACKAGE_ARCHITECTURE}")

    IF ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
        set(CPACK_STRIP_FILES TRUE)
    ENDIF()

    IF ( APPLE )
    ELSEIF ( UNIX )
        EXECUTE_PROCESS (
                    COMMAND ${HPCC_SOURCE_DIR}/cmake_modules/distrocheck.sh
                        OUTPUT_VARIABLE packageManagement
                            ERROR_VARIABLE  packageManagement
                    )
        EXECUTE_PROCESS (
                    COMMAND ${HPCC_SOURCE_DIR}/cmake_modules/getpackagerevisionarch.sh
                        OUTPUT_VARIABLE packageRevisionArch
                            ERROR_VARIABLE  packageRevisionArch
                    )
        EXECUTE_PROCESS (
                    COMMAND ${HPCC_SOURCE_DIR}/cmake_modules/getpackagerevisionarch.sh --noarch
                        OUTPUT_VARIABLE packageRevision
                            ERROR_VARIABLE  packageRevision
                    )

        MESSAGE ( "-- Auto Detecting Packaging type")
        MESSAGE ( "-- distro uses ${packageManagement}, revision is ${packageRevisionArch}" )

        IF ( "${packageManagement}" STREQUAL "DEB" )
            set(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME_PREFIX}-${version}-${stagever}${packageRevisionArch}")
        ELSEIF ( "${packageManagement}" STREQUAL "RPM" )
            set(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME_PREFIX}-${version}-${stagever}.${packageRevisionArch}")
        ELSE ()
            set(CPACK_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME_PREFIX}_${version}-${stagever}${CPACK_SYSTEM_NAME}")
        ENDIF ()
    ENDIF ()
    MESSAGE ("-- Current release version is ${CPACK_PACKAGE_FILE_NAME}")

    SET( CPACK_SOURCE_PACKAGE_FILE_NAME "${PACKAGE_FILE_NAME_PREFIX}-${version}" )
    SET( CPACK_SOURCE_GENERATOR TGZ )
    SET(CPACK_SOURCE_IGNORE_FILES
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
    SET( BUILD_TAG "${version}-${stagever}")
    IF (USE_GIT_DESCRIBE OR CHECK_GIT_TAG)
        FETCH_GIT_TAG (${CMAKE_SOURCE_DIR} ${projname}_${version} GIT_BUILD_TAG)
        MESSAGE ("-- Git tag is '${GIT_BUILD_TAG}'")
        IF (NOT "${GIT_BUILD_TAG}" STREQUAL "${BUILD_TAG}")
            IF (CHECK_GIT_TAG)
                MESSAGE(FATAL_ERROR "Git tag '${GIT_BUILD_TAG}' does not match source version '${BUILD_TAG}'" )
            ELSE()
                IF(NOT "${GIT_BUILD_TAG}" STREQUAL "") # probably means being built from a tarball...
                    SET( BUILD_TAG "${BUILD_TAG}[${GIT_BUILD_TAG}]")
                ENDIF()
            ENDIF()
        ENDIF()
    ENDIF()
    MESSAGE("-- Build tag is '${BUILD_TAG}'")

    IF(NOT "${BASE_BUILD_TAG}" STREQUAL "")
        set(BASE_BUILD_TAG "${BUILD_TAG}")
    ENDIF()
    MESSAGE("-- Base build tag is '${BASE_BUILD_TAG}'")

    CONFIGURE_FILE(${HPCC_SOURCE_DIR}/build-config.h.cmake "build-config.h" )

    IF( UNIX )
        IF( "${packageManagement}" STREQUAL "DEB" )
            set ( CPACK_GENERATOR "${packageManagement}" )
            message("-- Will build DEB package")
            ###
            ## CPack instruction required for Debian
            ###
            MESSAGE("-- Packing BASH installation files")
            SET( CPACK_DEBIAN_PACKAGE_CONTROL_EXTRA "${CMAKE_CURRENT_BINARY_DIR}/initfiles/bash/sbin/deb/postinst;${CMAKE_CURRENT_BINARY_DIR}/initfiles/sbin/prerm;${CMAKE_CURRENT_BINARY_DIR}/initfiles/bash/sbin/deb/postrm" )
        ELSEIF( "${packageManagement}" STREQUAL "RPM" )
            SET( CPACK_GENERATOR "${packageManagement}" )
            ###
            ## CPack instruction required for RPM
            ###
            MESSAGE("-- Will build RPM package")
            MESSAGE("-- Packing BASH installation files")
            SET( CPACK_RPM_POST_INSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/initfiles/bash/sbin/deb/postinst" )
            SET( CPACK_RPM_PRE_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/initfiles/sbin/prerm" )
            SET( CPACK_RPM_POST_UNINSTALL_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/initfiles/bash/sbin/deb/postrm" )
        ELSE()
            MESSAGE("WARNING: Unsupported package ${packageManagement}.")
        ENDIF()
    ENDIF ( UNIX )

    IF ( EXISTS ${HPCC_WSSQL_SOURCE_DIR}/cmake_modules/dependencies/${packageRevision}.cmake.in )
        CONFIGURE_FILE(${HPCC_WSSQL_SOURCE_DIR}/cmake_modules/dependencies/${packageRevision}.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/dependencies/${packageRevision}.cmake)
        INCLUDE(${CMAKE_CURRENT_BINARY_DIR}/dependencies/${packageRevision}.cmake)
    ELSE()
        MESSAGE("-- WARNING: DEPENDENCY FILE FOR ${packageRevision} NOT FOUND, Using deps template.")
        INCLUDE( ${HPCC_WSSQL_SOURCE_DIR}/cmake_modules/dependencies/template.cmake )
    ENDIF()

    IF ( UNIX )
        SET ( CPACK_PACKAGING_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}" )
    ENDIF ( UNIX )

    ###
    ## Below are the non-compile based install scripts required for
    ## the hpcc platform.
    ###

    INCLUDE (CPack)

ENDIF()

IF ( MAKE_DOCS )
    ADD_SUBDIRECTORY(docs)
ENDIF()

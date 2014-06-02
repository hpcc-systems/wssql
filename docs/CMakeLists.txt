################################################################################
# Copyright (C) 2014 HPCC Systems.
#
# All rights reserved. This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Affero General Public License as
# published by the Free Software Foundation, either version 3 of the
# License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU Affero General Public License for more details.
#
# You should have received a copy of the GNU Affero General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
################################################################################

cmake_minimum_required(VERSION 2.8)
PROJECT(docs)

define_property(GLOBAL PROPERTY DOC_TARGETS BRIEF_DOCS "docs" FULL_DOCS "docs")
mark_as_advanced(DOC_TARGETS)

#INCLUDE(${HPCC_SOURCE_DIR}/cmake_modules/docMacros.cmake)
INCLUDE(${CMAKE_MODULE_PATH}/docMacros.cmake)

SET (HPCC_DOCS_DIR ${HPCC_SOURCE_DIR}/docs)

SET (DOCBOOK_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/resources)
SET (DOCBOOK_XSL ${CMAKE_CURRENT_BINARY_DIR}/resources/docbook-xsl)
SET (DOC_IMAGES ${CMAKE_CURRENT_SOURCE_DIR}/)
SET (XML_CATALOG ${CMAKE_CURRENT_BINARY_DIR}/BuildTools/catalog.xml)
#set (DOC_VERSION "${HPCC_MAJOR}.${HPCC_MINOR}.${HPCC_POINT}")
SET (DOC_VERSION "${WSSQL_MAJOR}.${WSSQL_MINOR}.${WSSQL_POINT}")
SET (FO_XSL ${CMAKE_CURRENT_BINARY_DIR}/BuildTools/fo.xsl)
SET (VERSION_DIR ${CMAKE_CURRENT_BINARY_DIR}/)

IF ( ${HPCC_MATURITY} STREQUAL "release" )
    SET (DOC_VERSION "${DOC_VERSION}.${HPCC_SEQUENCE}")
ELSE()
    SET (DOC_VERSION "${DOC_VERSION}.${WSSQL_SEQUENCE}${WSSQL_MATURITY}")
ENDIF()

# Build image file list to add to source watch.
FILE(GLOB_RECURSE DOC_IMAGES_LIST images/*)

# Required directories to add.
add_subdirectory(${HPCC_DOCS_DIR}/common ${CMAKE_CURRENT_BINARY_DIR}/common)
add_subdirectory(${HPCC_DOCS_DIR}/BuildTools ${CMAKE_CURRENT_BINARY_DIR}/BuildTools)
add_subdirectory(${HPCC_DOCS_DIR}/resources ${CMAKE_CURRENT_BINARY_DIR}/resources)

# Doc dirs to include.
add_subdirectory(WsSQLUG)

GET_PROPERTY(_targets GLOBAL PROPERTY DOC_TARGETS)
add_custom_target(docs ALL DEPENDS ${_targets})
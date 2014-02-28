HPCC SYSTEMS software Copyright (C) 2014 HPCC Systems.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

http://hpccsystems.com

wssql
=====
SQL interface to HPCC engines
#########################################################

To build for Linux:
   1. Check out source hpcc-systems/wssql
   2. Check out pre-req source hpcc-systems/HPCC-Platform (check ~/wssql/platform-version-prereq.cmake for minimum version required)
   3. Setup Antlr and Antlr3c - Downloads available from ANTLRS github page https://github.com/antlr/website-antlr3/tree/gh-pages/download
    a. "antlr-${ANTLR_VER}-complete.jar" and "antlr-runtime-${ANTLR_VER}.jar" are required.
    b. Build process looks for these files here: /usr/local/ANTLR/${ANTLR_VER} where ANTLR_VER = 3.4
    c. "antlr-${ANTLR_VER}-complete.jar" is a build time dep
    e. "antlr-runtime-${ANTLR_VER}.jar" is a runtime dep (extract its contents in /usr/local/ANTLR/${ANTLR_VER})
    f. If building a distributable package, make sure to include ANTLR's license file: https://raw2.github.com/antlr/antlr4/master/LICENSE.txt in the ANTLR folder
   4. Create a build directory - preferably at the same level as the two source dirs "wssql and HPCC-Platform"
   5. cd to the build directory
   6a.To create makefiles to build native release version for local machine, run
       cmake ~/wssql
   6b.To create makefiles to build native debug version, run
       cmake -DCMAKE_BUILD_TYPE=Debug ~/wssql
   7. To build the makefiles just created above, run
       make

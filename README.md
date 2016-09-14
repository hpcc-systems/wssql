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

#wssql
=====
##SQL interface to HPCC engines
=====
###Install packages and documentation can be downloaded from the hpccsystems.com portal
=====

Steps for building in Linux:

- Check out source hpcc-systems/wssql
- Check out pre-req source hpcc-systems/HPCC-Platform
  - check ~/wssql/platform-version-prereq.cmake for minimum version required
- Setup Antlr and Antlr3c - Downloads available from ANTLR's github page https://github.com/antlr/website-antlr3/tree/gh-pages/download
  - "antlr-${ANTLR_VER}-complete.jar" and "antlr-runtime-${ANTLR_VER}.jar" are required.
  - Build process looks for these files here: /usr/local/ANTLR/${ANTLR_VER} where ANTLR_VER = 3.4
  - "antlr-${ANTLR_VER}-complete.jar" is a build time dep
  - "antlr-runtime-${ANTLR_VER}.jar" is also a build time dep (extract its contents in /usr/local/ANTLR/${ANTLR_VER})
  - If building a distributable package, make sure to include ANTLR's license file: https://raw2.github.com/antlr/antlr4/master/LICENSE.txt in the ANTLR folder and abide by their license. 
- Create a build directory - preferably at the same level as the two source dirs "wssql and HPCC-Platform"
- cd to the build directory
- Create the CMAKE project
  - To create makefiles to build native release version for local machine, run: cmake ~/wssql
  - To create makefiles to build native debug version, run: cmake -DCMAKE_BUILD_TYPE=Debug ~/wssql
- To build the makefiles just created above, run: make
- To create a build package, run: make package


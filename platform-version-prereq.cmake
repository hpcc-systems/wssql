###
## This file describes the required HPCC build version.
## The WSSQL ESP plug-in must be built against an HPCC 
## source tree of at least this version: 
###

set ( HPCC_PROJECT_REQ "community" )
set ( HPCC_MAJOR_REQ 6 )
set ( HPCC_MINOR_REQ 0 )
set ( HPCC_POINT_REQ 0 )
set ( HPCC_MATURITY_REQ "trunk" )
set ( HPCC_SEQUENCE_REQ 1 )
###

##List of HPCC Changes which directly affect this plug-in:

#JIRA NUMBER | PULL REQ | HPCC Branch | Merge Date     | DESCRIPTION
#HPCC-10826  | 5392     | master      | March 10, 2014 | WSWORKUNITS source logic visibility commit 3eb3efa059931abbd6c0b46e8c1bc4ea84756352
#                                                                                            commit ae3e8d86abd21952fa1239f4cc34b3c18e47ccd0
#HPCC-10699  | 5390     | master      | March 6, 2014  | Configmgr plug-in support changes   commit 6444fa5940cd33325705f6d656b092d8d14c69b7
#HPCC-10837  | 5389     | master      | Feb 21, 2014   | WsWorkunit(zlib) buildbreak issue   commit 5755ec885b293d99e5fb4616838d34129496c677
#HPCC-9401   | 5499     | master      | April 25, 2014 | WsEcl wuinfo members now private    commit ac38c6c3cc0fd0c0b0a8de97693727ca2ec58850

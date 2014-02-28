###
## This file describes the required HPCC build version.
## The WSSQL ESP plug-in must be built against an HPCC 
## source tree of at least this version: 
###
set ( HPCC_PROJECT_REQ "community" )
set ( HPCC_MAJOR_REQ 4 )
set ( HPCC_MINOR_REQ 3 )
set ( HPCC_POINT_REQ 0 )
set ( HPCC_MATURITY_REQ "trunk" )
set ( HPCC_SEQUENCE_REQ 1 )
###

##List of HPCC Changes which directly affect this plug-in:

#JIRA NUMBER | GITHUB PULL REQ | HPCC Ver | DESCRIPTION
#HPCC-10826  |                            | WSWORKUNITS source logic visibility
#HPCC-10699  |                            | Configmgr plug-in support changes

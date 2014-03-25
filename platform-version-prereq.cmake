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

#JIRA NUMBER | PULL REQ | HPCC Branch | Merge Date    | DESCRIPTION
#HPCC-10826  | 5392     | master      | March 7, 2014 | WSWORKUNITS source logic visibility
#HPCC-10699  | 5390     | master      | March 6, 2014 | Configmgr plug-in support changes
#HPCC-10837  | 5389     | master      | Feb 21, 2014  | Fixed buildbreak issue

# Install script for directory: E:/opencv/rtp/jrtplib-3.9.1/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "D:/qt/rtp/rtp/rtp/jrtplib-3.9.1/build")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/jrtplib3" TYPE FILE FILES
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtcpapppacket.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtcpbyepacket.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtcpcompoundpacket.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtcpcompoundpacketbuilder.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtcppacket.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtcppacketbuilder.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtcprrpacket.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtcpscheduler.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtcpsdesinfo.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtcpsdespacket.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtcpsrpacket.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtcpunknownpacket.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpaddress.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpcollisionlist.h"
    "D:/qt/rtp/rtp/rtp/jrtplib-3.9.1/build/src/rtpconfig.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpdebug.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpdefines.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtperrors.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtphashtable.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpinternalsourcedata.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpipv4address.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpipv4destination.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpipv6address.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpipv6destination.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpkeyhashtable.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtplibraryversion.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpmemorymanager.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpmemoryobject.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtppacket.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtppacketbuilder.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtppollthread.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtprandom.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtprandomrand48.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtprandomrands.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtprandomurandom.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtprawpacket.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpsession.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpsessionparams.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpsessionsources.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpsourcedata.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpsources.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpstructs.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtptimeutilities.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtptransmitter.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtptypes_win.h"
    "D:/qt/rtp/rtp/rtp/jrtplib-3.9.1/build/src/rtptypes.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpudpv4transmitter.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpudpv6transmitter.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpbyteaddress.h"
    "E:/opencv/rtp/jrtplib-3.9.1/src/rtpexternaltransmitter.h"
    )
endif()

if(NOT CMAKE_INSTALL_COMPONENT OR "${CMAKE_INSTALL_COMPONENT}" STREQUAL "Unspecified")
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "D:/qt/rtp/rtp/rtp/jrtplib-3.9.1/build/lib/jrtplib_d.lib")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "D:/qt/rtp/rtp/rtp/jrtplib-3.9.1/build/lib" TYPE STATIC_LIBRARY FILES "D:/qt/rtp/rtp/rtp/jrtplib-3.9.1/build/src/jrtplib_d.lib")
endif()


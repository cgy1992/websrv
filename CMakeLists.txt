cmake_minimum_required(VERSION 2.6)

set (PROJECT websrv)
project (${PROJECT})

message("${CMAKE_SYSTEM_NAME}")
message("${CMAKE_CURRENT_SOURCE_DIR}")

SET(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} "-fpermissive")

set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}")

FIND_PACKAGE(Lua 5.2)
if (LUA_FOUND)
  message("Lua was found!")
  message(${LUA_INCLUDE_DIR})
  message(${LUA_LIBRARY})
endif (LUA_FOUND)

file (GLOB FILES
	"*.h*"
	"*.c*"
	"libwebserver/*.h*"
	"libwebserver/*.c*"
)

IF( NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )
	list(REMOVE_ITEM FILES ${CMAKE_CURRENT_SOURCE_DIR}/libwebserver/dirent.c)
	list(REMOVE_ITEM FILES ${CMAKE_CURRENT_SOURCE_DIR}/libwebserver/dirent.h)
ENDIF( NOT ${CMAKE_SYSTEM_NAME} MATCHES "Windows" )

include_directories(libwebserver)
include_directories(${LUA_INCLUDE_DIR})

add_library(${PROJECT} SHARED ${FILES})

target_link_libraries (${PROJECT} ${LUA_LIBRARY})



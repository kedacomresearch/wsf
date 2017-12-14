cmake_minimum_required( VERSION 3.0 )

project( wsf )

set(VERSION 0.2.0)

option( ENABLE_ACTOR     "build actor model " ON)
		
include( AutoCMake )

autocmake_pkgconfig_init()
if( MSVC )
   add_definitions( -DNOMINMAX )
endif()

include_directories( ${CMAKE_SOURCE_DIR}/include )

include_directories( ${CMAKE_SOURCE_DIR}/transport/include )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} )		
set( TEST_WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${CMAKE_BUILD_TYPE} )


add_subdirectory( transport  )

if( ENABLE_ACTOR )
include_directories( ${CMAKE_SOURCE_DIR}/actor/include )
add_subdirectory( actor  )
endif()


INSTALL( DIRECTORY ${CMAKE_SOURCE_DIR}/include/wsf  DESTINATION include )


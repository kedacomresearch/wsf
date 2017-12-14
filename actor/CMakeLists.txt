project( wsf-actor CXX)


file( GLOB_RECURSE _SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/source/*.[ch]pp
                            ${CMAKE_CURRENT_SOURCE_DIR}/source/*.[ch] )
							
file( GLOB_RECURSE _HEADERS ${CMAKE_HOME_DIRECTORY}/include/wsf/actor/*.inl 
                            ${CMAKE_HOME_DIRECTORY}/include/wsf/actor/*.h 
                            ${CMAKE_HOME_DIRECTORY}/include/wsf/actor/*.hpp)

list(APPEND _FILES ${_SOURCES} ${_HEADERS})
autocmake_msvc_project_group( _FILES ${CMAKE_CURRENT_SOURCE_DIR})


					 
autocmake_add_library(wsf-actor STATIC 
					  SOURCES ${_FILES}
					  MODULES theron log4cplus libevent libwebsockets libuv
                     )

install(TARGETS wsf-actor
        ARCHIVE DESTINATION lib 
        LIBRARY DESTINATION lib )


configure_file(wsf-actor.pc.cmakein ${CMAKE_BINARY_DIR}/wsf-actor.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/wsf-actor.pc DESTINATION lib/pkgconfig)

add_subdirectory(testkits)
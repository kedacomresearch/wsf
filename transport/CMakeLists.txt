set(PROJECT_NAME wsf-transport)


#---------------------------fixed----------------------
project(${PROJECT_NAME})

set(PROJECT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_DIRECTORIES( ${PROJECT_DIRECTORY}/source )
INCLUDE_DIRECTORIES( ${PROJECT_DIRECTORY}/include )
INCLUDE_DIRECTORIES( ${PROJECT_DIRECTORY}/../include )

FILE(GLOB_RECURSE SRC_FILES ${PROJECT_DIRECTORY}/source/*.c*
${PROJECT_DIRECTORY}/source/*.h*)
FILE(GLOB_RECURSE INC_FILES ${PROJECT_DIRECTORY}/include/*.h*
${PROJECT_DIRECTORY}/include/*.c* 
${PROJECT_DIRECTORY}/include/*.i*)
FILE(GLOB_RECURSE INC_FILES_COMM ${PROJECT_DIRECTORY}/../include/*.h*
${PROJECT_DIRECTORY}/include/*.c* 
${PROJECT_DIRECTORY}/include/*.i*)
set( SOURCE_FILES ${SRC_FILES} ${INC_FILES} ${INC_FILES_COMM})

autocmake_msvc_project_group( SOURCE_FILES ${PROJECT_DIRECTORY})
#---------------------------fixed----------------------

autocmake_add_library(${PROJECT_NAME} STATIC 
                      MODULES libevent libwebsockets libuv log4cplus
                      SOURCES ${SOURCE_FILES} )

# target_link_libraries( ${PROJECT_NAME} asruntime)

install(TARGETS ${PROJECT_NAME} 
        ARCHIVE DESTINATION lib
		LIBRARY DESTINATION lib )
add_subdirectory(testkits)
install(DIRECTORY include/wsf DESTINATION include)


configure_file(wsf-transport.pc.cmakein ${CMAKE_BINARY_DIR}/wsf-transport.pc @ONLY)
install(FILES ${CMAKE_BINARY_DIR}/wsf-transport.pc DESTINATION lib/pkgconfig)
# add_dependencies( ${PROJECT_NAME} asruntime )
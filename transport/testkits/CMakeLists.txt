set(PROJECT_NAME wsf-transport-unittest)
set(VERSION 0.1.0)
#---------------------------fixed----------------------
project(${PROJECT_NAME})

set(PROJECT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

INCLUDE_DIRECTORIES( ${PROJECT_DIRECTORY}/source )
INCLUDE_DIRECTORIES( ${PROJECT_DIRECTORY}/include )

FILE(GLOB_RECURSE SRC_FILES ${PROJECT_DIRECTORY}/unittest/*.c*
${PROJECT_DIRECTORY}/unittest/*.h*)
FILE(GLOB_RECURSE INC_FILES ${PROJECT_DIRECTORY}/unittest/*.h*
${PROJECT_DIRECTORY}/unittest/*.c* 
${PROJECT_DIRECTORY}/unittest/*.i*)
set( SOURCE_FILES ${SRC_FILES} ${INC_FILES})

autocmake_msvc_project_group( SOURCE_FILES ${PROJECT_DIRECTORY})
#---------------------------fixed----------------------

autocmake_check_modules(this 
						log4cplus
						libevent
						libwebsockets
						libuv
						theron						
						)

include_directories( ${this_INCLUDE_DIRECTORIES} )
link_directories( ${this_LINK_DIRECTORIES} )

add_executable (${PROJECT_NAME} ${SOURCE_FILES})
target_link_libraries(${PROJECT_NAME} wsf-transport ${this_LIBS})
if( MSVC )
target_link_libraries(${PROJECT_NAME} ws2_32 )
endif()
# install(TARGETS ${PROJECT_NAME}
#         RUNTIME DESTINATION test/SCiF)

add_dependencies(${PROJECT_NAME} wsf-transport )

add_test(NAME transport-test
         COMMAND nosetests -w ${TEST_WORKING_DIRECTORY} -w ${CMAKE_CURRENT_SOURCE_DIR}/nose --with-html-output --html-out-file=${TEST_WORKING_DIRECTORY}/wsf-transport-unittest.html
		 )

install(TARGETS wsf-transport-unittest
		RUNTIME DESTINATION testkits/wsf/nose/transport
		)
install(DIRECTORY   nose/ 
        DESTINATION testkits/wsf/nose/transport
		FILES_MATCHING PATTERN "*.py")



project( wsf-actor-testkit CXX)

# Calc to be OBSOLETED
#set(CALC_SOURCES)
#file(GLOB_RECURSE  CALC_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/calc/*.[ch]pp)
#
#autocmake_add_add_executable(wsf-actor-calc
#					  SOURCES ${CALC_SOURCES}
#					  MODULES theron log4cplus libevent 
#                     )
#target_link_libraries(wsf-actor-calc ws2_32.lib wsf-transport wsf-actor)
#install(TARGETS wsf-actor-calc
#		RUNTIME DESTINATION testkits/wsf
#		)
#add_dependencies(wsf-actor-calc wsf-transport wsf-actor)		
#
# unittest
#


file(GLOB_RECURSE  UNITTEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/unittest/*.[ch]pp)
autocmake_msvc_project_group( UNITTEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/unittest)
autocmake_add_add_executable(wsf-actor-unittest
					  SOURCES ${UNITTEST_SOURCES}
					  MODULES theron log4cplus libevent libwebsockets libuv
					 )
autocmake_check_modules(this 
					 log4cplus
					 theron						
					 libevent
					 libwebsockets
					 libuv
					 )

target_link_libraries(wsf-actor-unittest  wsf-actor wsf-transport ${this_LIBS})
if(MSVC)
target_link_libraries(wsf-actor-unittest ws2_32 )
endif()

add_test(NAME actor-test
         COMMAND nosetests -w ${TEST_WORKING_DIRECTORY} -w ${CMAKE_CURRENT_SOURCE_DIR}/nose --with-html-output --html-out-file=${TEST_WORKING_DIRECTORY}/wsf-actor-unittest.html
		 )
		 
install(TARGETS wsf-actor-unittest
		RUNTIME DESTINATION testkits/wsf/nose/actor
		)
install(DIRECTORY   nose/ 
        DESTINATION testkits/wsf/nose/actor
		FILES_MATCHING PATTERN "*.py")
		
add_dependencies(wsf-actor-unittest wsf-transport wsf-actor)
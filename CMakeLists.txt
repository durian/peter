cmake_minimum_required(VERSION 2.8 FATAL_ERROR)
project(peter)

add_definitions(-DVERSION="1.1.6")

include(FindEXPAT)
find_package(EXPAT REQUIRED)
include_directories(${EXPAT_INCLUDE_DIRS})

find_package(Threads)
#target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})

add_executable(${PROJECT_NAME} src/Actions.cc src/Scheduler.cc src/otp.cc src/Config.cc src/Socket.cc src/debug.cc src/Events.cc src/WorkUnit.cc src/expat_parser.cc src/qlog.cc src/Program.cc src/XMLNode.cc src/main.cc src/util.cc)

add_executable(pcmd src/cmd.cc)
add_executable(otpcmd src/otp.cc src/util.cc src/XMLNode.cc src/expat_parser.cc src/otpcmd.cc)

#file(GLOB SOURCES "src/*.cc")
#file(GLOB HEADERS "src/*.h") 
#add_executable(peter ${SOURCES} ${HEADERS})

target_link_libraries(${PROJECT_NAME} ${EXPAT_LIBRARIES})
target_link_libraries(otpcmd ${EXPAT_LIBRARIES})

target_link_libraries(${PROJECT_NAME} pthread)

#CMakeLists.txt -> EasyGpp

cmake_minimum_required (VERSION 3.1) 
project(EasyGpp CXX)

set(CMAKE_BUILD_TYPE Release)
set(SOURCE_BASE /opt/GitHub/EasyGpp)
set(CMAKE_CXX_STANDARD 14)

include_directories("${SOURCE_BASE}/include")

set (EASYGPP_SOURCES "${SOURCE_BASE}/src/easygpp.cpp")

add_executable(easyg++ ${EASYGPP_SOURCES})
target_link_libraries(easyg++ tjlutils pthread)

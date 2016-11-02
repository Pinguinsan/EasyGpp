#CMakeLists.txt -> EasyGpp

cmake_minimum_required (VERSION 3.1) 
project(EasyGpp CXX)

set(CMAKE_BUILD_TYPE Release)
set(SOURCE_BASE /opt/GitHub/EasyGpp)
set(CMAKE_CXX_STANDARD 14)
set(GCC_ADDITIONAL_COMPILE_FLAGS "-fsanitize=undefined -frecord-gcc-switches")
#set(GCC_ADDITIONAL_COMPILE_FLAGS "-frecord-gcc-switches")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GCC_ADDITIONAL_COMPILE_FLAGS}")

include_directories("${SOURCE_BASE}/include")

set (EASYGPP_SOURCES "${SOURCE_BASE}/src/easygpp.cpp")

add_executable(easyg++ ${EASYGPP_SOURCES})
target_link_libraries(easyg++ tjlutils pthread)

cmake_minimum_required(VERSION 3.22)

project(test CXX)

set(CMAKE_CXX_COMPILER /opt/hisi-linux/x86-arm/arm-himix200-linux/bin/arm-himix200-linux-g++)

set(SRC_DIRS
    ${PROJECT_SOURCE_DIR}/src/vi
    ${PROJECT_SOURCE_DIR}/inc
    ${PROJECT_SOURCE_DIR}/src/vpss
    ${PROJECT_SOURCE_DIR}/src/vo
)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_executable(test 
    src/test.cpp 
    src/vi/vi.cpp
    src/vpss/vpss.cpp
    src/vo/vo.cpp
)

target_link_libraries(test 
    securec 
    pthread
    mpi 
    upvqe 
    VoiceEngine 
    dnvqe
    _hicalcflicker  
    _hildci         
    _hidehaze       
    _hidrc          
    isp
    _hiawb
    _hiae 
    sns_gc2053
    hdmi
)

include_directories(${SRC_DIRS})
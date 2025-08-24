cmake_minimum_required(VERSION 3.22)

project(test CXX)

set(CMAKE_CXX_COMPILER /opt/hisi-linux/x86-arm/arm-himix200-linux/bin/arm-himix200-linux-g++)

set(SRC_DIRS
    ${PROJECT_SOURCE_DIR}/src/vi
    ${PROJECT_SOURCE_DIR}/inc
)
link_directories(${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_executable(test test.cpp 
                    src/vi/vi.cpp)

target_link_libraries(test mpi securec upvqe VoiceEngine dnvqe pthread)

include_directories(${SRC_DIRS})

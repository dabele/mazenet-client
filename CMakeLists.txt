project(mazenet-client)
cmake_minimum_required(VERSION 2.8)

set(XSD_DIR "C:/Program Files (x86)/CodeSynthesis XSD 3.3" CACHE PATH "path to codesynthesis xsd root")
include_directories(${XSD_DIR}/include)
link_directories(${XSD_DIR}/lib/vc-10.0)

set(BOOST_DIR "D:/boost/boost_1_53_0" CACHE PATH "path to boost (including system, thread, regex, date_time, serialization")
include_directories(${BOOST_DIR})
link_directories(${BOOST_DIR}/lib)

add_executable(mazenet-client main.cpp client.hpp client.cpp mazeCom.hpp mazeCom.cpp)
target_link_libraries(mazenet-client debug xerces-c_3D.lib)
target_link_libraries(mazenet-client optimized xerces-c_3.lib)

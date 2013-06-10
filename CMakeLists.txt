project(mazenet-client)
cmake_minimum_required(VERSION 2.8)

set(XSD_DIR "C:/Program Files (x86)/CodeSynthesis XSD 3.3" CACHE PATH "path to codesynthesis xsd root")
include_directories(${XSD_DIR}/include)
link_directories(${XSD_DIR}/lib/vc-10.0)

set(BOOST_DIR "D:/boost/boost_1_53_0" CACHE PATH "path to boost (including system, thread, regex, date_time, serialization")
include_directories(${BOOST_DIR})
link_directories(${BOOST_DIR}/lib)

set(XSD_FILE "mazeCom.xsd" CACHE PATH "xml schema definition (absolute path or relative path from binary dir")

set(XSD_COMMAND 
	COMMAND xsd cxx-tree --generate-serialization --generate-ostream --output-dir "." 
			--hxx-suffix ".hpp" --cxx-suffix ".cpp" --ixx-suffix ".ipp" --fwd-suffix "-fwd.hpp" "${XSD_FILE}"
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR})	
execute_process(${XSD_COMMAND} RESULT_VARIABLE XSD_RETURN OUTPUT_VARIABLE XSD_MESSAGE ERROR_VARIABLE XSD_MESSAGE)
if(XSD_RETURN)
	message(FATAL_ERROR ${XSD_MESSAGE})
endif(XSD_RETURN)
include_directories(${CMAKE_BINARY_DIR})
	
add_executable(mazenet-client main.cpp client.hpp client.cpp mazeCom_.hpp mazeCom.hpp mazeCom.cpp)
target_link_libraries(mazenet-client debug xerces-c_3D.lib)
target_link_libraries(mazenet-client optimized xerces-c_3.lib)

add_custom_command(TARGET mazenet-client PRE_BUILD ${XSD_COMMAND} COMMENT "generating MazeCom sources" VERBATIM)
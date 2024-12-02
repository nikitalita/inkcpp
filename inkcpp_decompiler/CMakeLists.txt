cmake_minimum_required(VERSION 3.10)
project(inkcpp_decompiler)

# Set the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Source files
set(SOURCES
    decompiler.cpp
    reverse_compiler.cpp
    json_emitter.cpp
	instruction_reader.cpp
	binstr_modifier.cpp
)

# Header files
set(HEADERS
    include/decompiler.h
    reverse_compiler.h
    json_emitter.h
	instruction_reader.h
	binstr_modifier.h
	args.h
)

# Add the library
add_library(inkcpp_decompiler_o OBJECT ${SOURCES} ${HEADERS})
add_library(inkcpp_decompiler_lib $<TARGET_OBJECTS:inkcpp_decompiler_o>)
set_target_properties(inkcpp_decompiler_lib PROPERTIES OUTPUT_NAME inkcpp_decompiler)

add_executable(inkcpp_bineditor main.cpp $<TARGET_OBJECTS:inkcpp_decompiler_o>)

target_include_directories(inkcpp_decompiler_o PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

target_include_directories(inkcpp_decompiler_lib PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)
# Include directories
target_include_directories( 
    inkcpp_decompiler_o
    PRIVATE
    ${CMAKE_SOURCE_DIR}/inkcpp
    ${CMAKE_SOURCE_DIR}/inkcpp_compiler
    ${CMAKE_SOURCE_DIR}/shared/public 
    ${CMAKE_SOURCE_DIR}/shared/private

)
target_include_directories( 
    inkcpp_decompiler_lib
    PRIVATE
    ${CMAKE_SOURCE_DIR}/inkcpp
    ${CMAKE_SOURCE_DIR}/inkcpp_compiler
    ${CMAKE_SOURCE_DIR}/shared/public 
    ${CMAKE_SOURCE_DIR}/shared/private
)

target_include_directories( 
	inkcpp_bineditor
	PRIVATE
	${CMAKE_SOURCE_DIR}/inkcpp
	${CMAKE_SOURCE_DIR}/inkcpp_compiler
	${CMAKE_SOURCE_DIR}/shared/public 
	${CMAKE_SOURCE_DIR}/shared/private
)


FILE(GLOB PUBLIC_HEADERS "include/*")
set_target_properties(inkcpp_decompiler_lib PROPERTIES PUBLIC_HEADER "${PUBLIC_HEADERS}")


# Make sure this project and all dependencies use the C++17 standard
target_compile_features(inkcpp_decompiler_o PUBLIC cxx_std_17)
target_compile_features(inkcpp_decompiler_lib PUBLIC cxx_std_17)
target_compile_features(inkcpp_bineditor PUBLIC cxx_std_17)

message(INFO "${CMAKE_SOURCE_DIR}")
# Link to inkcpp
target_link_libraries(inkcpp_decompiler_o PRIVATE inkcpp_shared inkcpp inkcpp_compiler)
target_link_libraries(inkcpp_decompiler_lib PRIVATE inkcpp_shared inkcpp inkcpp_compiler)
target_link_libraries(inkcpp_bineditor PRIVATE inkcpp_decompiler_lib inkcpp_shared inkcpp inkcpp_compiler)

cmake_minimum_required(VERSION 2.6)
  
project(DSC_Mixer)

option(BUILD_OGL43 "Bild OpenGL 4.3 examples" ON)
mark_as_advanced(BUILD_OGL43)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/CMakeModules/)

find_package(OpenGL REQUIRED)
find_package(FMOD REQUIRED)
set( BOOST_ROOT "C:/local/boost_1_61_0/" )
set( BOOST_LIBRARYDIR "C:/local/boost_1_61_0/lib64-msvc-14.0/" )
message( " NOTE: SETTING HARD PATHS TO BOOST LOCATIONS " )
message( "   BOOST_ROOT ${BOOST_ROOT} " )
message( "   BOOST_LIBRARYDIR ${BOOST_LIBRARYDIR} " )
find_package( Boost REQUIRED filesystem )

add_subdirectory(glfw)
add_subdirectory(glxw)

include_directories(glfw/include)
include_directories(glm)
include_directories(${CMAKE_BINARY_DIR}/glxw/include)
include_directories(${FMOD_INCLUDE_DIRS})
include_directories(${Boost_INCLUDE_DIRS})

add_definitions( ${Boost_DEFINITIONS} )
 
SET(LIBRARIES glfw glxw ${Boost_LIBRARIES} ${GLFW_LIBRARIES} ${GLXW_LIBRARY} ${OPENGL_LIBRARY} ${CMAKE_DL_LIBS} ${FMOD_LIBRARIES} )

link_directories (${OPENGLEXAMPLES_BINARY_DIR}/bin)
 
add_executable (Mixer Mixer.cpp)
target_link_libraries(09transform_feedback ${LIBRARIES} )


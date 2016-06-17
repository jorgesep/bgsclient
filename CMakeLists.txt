cmake_minimum_required(VERSION 2.8)

project(bgsclient)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++0x")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99")

IF(UNIX)
  # add some standard warnings
  ADD_DEFINITIONS(-Wno-variadic-macros -Wno-long-long -Wall -Wextra -Winit-self -Woverloaded-virtual -Wsign-promo -Wno-unused-parameter -pedantic -Woverloaded-virtual -Wno-unknown-pragmas)
  
  # -ansi does not compile with sjn module
  #ADD_DEFINITIONS(-ansi)

  # if you like to have warinings about conversions, e.g. double->int or double->float etc., or float compare
  #ADD_DEFINITIONS(-Wconversion -Wfloat-equal)
endif(UNIX)


find_package( Boost REQUIRED COMPONENTS filesystem system regex date_time)
include_directories( ${Boost_INCLUDE_DIRS} )

find_package(OpenCV REQUIRED)

message(STATUS "OpenCV library status:")
message(STATUS "    version: ${OpenCV_VERSION}")
message(STATUS "    libraries: ${OpenCV_LIBS}")
message(STATUS "    include path: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "    HOME ENV: $ENV{HOME}")
message(STATUS "    HOME ENV: ${PROJECT_SOURCE_DIR}")

if(${OpenCV_VERSION} VERSION_EQUAL 3 OR ${OpenCV_VERSION} VERSION_GREATER 3)
  message (FATAL_ERROR "OpenCV version is not compatible: ${OpenCV_VERSION}")
endif()

if(${OpenCV_VERSION} VERSION_LESS 2.3.1)
  message (FATAL_ERROR "OpenCV version is not compatible: ${OpenCV_VERSION}")
endif()

file(GLOB imbs ${PROJECT_SOURCE_DIR}/src/Main.cpp)
FILE ( GLOB LIBS ${PROJECT_SOURCE_DIR}/src/IMBSBuilder.cpp ${PROJECT_SOURCE_DIR}/src/IMBSBuilder.h )
FILE ( GLOB SCRIPTS ${PROJECT_SOURCE_DIR}/*.py ${PROJECT_SOURCE_DIR}/*.sh )

# Find custom library location of bgs and bgslibrary 
set(LOCAL_PATH "$ENV{HOME}/local")
set(CUSTOM_LIB  ${LOCAL_PATH}/lib)
set(CUSTOM_INC  ${LOCAL_PATH}/include)
set(CUSTOM_BIN  ${LOCAL_PATH}/bin)
find_library(BASE_SYSTEM  NAMES BaseSystem  HINTS ${CUSTOM_LIB})
find_library(FRAME_READER NAMES FrameReader HINTS ${CUSTOM_LIB})
find_library(IMAGE_UTILS  NAMES ImageUtils  HINTS ${CUSTOM_LIB})
find_library(UTILS        NAMES Utils       HINTS ${CUSTOM_LIB})
find_library(TIMER        NAMES Timer       HINTS ${CUSTOM_LIB})
find_library(BGS_LIB      NAMES bgs         HINTS ${CUSTOM_LIB})

INCLUDE_DIRECTORIES(${CUSTOM_INC}/bgs ${CUSTOM_INC}/package_bgs)

ADD_LIBRARY( IMBSBuilder SHARED ${LIBS})
target_link_libraries( IMBSBuilder ${BGS_LIB} ${UTILS} ${OpenCV_LIBS} ${Boost_LIBRARIES} ${TIMER} )
set_property(TARGET IMBSBuilder PROPERTY LIBRARY_OUTPUT_DIRECTORY ${bgsclient_BINARY_DIR}/lib)

add_executable(bgs_imbs ${imbs})
target_link_libraries(bgs_imbs ${BASE_SYSTEM} IMBSBuilder ${FRAME_READER} ${IMAGE_UTILS} ${OpenCV_LIBS})
set_property(TARGET bgs_imbs PROPERTY RUNTIME_OUTPUT_DIRECTORY ${bgsclient_BINARY_DIR}/bin)

#file(COPY ${PROJECT_SOURCE_DIR}/../config DESTINATION ${BGS_BINARY_DIR}/)
#file(COPY ${SCRIPTS} DESTINATION ${BGS_BINARY_DIR}/bin/)

INSTALL(TARGETS bgs_imbs IMBSBuilder 
  RUNTIME DESTINATION bin COMPONENT app
  LIBRARY DESTINATION lib COMPONENT runtime
  ARCHIVE DESTINATION lib COMPONENT runtime
  PUBLIC_HEADER DESTINATION include/package_bgs COMPONENT dev
  FRAMEWORK DESTINATION "/Library/Frameworks"
)

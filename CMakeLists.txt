cmake_minimum_required(VERSION 2.8)

project(NormaChain)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set (CMAKE_RUNTIME_OUTPUT_DIRECTORY "../../")

######### finding OpenSSL Libraries #####
find_package ( OpenSSL REQUIRED )

if(OPENSSL_FOUND)
    message(STATUS "Found OpenSSL (include: ${OPENSSL_INCLUDE_DIR}, library: ${OPENSSL_LIBRARIES})")
else()
    message(STATUS "Package 'OpenSSL' not found")
endif()
##############

############# finding Boost Libraries ######
find_package(Boost REQUIRED COMPONENTS system thread serialization)

if(Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  link_directories(${Boost_LIBRARY_DIRS})
else()
    message(STATUS "Package 'Boost' not found")
endif(Boost_FOUND)
##############

############# findning PBC libraries #######
find_package(PBC REQUIRED )
###########################################

add_subdirectory (src)

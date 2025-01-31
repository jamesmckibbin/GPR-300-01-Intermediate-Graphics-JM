cmake_minimum_required (VERSION 3.8)

project ("src")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/")
set(SDL3_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/include/")
set(DIRECTX12_HEADERS_DIR "${CMAKE_CURRENT_LIST_DIR}/include/DirectX12/directx")
set(DXGUID_HEADERS_DIR "${CMAKE_CURRENT_LIST_DIR}/include/DirectX12/dxguids")

find_package(SDL3 REQUIRED)
include_directories(${SDL3_INCLUDE_DIRS})
include_directories(${DIRECTX12_HEADERS_DIR})
include_directories(${DXGUID_HEADERS_DIR})

add_executable (src "main.cpp" )

target_link_libraries(src ${SDL3_LIBRARIES})

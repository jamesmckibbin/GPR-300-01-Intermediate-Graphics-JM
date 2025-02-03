cmake_minimum_required (VERSION 3.8)

project ("DirectXPurgatory")

add_subdirectory(src)
add_subdirectory(libs)

set(SDL_INCLUDES "libs/SDL3/include")
set(SDL_LIBRARY_PATH "libs/SDL3/")
set(DIRECTX_INCLUDES "libs/DirectX12/include/directx/")
set(DXGUID_INCLUDES "libs/DirectX12/include/dxguid/")

add_executable(app ${SOURCES} ${HEADERS})

target_include_directories(app SYSTEM PRIVATE ${SDL_INCLUDES})
target_include_directories(app SYSTEM PRIVATE ${DIRECTX_INCLUDES})
target_include_directories(app SYSTEM PRIVATE ${DXGUID_INCLUDES})

target_link_directories(app PUBLIC ${SDL_LIBRARY_PATH})
target_link_libraries(app PUBLIC d3d12)
target_link_libraries(app PUBLIC dxgi)
target_link_libraries(app PUBLIC d3dcompiler)
target_link_libraries(app PUBLIC dxguid)
target_link_libraries(app PUBLIC SDL3)

add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${PROJECT_SOURCE_DIR}/libs/SDL3/SDL3.dll"
        $<TARGET_FILE_DIR:app>)

add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        "${PROJECT_BINARY_DIR}/assets/")

add_custom_command(TARGET app POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/shaders"
        "${PROJECT_BINARY_DIR}/shaders/")

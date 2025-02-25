# Install script for directory: C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/out/install/x64-debug")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY FILES "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/out/build/x64-Debug/lib/DirectXTK12.lib")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/directxtk12/DirectXTK12-targets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/directxtk12/DirectXTK12-targets.cmake"
         "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/out/build/x64-Debug/libs/DirectXTK/CMakeFiles/Export/dc79b1416ca4e830525952dfb83a96c7/DirectXTK12-targets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/directxtk12/DirectXTK12-targets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/share/directxtk12/DirectXTK12-targets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/directxtk12" TYPE FILE FILES "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/out/build/x64-Debug/libs/DirectXTK/CMakeFiles/Export/dc79b1416ca4e830525952dfb83a96c7/DirectXTK12-targets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/directxtk12" TYPE FILE FILES "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/out/build/x64-Debug/libs/DirectXTK/CMakeFiles/Export/dc79b1416ca4e830525952dfb83a96c7/DirectXTK12-targets-debug.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/directxtk12" TYPE FILE FILES
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/BufferHelpers.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/CommonStates.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/DDSTextureLoader.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/DescriptorHeap.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/DirectXHelpers.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/Effects.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/EffectPipelineStateDescription.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/GeometricPrimitive.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/GraphicsMemory.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/Model.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/PostProcess.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/PrimitiveBatch.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/RenderTargetState.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/ResourceUploadBatch.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/ScreenGrab.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/SpriteBatch.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/SpriteFont.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/VertexTypes.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/WICTextureLoader.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/GamePad.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/Keyboard.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/Mouse.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/SimpleMath.h"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/SimpleMath.inl"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/libs/DirectXTK/Inc/Audio.h"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/directxtk12" TYPE FILE FILES
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/out/build/x64-Debug/libs/DirectXTK/directxtk12-config.cmake"
    "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/out/build/x64-Debug/libs/DirectXTK/directxtk12-config-version.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig" TYPE FILE FILES "C:/Dev/Repositories/GPR-300-01-Intermediate-Graphics-JM/out/build/x64-Debug/libs/DirectXTK/DirectXTK12.pc")
endif()


cmake_minimum_required(VERSION 3.12)
project(ummp LANGUAGES CXX)

if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
      "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,\
      $<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,\
      $<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo;MinSizeRel" CACHE STRING "" FORCE)
else()
  if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "RelWithDebInfo" CACHE STRING "Build type" FORCE)
  endif()
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug;Release;RelWithDebInfo;MinSizeRel")
endif()

file(GLOB_RECURSE PAYLOAD_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/payload/*.cc")
file(GLOB_RECURSE DRIVER_SOURCES  CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/driver/*.cc")

get_filename_component(UMMP_CONSTANT_HEADER "${CMAKE_SOURCE_DIR}/common/constant.hh" ABSOLUTE)

function(ummp_force_include target header)
  if (MSVC)
    target_compile_options(${target} PRIVATE "/FI${header}")
  else()
    target_compile_options(${target} PRIVATE -include "${header}")
  endif()
endfunction()

set(UMMP_INCLUDE_DIRS
  "${CMAKE_SOURCE_DIR}"
  "${CMAKE_SOURCE_DIR}/payload"
  "${CMAKE_SOURCE_DIR}/driver"
  "${CMAKE_SOURCE_DIR}/common"
)

foreach(dir RUNTIME LIBRARY ARCHIVE)
  set(CMAKE_${dir}_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<IF:$<STREQUAL:${dir},ARCHIVE>,lib,bin>")
endforeach()

function(ummp_hardening target)
  target_compile_definitions(${target} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG;DEBUG>
    $<$<CONFIG:Release,RelWithDebInfo,MinSizeRel>:NDEBUG>
  )
  target_compile_definitions(${target} PRIVATE WIN32_LEAN_AND_MEAN NOMINMAX)

  if (MSVC)
    target_compile_options(${target} PRIVATE
      /W4 /WX /permissive- /Zc:__cplusplus /sdl /EHsc
      $<$<CONFIG:Debug>:/Od;/Zi;/RTC1>
      $<$<CONFIG:Release>:/O2;/Oi;/DNDEBUG>
      $<$<CONFIG:RelWithDebInfo>:/O2;/Oi;/Zi;/DNDEBUG>
      $<$<CONFIG:MinSizeRel>:/O1;/DNDEBUG>
    )
    target_link_options(${target} PRIVATE
      $<$<CONFIG:Release>:/LTCG>
      $<$<CONFIG:RelWithDebInfo>:/LTCG>
    )
  else()
    target_compile_options(${target} PRIVATE
      -Werror -Wall -Wextra -Wpedantic
      -Wconversion -Wsign-conversion -Wshadow
      -Wformat=2 -Wundef -Wnon-virtual-dtor -Woverloaded-virtual
      -Wold-style-cast -Wdouble-promotion -Wnull-dereference
      -Wcast-align=strict
      $<$<CXX_COMPILER_ID:GNU>:-Wduplicated-cond -Wduplicated-branches>
      $<$<CONFIG:Debug>:-O0 -g>
      $<$<CONFIG:Release>:-O3 -DNDEBUG>
      $<$<CONFIG:RelWithDebInfo>:-O2 -g -DNDEBUG>
      $<$<CONFIG:MinSizeRel>:-Os -DNDEBUG>
    )
  endif()
endfunction()

add_library(ummp_payload SHARED ${PAYLOAD_SOURCES})
target_include_directories(ummp_payload PUBLIC ${UMMP_INCLUDE_DIRS})
set_target_properties(ummp_payload PROPERTIES POSITION_INDEPENDENT_CODE ON)
ummp_hardening(ummp_payload)

add_executable(ummp_driver WIN32 ${DRIVER_SOURCES})
target_include_directories(ummp_driver PRIVATE ${UMMP_INCLUDE_DIRS})
ummp_hardening(ummp_driver)

ummp_force_include(ummp_payload "${UMMP_CONSTANT_HEADER}")
ummp_force_include(ummp_driver "${UMMP_CONSTANT_HEADER}")
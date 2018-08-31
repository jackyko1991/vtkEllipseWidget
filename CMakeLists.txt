cmake_minimum_required(VERSION 2.8)
 
PROJECT(EllipseWidget)
 
find_package(VTK REQUIRED)

file(GLOB SRC *.c*)
file(GLOB HDR *.h*)
include(${VTK_USE_FILE})
include_directories(${CMAKE_SOURCE_DIR})

add_executable(EllipseWidget MACOSX_BUNDLE ${SRC} ${HDR})
 
if(VTK_LIBRARIES)
  target_link_libraries(EllipseWidget ${VTK_LIBRARIES})
else()
  target_link_libraries(EllipseWidget vtkHybrid vtkWidgets)
endif()
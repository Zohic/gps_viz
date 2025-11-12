cmake_minimum_required(VERSION 3.18)
include(CMakePrintHelpers)
include(FindOpenGL)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-D LINUX_OS ${CMAKE_CXX_FLAGS}")
set(CMAKE_C_FLAGS "-D LINUX_OS ${CMAKE_C_FLAGS}")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS}")

project(GPS
  VERSION 1.0
  DESCRIPTION "GPS visualization"     
  HOMEPAGE_URL "https://github.com/Zohic/" 
  LANGUAGES C CXX                      
)

set(APP_NAME "${PROJECT_NAME}_VISUALIZER")

#set(OPENGL_LIB_PATH "C:\\Program Files (x86)\\Windows Kits\\10\\Lib\\10.0.26100.0\\um\\x64\\OpenGL32.Lib")
#set(OPENGL_INCLUDE_PATH "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.26100.0\\um\\gl")

#IF(NOT DEFINED OPENGL_INCLUDE_PATH)
#	message(FATAL_ERROR "opengl include directory isnt specified")
#ENDIF()

#IF(NOT DEFINED OPENGL_LIB_PATH)
#	message(FATAL_ERROR "opengl lib directory isnt specified")
#ENDIF()

set(THIRD_PATH "${CMAKE_SOURCE_DIR}/third-party")

set(IMGUI_PATH "${THIRD_PATH}/imgui")
set(IMPLOT_PATH "${THIRD_PATH}/implot")
set(GLFW_PATH "${THIRD_PATH}/glfw")

find_package(OpenGL)

if(OpenGL_FOUND)
  message("found OpenGL")
  message("include dirs ${OPENGL_INCLUDE_DIR}")
  message(${OPENGL_LIBRARIES})
  message($<TARGET_PROPERTY:GL,INCLUDE_DIRECTORIES>)
  message("------------")
  #message($<TARGET_PROPERTY:OpenGL::GL,OPENGL_gl_LIBRARY>)
else()
  message("cant find opengl")
endif()

set(IMGUI_SOURCES 
"${IMGUI_PATH}/imgui.cpp"
"${IMGUI_PATH}/imgui_draw.cpp"
"${IMGUI_PATH}/imgui_widgets.cpp"
"${IMGUI_PATH}/imgui_demo.cpp"
"${IMGUI_PATH}/imgui_tables.cpp"
"${IMGUI_PATH}/imgui_impl_glfw.cpp"
"${IMGUI_PATH}/imgui_impl_opengl3.cpp")

set(IMPLOT_SOURCES 
"${IMPLOT_PATH}/implot.cpp"
"${IMPLOT_PATH}/implot_items.cpp"
"${IMPLOT_PATH}/implot_demo.cpp")

add_subdirectory(${GLFW_PATH})

set(APP_SOURCES main.cpp gps.cpp gps.h result.h)
set(DSLP_DIR "${CMAKE_SOURCE_DIR}/libdspl-2.0/_release")
set(DSPL_SOURCES 
"${DSLP_DIR}/dspl.c" 
"${DSLP_DIR}/dspl.h"
)


message("dspl sources")
message(${DSPL_SOURCES})
message(${DSPL_SOURCES})
message("----------------")

add_executable("${APP_NAME}")

target_link_libraries(${APP_NAME} glfw)

message("glfw includes: ${GLFW_INCLUDE_DIRS}")
target_sources("${APP_NAME}" PRIVATE 
	"${APP_SOURCES}"
	"${IMGUI_SOURCES}"
	"${IMPLOT_SOURCES}"
	"${DSPL_SOURCES}")

target_link_libraries("${APP_NAME}" "${GLFW_LIB}" "${OPENGL_LIBRARIES}")

target_include_directories("${APP_NAME}" PRIVATE 
"${CMAKE_SOURCE_DIR}/FFT/include"
"${BASE_PATH}"
"${IMGUI_PATH}"
"${IMPLOT_PATH}"
"${GLFW_PATH}"
"${GLFW_PATH}/include"
"${OPENGL_INCLUDE_PATH}"
"${DSLP_DIR}")

#add_subdirectory("./libdspl_built")
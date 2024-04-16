cmake_minimum_required(VERSION 3.25)

project(open_virtual_camera)

set(OpenCV_DIR "opencv/build")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include_directories(./include)
include_directories(${OpenCV_DIR})
include_directories(${OpenCV_DIR}/include)
include_directories(${OpenCV_DIR}/include/opencv2)

find_package(OpenCV REQUIRED)
message("### OPENCV libpath: ${OpenCV_LIB_PATH}")
include_directories(${OpenCV_INCLUDE_DIRS})

file(GLOB tp_files ${OpenCV_DIR}/x64/vc16/bin/opencv_videoio_msmf490_64.dll  ${OpenCV_DIR}/x64/vc16/bin/opencv_world490.dll)
foreach(tp_file ${tp_files})
	file(COPY ${tp_file} DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)
	file(COPY ${tp_file} DESTINATION ${CMAKE_BINARY_DIR}/Release/)
endforeach()
file(COPY camera_name.txt DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)
file(COPY install.bat DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)
file(COPY uninstall.bat DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)
file(COPY LICENSE.txt DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)
file(COPY README.md DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)
file(COPY win-dshow DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)

link_directories(${OpenCV_DIR}/libs)
file(GLOB src
     "src/*.c"
     "src/*.cpp"
)
add_executable(vcam ${src})
target_link_libraries(vcam ${OpenCV_LIBS})
set_target_properties(vcam PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/release)

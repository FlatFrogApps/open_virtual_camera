cmake_minimum_required(VERSION 3.25)

project(open_virtual_camera)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

include_directories(./include)

file(GLOB src
     "src/*.c"
     "src/*.cpp"
)
add_executable(vcam ${src})
set_target_properties(vcam PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_LIST_DIR}/release)

file(COPY camera_name.txt DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/Debug)
file(COPY camera_name.txt DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)
file(COPY install.bat DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)
file(COPY uninstall.bat DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)
file(COPY LICENSE.txt DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)
file(COPY README.md DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)
file(COPY win-dshow DESTINATION ${CMAKE_CURRENT_LIST_DIR}/release)

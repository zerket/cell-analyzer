# CellAnalyzer Release Build Script
# Usage: cmake -P build-release.cmake

# Configuration
set(BUILD_DIR "build-release")
set(OPENCV_BIN "D:/opencv/build/x64/vc16/bin")

# Clean previous build
message(STATUS "Cleaning previous build...")
if(EXISTS ${BUILD_DIR})
    file(REMOVE_RECURSE ${BUILD_DIR})
    message(STATUS "Previous build directory removed.")
endif()

# Create build directory
message(STATUS "Creating build directory...")
file(MAKE_DIRECTORY ${BUILD_DIR})

# Configure
message(STATUS "Configuring project...")
execute_process(
    COMMAND ${CMAKE_COMMAND} -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release -S . -B ${BUILD_DIR}
    RESULT_VARIABLE CONFIG_RESULT
)

if(NOT CONFIG_RESULT EQUAL 0)
    message(FATAL_ERROR "CMake configuration failed!")
endif()

# Build
message(STATUS "Building project...")
execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${BUILD_DIR} --config Release
    RESULT_VARIABLE BUILD_RESULT
)

if(NOT BUILD_RESULT EQUAL 0)
    message(FATAL_ERROR "Build failed!")
endif()

# Deploy Qt
message(STATUS "Deploying Qt dependencies...")
execute_process(
    COMMAND windeployqt --release --no-translations --no-system-d3d-compiler --no-opengl-sw ${BUILD_DIR}/Release/CellAnalyzer.exe
    RESULT_VARIABLE DEPLOY_RESULT
)

if(NOT DEPLOY_RESULT EQUAL 0)
    message(WARNING "Qt deployment may have issues!")
endif()

# Copy OpenCV DLLs
message(STATUS "Copying OpenCV libraries...")
set(OPENCV_DLLS
    opencv_world4110.dll
    opencv_videoio_msmf4110_64.dll
    opencv_videoio_ffmpeg4110_64.dll
)

foreach(DLL ${OPENCV_DLLS})
    set(SOURCE_DLL "${OPENCV_BIN}/${DLL}")
    set(DEST_DLL "${BUILD_DIR}/Release/${DLL}")
    
    if(EXISTS ${SOURCE_DLL})
        file(COPY ${SOURCE_DLL} DESTINATION ${BUILD_DIR}/Release/)
        message(STATUS "  - Copied ${DLL}")
    else()
        message(WARNING "  - ${DLL} not found!")
    endif()
endforeach()

message(STATUS "========================================")
message(STATUS "Build completed successfully!")
message(STATUS "========================================")
message(STATUS "Release location: ${BUILD_DIR}/Release/")
message(STATUS "Executable: ${BUILD_DIR}/Release/CellAnalyzer.exe")
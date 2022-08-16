cmake_minimum_required(VERSION 3.23)
project(MMAudioDeviceTests)

set(CMAKE_CXX_STANDARD 17)

add_library(audiolib STATIC
    audio/MyAudioSink.cpp
    audio/MyAudioSink.h
    audio/MyAudioSource.cpp
    audio/MyAudioSource.h
    audio/Utils.cpp
    audio/Utils.h
)
target_link_libraries(audiolib avrt.lib)

add_executable(PrintEndpointInfo PrintEndpointInfo.cpp)
add_executable(SetupAPIInfo SetupAPIInfo.cpp)

add_executable(PlayExclusiveStream PlayExclusiveStream.cpp)
target_link_libraries(PlayExclusiveStream audiolib)

add_executable(RecordExclusiveStream RecordExclusiveStream.cpp)
target_link_libraries(RecordExclusiveStream audiolib)

add_executable(ChangeRegistry ChangeRegistry.cpp)
target_link_libraries(ChangeRegistry audiolib)

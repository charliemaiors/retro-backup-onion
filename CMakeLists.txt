cmake_minimum_required(VERSION 3.16)
project(retro_backup VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RETRO_BACKUP_Miyoo "Build for Miyoo Mini Plus / OnionOS" OFF)
option(RETRO_BACKUP_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)

add_executable(retro-backup
    src/main.cpp
    src/app.cpp
    src/font.cpp
    src/config.cpp
    src/sftp.cpp
)

target_include_directories(retro-backup PRIVATE src)

if(RETRO_BACKUP_Miyoo)
    if(NOT DEFINED SDL2_INCLUDE_DIR OR NOT DEFINED SDL2_LIBRARY)
        message(FATAL_ERROR "Set SDL2_INCLUDE_DIR and SDL2_LIBRARY for the Miyoo SDL2 port")
    endif()
    target_include_directories(retro-backup PRIVATE "${SDL2_INCLUDE_DIR}")
    target_link_libraries(retro-backup PRIVATE "${SDL2_LIBRARY}" ${SDL2_EXTRA_LIBRARIES})
    target_link_options(retro-backup PRIVATE "-Wl,-rpath,\$ORIGIN/lib")
else()
    find_package(SDL2 REQUIRED)
    if(TARGET SDL2::SDL2)
        target_link_libraries(retro-backup PRIVATE SDL2::SDL2)
    else()
        target_include_directories(retro-backup PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(retro-backup PRIVATE ${SDL2_LIBRARIES})
    endif()
endif()

target_compile_options(retro-backup PRIVATE
    -Wall -Wextra -Wpedantic
    $<$<BOOL:${RETRO_BACKUP_WARNINGS_AS_ERRORS}>:-Werror>
)

install(TARGETS retro-backup RUNTIME DESTINATION .)

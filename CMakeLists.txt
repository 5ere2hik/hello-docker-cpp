cmake_minimum_required(VERSION 3.14) # FetchContent требует 3.11+
project(HelloWorld CXX)

set(CMAKE_CXX_STANDARD 17)

# Declare sources and include directory variables
set(SOURCES src/main.cpp)
list(APPEND SOURCES src/pipeline.cpp)

set(INCLUDES ${CMAKE_SOURCE_DIR}/src)
list(APPEND INCLUDES ${CMAKE_SOURCE_DIR}/src/include)

# Try to find FFmpeg libraries via pkg-config so we can include and link them
find_package(PkgConfig REQUIRED)

# Search for libavformat, libavcodec and libavutil
pkg_check_modules(AVFORMAT REQUIRED libavformat)
pkg_check_modules(AVCODEC REQUIRED libavcodec)
pkg_check_modules(AVUTIL REQUIRED libavutil)

# --- Подключаем Catch2 из GitHub ---
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # Указываем конкретную стабильную версию
)
FetchContent_MakeAvailable(Catch2)

# RapidYAML for YAML parsing at runtime
FetchContent_Declare(
  RapidYAML
  GIT_REPOSITORY https://github.com/biojppm/rapidyaml.git
  GIT_TAG        master
)
FetchContent_MakeAvailable(RapidYAML)

# --- Основное приложение ---
add_executable(hello_exec ${SOURCES})
# include our project headers
target_include_directories(hello_exec PRIVATE ${INCLUDES})
# include FFmpeg include directories discovered via pkg-config
target_include_directories(hello_exec PRIVATE ${AVFORMAT_INCLUDE_DIRS} ${AVCODEC_INCLUDE_DIRS} ${AVUTIL_INCLUDE_DIRS})
# add any required compiler flags from pkg-config
target_compile_options(hello_exec PRIVATE ${AVFORMAT_CFLAGS_OTHER} ${AVCODEC_CFLAGS_OTHER} ${AVUTIL_CFLAGS_OTHER})
# link FFmpeg libraries
target_link_libraries(hello_exec PRIVATE ${AVFORMAT_LIBRARIES} ${AVCODEC_LIBRARIES} ${AVUTIL_LIBRARIES})
target_link_libraries(hello_exec PRIVATE ryml)

# Default stream URL for runtime config
set(DEFAULT_STREAM_URL "rtsp://192.168.0.171:8554/mystream")

# Generate a runtime config near the executable with default stream URL
configure_file(${CMAKE_SOURCE_DIR}/src/config.yml.in ${CMAKE_BINARY_DIR}/config.yml @ONLY)
add_custom_command(TARGET hello_exec POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    ${CMAKE_BINARY_DIR}/config.yml $<TARGET_FILE_DIR:hello_exec>/config.yml
  COMMENT "Copy config.yml next to hello_exec"
)

# --- Тесты ---
add_executable(unit_tests src/tests/tests.cpp)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)
target_include_directories(unit_tests PRIVATE ${INCLUDES})

# Регистрация тестов для запуска через 'ctest'
enable_testing()
add_test(NAME MyTests COMMAND unit_tests)
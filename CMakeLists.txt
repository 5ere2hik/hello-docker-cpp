cmake_minimum_required(VERSION 3.14) # FetchContent требует 3.11+
project(HelloWorld CXX)

set(CMAKE_CXX_STANDARD 17)

# Declare sources and include directory variables
set(SOURCES src/main.cpp)
list(APPEND SOURCES src/pipeline.cpp)

set(INCLUDES ${CMAKE_SOURCE_DIR}/src)
list(APPEND INCLUDES ${CMAKE_SOURCE_DIR}/src/include)

# --- Подключаем Catch2 из GitHub ---
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # Указываем конкретную стабильную версию
)
FetchContent_MakeAvailable(Catch2)

# --- Основное приложение ---
add_executable(hello_exec ${SOURCES})
target_include_directories(hello_exec PRIVATE ${INCLUDES})

# --- Тесты ---
add_executable(unit_tests src/tests/tests.cpp)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)
target_include_directories(unit_tests PRIVATE ${INCLUDES})

# Регистрация тестов для запуска через 'ctest'
enable_testing()
add_test(NAME MyTests COMMAND unit_tests)
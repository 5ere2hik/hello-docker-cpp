cmake_minimum_required(VERSION 3.14) # FetchContent требует 3.11+
project(HelloWorld CXX)

set(CMAKE_CXX_STANDARD 17)

# --- Подключаем Catch2 из GitHub ---
include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.4.0 # Указываем конкретную стабильную версию
)
FetchContent_MakeAvailable(Catch2)

# --- Основное приложение ---
add_executable(hello_exec src/main.cpp)

# --- Тесты ---
add_executable(unit_tests src/tests/tests.cpp)
target_link_libraries(unit_tests PRIVATE Catch2::Catch2WithMain)

# Регистрация тестов для запуска через 'ctest'
enable_testing()
add_test(NAME MyTests COMMAND unit_tests)
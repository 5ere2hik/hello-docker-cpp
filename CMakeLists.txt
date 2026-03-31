cmake_minimum_required(VERSION 3.10)
project(HelloWorld)

# Указываем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Создаем исполняемый файл
add_executable(hello_exec src/main.cpp)
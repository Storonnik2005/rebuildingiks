cmake_minimum_required(VERSION 3.10)
project(GithubManager VERSION 1.0)

# Устанавливаем стандарт C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Добавляем исполняемый файл
add_executable(github-manager main.cpp)

# Устанавливаем кодировку UTF-8 для MSVC компилятора
if(MSVC)
    target_compile_options(github-manager PRIVATE "/utf-8")
endif()

# Устанавливаем выходную директорию
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Проверяем наличие библиотеки filesystem для старых версий компиляторов
if(MINGW)
    target_link_libraries(github-manager stdc++fs)
endif()

# Информация о сборке
message(STATUS "Configured for build type: ${CMAKE_BUILD_TYPE}")

# Установка
install(TARGETS github-manager DESTINATION bin) 
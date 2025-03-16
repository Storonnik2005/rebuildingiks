@echo off
echo === Сборка GitHub Manager ===
echo.

REM Проверяем наличие CMake
where cmake >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ОШИБКА: CMake не найден. Пожалуйста, установите CMake.
    echo Скачать можно здесь: https://cmake.org/download/
    exit /b 1
)

REM Проверяем наличие компилятора
where g++ >nul 2>nul
if %ERRORLEVEL% == 0 (
    echo Найден компилятор g++.
    set COMPILER=gcc
) else (
    where cl >nul 2>nul
    if %ERRORLEVEL% == 0 (
        echo Найден компилятор MSVC.
        set COMPILER=msvc
    ) else (
        echo ОШИБКА: Компилятор не найден. Пожалуйста, установите MinGW или Visual Studio.
        exit /b 1
    )
)

REM Создаем директорию для сборки
if not exist build mkdir build

REM Переходим в директорию сборки
cd build

echo Генерирование проекта с помощью CMake...
cmake .. -G "MinGW Makefiles"

echo.
echo Сборка проекта...
cmake --build .

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ОШИБКА: Сборка завершилась с ошибкой.
    exit /b 1
)

echo.
echo Сборка успешно завершена!
echo Исполняемый файл находится в: %CD%\bin\github-manager.exe

echo.
echo Запустить программу? [y/n]
set /p RUN=

if /i "%RUN%"=="y" (
    echo.
    echo Запуск GitHub Manager...
    bin\github-manager.exe
)

cd ..

echo.
echo === Готово === 
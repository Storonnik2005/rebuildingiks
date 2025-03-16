@echo off
echo === Простая сборка GitHub Manager ===
echo.

REM Проверяем наличие компилятора g++
where g++ >nul 2>nul
if %ERRORLEVEL% NEQ 0 (
    echo ОШИБКА: Компилятор g++ не найден. Пожалуйста, установите MinGW.
    echo Скачать можно здесь: https://sourceforge.net/projects/mingw-w64/
    exit /b 1
)

echo Компиляция программы...
g++ main.cpp -o github-manager.exe -std=c++17

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ОШИБКА: Сборка завершилась с ошибкой.
    exit /b 1
)

echo.
echo Сборка успешно завершена!
echo Исполняемый файл: github-manager.exe

echo.
echo Запустить программу? [y/n]
set /p RUN=

if /i "%RUN%"=="y" (
    echo.
    echo Запуск GitHub Manager...
    github-manager.exe
)

echo.
echo === Готово === 
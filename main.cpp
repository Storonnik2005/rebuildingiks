#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <array>
#include <regex>
#include <map>
#include <set>
#include <chrono>
#include <thread>
#include <locale>
#include <codecvt>
#include <functional>
#include <iomanip>
#include <windows.h>
#include <shobjidl.h>
#include <shlobj.h>
#include <commdlg.h>
#include <objbase.h>

// Класс для управления GitHub репозиториями
class GitHubManager {
private:
    std::wstring username;
    std::wstring email;
    bool isAuthenticated = false;
    std::wstring currentDirectory; // Текущая рабочая директория

    // Выполнение команды в командной строке
    std::string executeCommand(const std::string& command) {
        std::string result;
        char buffer[128];
        FILE* pipe = _popen(command.c_str(), "r");
        
        if (!pipe) {
            return "Ошибка выполнения команды.";
        }

        while (!feof(pipe)) {
            if (fgets(buffer, 128, pipe) != nullptr) {
                result += buffer;
            }
        }

        _pclose(pipe);
        return result;
    }

    // Выполнение команды в указанной директории
    std::string executeCommandInDirectory(const std::string& command, const std::string& directory) {
        // Сохраняем текущую директорию
        char currentDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, currentDir);
        
        // Переходим в указанную директорию
        SetCurrentDirectoryA(directory.c_str());
        
        // Выполняем команду
        std::string result = executeCommand(command);
        
        // Возвращаемся в исходную директорию
        SetCurrentDirectoryA(currentDir);
        
        return result;
    }

    // Конвертация из string в wstring
    std::wstring stringToWstring(const std::string& str) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
    }

    // Конвертация из wstring в string
    std::string wstringToString(const std::wstring& wstr) {
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(wstr);
    }

    // Проверка, находимся ли мы в директории Git репозитория
    bool isGitRepository() {
        std::string result = executeCommand("git rev-parse --is-inside-work-tree 2>nul");
        return (result.find("true") != std::string::npos);
    }

    // Проверка, является ли указанная директория Git репозиторием
    bool isGitRepository(const std::string& directory) {
        std::string result = executeCommandInDirectory("git rev-parse --is-inside-work-tree 2>nul", directory);
        return (result.find("true") != std::string::npos);
    }

    // Проверка, хочет ли пользователь вернуться в главное меню
    bool checkForHomeCommand(const std::wstring& input) {
        std::wstring lowerInput = input;
        std::transform(lowerInput.begin(), lowerInput.end(), lowerInput.begin(), ::tolower);
        return (lowerInput == L"home" || lowerInput == L"домой" || lowerInput == L"меню" || lowerInput == L"выход");
    }

    // Создание первого коммита в репозитории
    bool createInitialCommit(const std::string& workDir = "") {
        std::wcout << L"В репозитории пока нет коммитов. Необходимо сделать первый коммит.\n";
        std::wcout << L"Хотите создать первый коммит? (д/н): ";
        wchar_t choice;
        std::wcin >> choice;
        std::wcin.ignore();
        
        if (choice == L'д' || choice == L'Д') {
            // Создаем пустой README.md для первого коммита
            std::string createReadmeCmd = "echo # New Repository > README.md";
            std::string addCmd = "git add README.md";
            std::string commitCmd = "git commit -m \"Initial commit\"";
            
            if (workDir.empty()) {
                executeCommand(createReadmeCmd);
                executeCommand(addCmd);
                std::string result = executeCommand(commitCmd);
                std::wcout << stringToWstring(result) << L"\n";
            } else {
                executeCommandInDirectory(createReadmeCmd, workDir);
                executeCommandInDirectory(addCmd, workDir);
                std::string result = executeCommandInDirectory(commitCmd, workDir);
                std::wcout << stringToWstring(result) << L"\n";
            }
            
            std::wcout << L"Первый коммит создан. Теперь можно работать с ветками.\n";
            return true;
        } else {
            std::wcout << L"Операция отменена. Создайте первый коммит для работы с ветками.\n";
            return false;
        }
    }

    // Проверка наличия коммитов в репозитории
    bool hasCommits(const std::string& workDir = "") {
        std::string revParseCmd = "git rev-parse --verify HEAD 2>nul";
        std::string revParseResult;
        
        if (workDir.empty()) {
            revParseResult = executeCommand(revParseCmd);
        } else {
            revParseResult = executeCommandInDirectory(revParseCmd, workDir);
        }
        
        return !(revParseResult.empty() || revParseResult.find("fatal") != std::string::npos);
    }

    // Запрос директории для выполнения Git-операции
    std::string requestGitDirectory() {
        std::wcout << L"Текущая директория не является Git репозиторием.\n";
        
        // Используем диалог выбора директории вместо ручного ввода
        std::wcout << L"Выберите директорию с Git репозиторием...\n";
        std::wstring path = openFolderDialog(L"Выберите директорию с Git репозиторием");
        
        if (path.empty()) {
            std::wcout << L"Выбор директории отменен.\n";
            return "";
        }
        
        std::string pathStr = wstringToString(path);
        
        // Проверяем, существует ли директория
        if (!std::filesystem::exists(pathStr)) {
            std::wcout << L"Указанная директория не существует!\n";
            return "";
        }
        
        // Проверяем, является ли директория Git репозиторием
        if (!isGitRepository(pathStr)) {
            std::wcout << L"Указанная директория не является Git репозиторием!\n";
            std::wcout << L"Хотите инициализировать Git репозиторий в этой директории? (д/н): ";
            wchar_t choice;
            std::wcin >> choice;
            std::wcin.ignore();
            
            if (choice == L'д' || choice == L'Д') {
                std::string initCmd = "git init";
                std::string result = executeCommandInDirectory(initCmd, pathStr);
                std::wcout << stringToWstring(result) << L"\n";
                std::wcout << L"Git репозиторий инициализирован в директории: " << path << L"\n";
                
                // Проверяем, что репозиторий действительно инициализирован
                if (isGitRepository(pathStr)) {
                    // Предлагаем создать первый коммит сразу после инициализации
                    if (createInitialCommit(pathStr)) {
                        return pathStr; // Возвращаем путь для продолжения операции
                    } else {
                        return ""; // Пользователь отказался создавать коммит
                    }
                } else {
                    std::wcout << L"Не удалось инициализировать Git репозиторий!\n";
                    return "";
                }
            } else {
                return "";
            }
        }
        
        // Проверяем, есть ли коммиты в репозитории
        if (!hasCommits(pathStr)) {
            if (!createInitialCommit(pathStr)) {
                return ""; // Пользователь отказался создавать коммит
            }
        }
        
        return pathStr;
    }

public:
    GitHubManager() {
        // Настройка кодировки для корректного отображения русских символов
        SetConsoleOutputCP(CP_UTF8);
        SetConsoleCP(CP_UTF8);
        
        // Сохраняем текущую директорию
        char currentDir[MAX_PATH];
        GetCurrentDirectoryA(MAX_PATH, currentDir);
        currentDirectory = stringToWstring(currentDir);
    }

    // Аутентификация через GitHub CLI
    bool authenticate() {
        std::wcout << L"Проверка аутентификации GitHub CLI...\n";
        std::string result = executeCommand("gh auth status");

        if (result.find("not logged") != std::string::npos) {
            std::wcout << L"Вы не авторизованы в GitHub CLI. Выполняется вход...\n";
            result = executeCommand("gh auth login -w");
            std::wcout << L"Откройте браузер и выполните инструкции по авторизации.\n";
            std::wcout << L"После авторизации нажмите Enter для продолжения...";
            std::cin.ignore();
            std::cin.get();
        }
        
        result = executeCommand("gh auth status");
        isAuthenticated = (result.find("not logged") == std::string::npos);
        
        if (isAuthenticated) {
            std::wcout << L"Авторизация успешна!\n";
            loadUserInfo();
        }
        
        return isAuthenticated;
    }

    // Загрузка информации о пользователе
    void loadUserInfo() {
        std::string usernameStr = executeCommand("gh api user -q .login");
        if (!usernameStr.empty()) {
            usernameStr.erase(usernameStr.find_last_not_of("\r\n") + 1);
            username = stringToWstring(usernameStr);
        }

        std::string emailStr = executeCommand("git config user.email");
        if (!emailStr.empty()) {
            emailStr.erase(emailStr.find_last_not_of("\r\n") + 1);
            email = stringToWstring(emailStr);
        }
    }

    // Функция для выбора файлов через стандартный диалог Windows
    std::vector<std::wstring> openFileDialog(const std::wstring& initialDir, bool multiSelect = true) {
        std::vector<std::wstring> selectedFiles;
        
        // Инициализация COM
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr)) {
            IFileOpenDialog* pFileOpen;
            
            // Создаем экземпляр диалога выбора файлов
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
                                 IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
            
            if (SUCCEEDED(hr)) {
                // Настраиваем диалог
                if (multiSelect) {
                    DWORD dwOptions;
                    pFileOpen->GetOptions(&dwOptions);
                    pFileOpen->SetOptions(dwOptions | FOS_ALLOWMULTISELECT);
                }
                
                // Устанавливаем начальную директорию, если она указана
                if (!initialDir.empty()) {
                    IShellItem* pItem;
                    hr = SHCreateItemFromParsingName(initialDir.c_str(), NULL, IID_IShellItem, 
                                                   reinterpret_cast<void**>(&pItem));
                    if (SUCCEEDED(hr)) {
                        pFileOpen->SetFolder(pItem);
                        pItem->Release();
                    }
                }
                
                // Показываем диалог
                hr = pFileOpen->Show(NULL);
                
                if (SUCCEEDED(hr)) {
                    if (multiSelect) {
                        // Получаем выбранные файлы
                        IShellItemArray* pItemArray;
                        hr = pFileOpen->GetResults(&pItemArray);
                        
                        if (SUCCEEDED(hr)) {
                            DWORD count;
                            pItemArray->GetCount(&count);
                            
                            for (DWORD i = 0; i < count; i++) {
                                IShellItem* pItem;
                                hr = pItemArray->GetItemAt(i, &pItem);
                                
                                if (SUCCEEDED(hr)) {
                                    PWSTR pszFilePath;
                                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                                    
                                    if (SUCCEEDED(hr)) {
                                        selectedFiles.push_back(pszFilePath);
                                        CoTaskMemFree(pszFilePath);
                                    }
                                    pItem->Release();
                                }
                            }
                            pItemArray->Release();
                        }
                    } else {
                        // Получаем выбранный файл
                        IShellItem* pItem;
                        hr = pFileOpen->GetResult(&pItem);
                        
                        if (SUCCEEDED(hr)) {
                            PWSTR pszFilePath;
                            hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);
                            
                            if (SUCCEEDED(hr)) {
                                selectedFiles.push_back(pszFilePath);
                                CoTaskMemFree(pszFilePath);
                            }
                            pItem->Release();
                        }
                    }
                }
                pFileOpen->Release();
            }
            CoUninitialize();
        }
        
        return selectedFiles;
    }
    
    // Функция для выбора директории через стандартный диалог Windows
    std::wstring openFolderDialog(const std::wstring& title = L"Выберите директорию") {
        std::wstring selectedFolder;
        
        // Инициализация COM
        HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
        if (SUCCEEDED(hr)) {
            IFileOpenDialog* pFileOpen;
            
            // Создаем экземпляр диалога выбора файлов
            hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL, 
                                 IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
            
            if (SUCCEEDED(hr)) {
                // Настраиваем диалог для выбора папок
                DWORD dwOptions;
                pFileOpen->GetOptions(&dwOptions);
                pFileOpen->SetOptions(dwOptions | FOS_PICKFOLDERS);
                
                // Устанавливаем заголовок
                pFileOpen->SetTitle(title.c_str());
                
                // Показываем диалог
                hr = pFileOpen->Show(NULL);
                
                if (SUCCEEDED(hr)) {
                    // Получаем выбранную папку
                    IShellItem* pItem;
                    hr = pFileOpen->GetResult(&pItem);
                    
                    if (SUCCEEDED(hr)) {
                        PWSTR pszFolderPath;
                        hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFolderPath);
                        
                        if (SUCCEEDED(hr)) {
                            selectedFolder = pszFolderPath;
                            CoTaskMemFree(pszFolderPath);
                        }
                        pItem->Release();
                    }
                }
                pFileOpen->Release();
            }
            CoUninitialize();
        }
        
        return selectedFolder;
    }

    // Создать полный проект (репозиторий + локальный проект)
    void createFullProject() {
        if (!isAuthenticated && !authenticate()) return;

        // Запрашиваем информацию о репозитории
        std::wstring repoName, description;
        std::wcout << L"Введите имя репозитория: ";
        std::getline(std::wcin, repoName);
        
        if (checkForHomeCommand(repoName)) {
            return;
        }
        
        if (repoName.empty()) {
            std::wcout << L"Имя репозитория не может быть пустым!\n";
            return;
        }
        
        std::wcout << L"Введите описание репозитория (опционально): ";
        std::getline(std::wcin, description);
        
        if (checkForHomeCommand(description)) {
            return;
        }
        
        // Запрашиваем тип репозитория
        std::wcout << L"Выберите тип репозитория:\n";
        std::wcout << L"1. Приватный\n";
        std::wcout << L"2. Публичный\n";
        std::wcout << L"Ваш выбор (или 'home' для отмены): ";
        
        std::wstring choiceStr;
        std::getline(std::wcin, choiceStr);
        
        if (checkForHomeCommand(choiceStr)) {
            return;
        }
        
        int choice;
        try {
            choice = std::stoi(choiceStr);
        } catch (const std::exception&) {
            std::wcout << L"Неверный ввод. Отмена операции.\n";
            return;
        }
        
        std::string visibility = (choice == 1) ? "--private" : "--public";
        
        // Запрашиваем локальную директорию для проекта через диалог выбора папки
        std::wcout << L"Выберите директорию для создания локального проекта...\n";
        std::wstring localPath = openFolderDialog(L"Выберите директорию для создания проекта");
        
        if (localPath.empty()) {
            std::wcout << L"Директория не выбрана. Отмена операции.\n";
            return;
        }
        
        std::string localPathStr = wstringToString(localPath);
        
        // Добавляем имя репозитория к пути, если его там нет
        if (localPathStr.find(wstringToString(repoName)) == std::string::npos) {
            localPathStr += "\\" + wstringToString(repoName);
        }
        
        // Создаем директорию, если она не существует
        if (!std::filesystem::exists(localPathStr)) {
            try {
                std::filesystem::create_directories(localPathStr);
                std::wcout << L"Создана директория: " << stringToWstring(localPathStr) << L"\n";
            } catch (const std::exception& e) {
                std::wcout << L"Ошибка при создании директории: " << stringToWstring(e.what()) << L"\n";
                return;
            }
        }
        
        // Создаем репозиторий на GitHub
        std::wcout << L"Создание репозитория на GitHub...\n";
        std::string cmd = "gh repo create " + wstringToString(repoName) + " " + visibility;
        if (!description.empty()) {
            cmd += " --description \"" + wstringToString(description) + "\"";
        }
        
        std::string result = executeCommand(cmd);
        std::wcout << stringToWstring(result) << L"\n";
        
        // Проверяем успешность создания репозитория
        bool repoCreated = result.find("Created repository") != std::string::npos || 
                          result.find("HTTP 201") != std::string::npos ||
                          result.find("https://github.com/") != std::string::npos;
                          
        // Если в выводе есть сообщение об ошибке, считаем, что репозиторий не создан
        bool hasError = result.find("GraphQL:") != std::string::npos && 
                       result.find("error") != std::string::npos;
                       
        if (!repoCreated || hasError) {
            std::wcout << L"Ошибка при создании репозитория на GitHub. Проверьте вывод выше.\n";
            return;
        }
        
        // Инициализируем локальный репозиторий
        std::wcout << L"Инициализация локального репозитория...\n";
        std::string initCmd = "git init";
        result = executeCommandInDirectory(initCmd, localPathStr);
        std::wcout << stringToWstring(result) << L"\n";
        
        // Создаем README.md
        std::string readmePath = localPathStr + "/README.md";
        if (!std::filesystem::exists(readmePath)) {
            std::ofstream readmeFile(readmePath);
            readmeFile << "# " << wstringToString(repoName) << "\n\n";
            
            if (!description.empty()) {
                readmeFile << wstringToString(description) << "\n\n";
            }
            
            readmeFile << "## Содержание\n\n";
            readmeFile << "- [Установка](#установка)\n";
            readmeFile << "- [Использование](#использование)\n";
            readmeFile << "- [Лицензия](#лицензия)\n\n";
            
            readmeFile << "## Установка\n\n";
            readmeFile << "```\n";
            readmeFile << "# Клонирование репозитория\n";
            readmeFile << "git clone https://github.com/" << wstringToString(username) << "/" << wstringToString(repoName) << ".git\n";
            readmeFile << "cd " << wstringToString(repoName) << "\n";
            readmeFile << "```\n\n";
            
            readmeFile << "## Использование\n\n";
            readmeFile << "Добавьте примеры использования.\n\n";
            
            readmeFile << "## Лицензия\n\n";
            readmeFile << "Этот проект лицензирован под [MIT License](LICENSE).\n";
            
            readmeFile.close();
            std::wcout << L"Создан файл README.md\n";
        }
        
        // Спрашиваем, хочет ли пользователь добавить файлы
        std::wcout << L"Хотите добавить файлы в проект? (д/н): ";
        wchar_t addFiles;
        std::wcin >> addFiles;
        std::wcin.ignore();
        
        std::vector<std::wstring> selectedFilePaths;
        bool keepStructure = false;
        
        if (addFiles == L'д' || addFiles == L'Д') {
            // Запрашиваем исходную директорию с файлами через диалог
            std::wcout << L"Выберите директорию с исходными файлами...\n";
            std::wstring sourceDirPath = openFolderDialog(L"Выберите директорию с исходными файлами");
            
            if (sourceDirPath.empty()) {
                std::wcout << L"Директория не выбрана. Продолжаем без добавления файлов.\n";
            } else {
                // Открываем диалог выбора файлов
                std::wcout << L"Выберите файлы для добавления в проект...\n";
                selectedFilePaths = openFileDialog(sourceDirPath);
                
                if (selectedFilePaths.empty()) {
                    std::wcout << L"Файлы не выбраны. Продолжаем без добавления файлов.\n";
                } else {
                    std::wcout << L"Выбрано " << selectedFilePaths.size() << L" файлов.\n";
                    
                    // Запрашиваем структуру директорий в репозитории
                    std::wcout << L"Сохранить структуру директорий? (д/н): ";
                    wchar_t preserveStructure;
                    std::wcin >> preserveStructure;
                    std::wcin.ignore();
                    
                    keepStructure = (preserveStructure == L'д' || preserveStructure == L'Д');
                    
                    // Копируем выбранные файлы
                    int copiedCount = 0;
                    for (const auto& srcPath : selectedFilePaths) {
                        std::filesystem::path src(srcPath);
                        std::filesystem::path relativePath;
                        
                        if (keepStructure) {
                            // Получаем относительный путь от исходной директории
                            relativePath = std::filesystem::relative(src, sourceDirPath);
                        } else {
                            // Используем только имя файла
                            relativePath = src.filename();
                        }
                        
                        // Формируем полный целевой путь
                        std::filesystem::path destPath = localPathStr + "/" + relativePath.string();
                        
                        // Создаем промежуточные директории, если нужно
                        std::filesystem::path destDir = destPath.parent_path();
                        if (!destDir.empty() && !std::filesystem::exists(destDir)) {
                            std::filesystem::create_directories(destDir);
                        }
                        
                        try {
                            std::filesystem::copy_file(src, destPath, 
                                                     std::filesystem::copy_options::overwrite_existing);
                            std::wcout << L"Файл скопирован: " << src.wstring() << L" -> " << destPath.wstring() << L"\n";
                            copiedCount++;
                        } catch (const std::exception& e) {
                            std::wcout << L"Ошибка при копировании файла " << src.wstring() << L": " 
                                      << stringToWstring(e.what()) << L"\n";
                        }
                    }
                    
                    std::wcout << L"Скопировано " << copiedCount << L" из " << selectedFilePaths.size() << L" файлов.\n";
                }
            }
        }
        
        // Добавляем файлы в Git и создаем коммит
        std::string addCmd = "git add .";
        result = executeCommandInDirectory(addCmd, localPathStr);
        std::wcout << L"Файлы добавлены в индекс Git.\n";
        
        std::string commitCmd = "git commit -m \"Initial commit\"";
        result = executeCommandInDirectory(commitCmd, localPathStr);
        std::wcout << stringToWstring(result) << L"\n";
        
        // Связываем локальный и удаленный репозитории
        std::string remoteCmd = "git remote add origin https://github.com/" + 
                              wstringToString(username) + "/" + wstringToString(repoName) + ".git";
        result = executeCommandInDirectory(remoteCmd, localPathStr);
        
        // Push в удаленный репозиторий
        std::string pushCmd = "git push -u origin master";
        result = executeCommandInDirectory(pushCmd, localPathStr);
        std::wcout << stringToWstring(result) << L"\n";
        
        std::wcout << L"Проект успешно создан в папке: " << stringToWstring(localPathStr) << L"\n";
        std::wcout << L"Репозиторий доступен по адресу: https://github.com/" << username << L"/" << repoName << L"\n";
    }

    // Создать репозиторий на GitHub
    void createGitHubRepository() {
        if (!isAuthenticated && !authenticate()) return;

        std::wstring repoName, description;
        std::wcout << L"Введите имя репозитория: ";
        std::getline(std::wcin, repoName);
        
        std::wcout << L"Введите описание репозитория (опционально): ";
        std::getline(std::wcin, description);
        
        std::wcout << L"Выберите тип репозитория:\n";
        std::wcout << L"1. Приватный\n";
        std::wcout << L"2. Публичный\n";
        std::wcout << L"Ваш выбор: ";
        
        int choice;
        std::wcin >> choice;
        std::wcin.ignore();
        
        std::string visibility = (choice == 1) ? "--private" : "--public";
        
        std::string cmd = "gh repo create " + wstringToString(repoName) + " " + visibility;
        if (!description.empty()) {
            cmd += " --description \"" + wstringToString(description) + "\"";
        }
        
        std::string result = executeCommand(cmd);
        std::wcout << stringToWstring(result) << L"\n";
        
        // Проверяем успешность создания репозитория
        bool repoCreated = result.find("Created repository") != std::string::npos || 
                          result.find("HTTP 201") != std::string::npos ||
                          result.find("https://github.com/") != std::string::npos;
                          
        // Если в выводе есть сообщение об ошибке, считаем, что репозиторий не создан
        bool hasError = result.find("GraphQL:") != std::string::npos && 
                       result.find("error") != std::string::npos;
                       
        if (!repoCreated || hasError) {
            std::wcout << L"Ошибка при создании репозитория на GitHub. Проверьте вывод выше.\n";
        } else {
            std::wcout << L"Репозиторий успешно создан!\n";
            std::wcout << L"Репозиторий доступен по адресу: https://github.com/" << username << L"/" << repoName << L"\n";
        }
    }

    // Инициализировать локальный репозиторий
    void initLocalRepository() {
        std::wstring path;
        std::wcout << L"Введите путь для создания репозитория (пустое значение для текущей директории): ";
        std::getline(std::wcin, path);

        std::string cmd;
        if (path.empty()) {
            cmd = "git init";
        } else {
            cmd = "mkdir \"" + wstringToString(path) + "\" && cd \"" + wstringToString(path) + "\" && git init";
        }

        std::string result = executeCommand(cmd);
        std::wcout << stringToWstring(result) << L"\n";
        std::wcout << L"Локальный репозиторий успешно инициализирован!\n";
    }

    // Связать локальный и удаленный репозитории
    void linkRepositories() {
        if (!isGitRepository()) {
            std::wcout << L"Текущая директория не является Git репозиторием.\n";
            std::wcout << L"Хотите инициализировать локальный репозиторий? (д/н): ";
            wchar_t choice;
            std::wcin >> choice;
            std::wcin.ignore();
            
            if (choice == L'д' || choice == L'Д') {
                initLocalRepository();
            } else {
                return;
            }
        }

        std::wstring remoteUrl;
        std::wcout << L"Введите URL удаленного репозитория: ";
        std::getline(std::wcin, remoteUrl);

        std::string cmd = "git remote add origin " + wstringToString(remoteUrl);
        std::string result = executeCommand(cmd);
        
        // Проверяем, есть ли ошибки в выводе команды
        if (result.find("error") != std::string::npos || result.find("fatal") != std::string::npos) {
            std::wcout << L"Ошибка при связывании репозиториев: " << stringToWstring(result) << L"\n";
            
            // Проверяем, существует ли уже remote с именем origin
            if (result.find("remote origin already exists") != std::string::npos) {
                std::wcout << L"Remote с именем 'origin' уже существует. Хотите обновить URL? (д/н): ";
                wchar_t updateChoice;
                std::wcin >> updateChoice;
                std::wcin.ignore();
                
                if (updateChoice == L'д' || updateChoice == L'Д') {
                    cmd = "git remote set-url origin " + wstringToString(remoteUrl);
                    result = executeCommand(cmd);
                    std::wcout << L"URL удаленного репозитория обновлен!\n";
                } else {
                    return;
                }
            } else {
                return;
            }
        } else {
            std::wcout << L"Локальный репозиторий связан с удаленным!\n";
        }
        
        std::wcout << L"Выполнить первоначальный push? (д/н): ";
        
        wchar_t pushChoice;
        std::wcin >> pushChoice;
        std::wcin.ignore();
        
        if (pushChoice == L'д' || pushChoice == L'Д') {
            result = executeCommand("git push -u origin master");
            std::wcout << stringToWstring(result) << L"\n";
            
            // Проверяем успешность push
            if (result.find("error") != std::string::npos || result.find("fatal") != std::string::npos) {
                std::wcout << L"Ошибка при выполнении push. Проверьте вывод выше.\n";
            } else {
                std::wcout << L"Push выполнен успешно!\n";
            }
        }
    }

    // Создать файл README.md
    void createReadmeFile() {
        std::wstring projectName, description;
        
        std::wcout << L"Введите название проекта: ";
        std::getline(std::wcin, projectName);
        
        std::wcout << L"Введите описание проекта (опционально): ";
        std::getline(std::wcin, description);
        
        std::ofstream readmeFile("README.md");
        readmeFile << "# " << wstringToString(projectName) << "\n\n";
        
        if (!description.empty()) {
            readmeFile << wstringToString(description) << "\n\n";
        }
        
        readmeFile << "## Содержание\n\n";
        readmeFile << "- [Установка](#установка)\n";
        readmeFile << "- [Использование](#использование)\n";
        readmeFile << "- [Лицензия](#лицензия)\n\n";
        
        readmeFile << "## Установка\n\n";
        readmeFile << "```\n";
        readmeFile << "# Клонирование репозитория\n";
        readmeFile << "git clone https://github.com/username/" << wstringToString(projectName) << ".git\n";
        readmeFile << "cd " << wstringToString(projectName) << "\n";
        readmeFile << "```\n\n";
        
        readmeFile << "## Использование\n\n";
        readmeFile << "Добавьте примеры использования.\n\n";
        
        readmeFile << "## Лицензия\n\n";
        readmeFile << "Этот проект лицензирован под [MIT License](LICENSE).\n";
        
        readmeFile.close();
        
        std::wcout << L"Файл README.md успешно создан!\n";
    }

    // Добавить файлы в индекс
    void addToIndex() {
        if (!isGitRepository()) {
            std::wcout << L"Текущая директория не является Git репозиторием.\n";
            return;
        }

        std::wstring files;
        std::wcout << L"Введите пути к файлам для добавления (пустое значение для всех файлов): ";
        std::getline(std::wcin, files);

        std::string cmd;
        if (files.empty()) {
            cmd = "git add .";
        } else {
            cmd = "git add " + wstringToString(files);
        }

        std::string result = executeCommand(cmd);
        std::wcout << L"Файлы добавлены в индекс!\n";
        
        // Показать статус
        result = executeCommand("git status");
        std::wcout << stringToWstring(result) << L"\n";
    }

    // Создать коммит
    void createCommit() {
        if (!isGitRepository()) {
            std::wcout << L"Текущая директория не является Git репозиторием.\n";
            return;
        }

        std::wstring message;
        std::wcout << L"Введите сообщение коммита: ";
        std::getline(std::wcin, message);

        if (message.empty()) {
            std::wcout << L"Сообщение коммита не может быть пустым!\n";
            return;
        }

        std::string cmd = "git commit -m \"" + wstringToString(message) + "\"";
        std::string result = executeCommand(cmd);
        std::wcout << stringToWstring(result) << L"\n";
    }

    // Отправить изменения на GitHub
    void pushChanges() {
        if (!isGitRepository()) {
            std::wcout << L"Текущая директория не является Git репозиторием.\n";
            return;
        }

        std::wstring branch;
        std::wcout << L"Введите имя ветки (пустое значение для текущей ветки): ";
        std::getline(std::wcin, branch);

        std::string cmd;
        if (branch.empty()) {
            cmd = "git push";
        } else {
            cmd = "git push origin " + wstringToString(branch);
        }

        std::string result = executeCommand(cmd);
        std::wcout << stringToWstring(result) << L"\n";
    }

    // Сохранить имя пользователя
    void saveUsername() {
        std::wstring newUsername;
        std::wcout << L"Введите имя пользователя GitHub: ";
        std::getline(std::wcin, newUsername);

        if (newUsername.empty()) {
            std::wcout << L"Имя пользователя не может быть пустым!\n";
            return;
        }

        username = newUsername;
        std::wcout << L"Имя пользователя сохранено: " << username << L"\n";
    }

    // Открыть GitHub в браузере
    void openGitHubInBrowser() {
        std::wstring url = L"https://github.com/";
        
        if (!username.empty()) {
            url += username;
        }
        
        std::string cmd = "start " + wstringToString(url);
        executeCommand(cmd);
        
        std::wcout << L"GitHub открыт в браузере!\n";
    }

    // Настроить информацию Git
    void configureGitInfo() {
        std::wstring name, email;
        
        std::wcout << L"Введите ваше имя для Git: ";
        std::getline(std::wcin, name);
        
        std::wcout << L"Введите ваш email для Git: ";
        std::getline(std::wcin, email);
        
        if (!name.empty()) {
            std::string cmd = "git config --global user.name \"" + wstringToString(name) + "\"";
            executeCommand(cmd);
        }
        
        if (!email.empty()) {
            std::string cmd = "git config --global user.email \"" + wstringToString(email) + "\"";
            executeCommand(cmd);
            this->email = email;
        }
        
        std::wcout << L"Информация Git настроена успешно!\n";
        
        // Показать текущие настройки
        std::string result = executeCommand("git config --global user.name");
        std::wcout << L"Текущее имя: " << stringToWstring(result);
        
        result = executeCommand("git config --global user.email");
        std::wcout << L"Текущий email: " << stringToWstring(result);
    }

    // Просмотреть список репозиториев
    void listRepositories() {
        if (!isAuthenticated && !authenticate()) return;
        
        std::wcout << L"Загрузка списка репозиториев...\n";
        std::string result = executeCommand("gh repo list");
        std::wcout << stringToWstring(result) << L"\n";
    }

    // Клонировать репозиторий
    void cloneRepository() {
        if (!isAuthenticated && !authenticate()) return;
        
        std::wstring repoUrl;
        std::wcout << L"Введите URL репозитория или имя [username]/[repository]: ";
        std::getline(std::wcin, repoUrl);
        
        if (repoUrl.empty()) {
            std::wcout << L"URL репозитория не может быть пустым!\n";
            return;
        }
        
        std::wstring directory;
        std::wcout << L"Введите директорию для клонирования (пустое значение для текущей директории): ";
        std::getline(std::wcin, directory);
        
        std::string cmd = "gh repo clone " + wstringToString(repoUrl);
        if (!directory.empty()) {
            cmd += " " + wstringToString(directory);
        }
        
        std::string result = executeCommand(cmd);
        std::wcout << stringToWstring(result) << L"\n";
        std::wcout << L"Репозиторий успешно клонирован!\n";
    }

    // Удалить репозиторий
    void deleteRepository() {
        if (!isAuthenticated && !authenticate()) return;
        
        std::wstring repoName;
        std::wcout << L"Введите имя репозитория для удаления: ";
        std::getline(std::wcin, repoName);
        
        if (repoName.empty()) {
            std::wcout << L"Имя репозитория не может быть пустым!\n";
            return;
        }
        
        std::wcout << L"ВНИМАНИЕ: Удаление репозитория необратимо!\n";
        std::wcout << L"Введите 'YES' для подтверждения удаления: ";
        std::wstring confirmation;
        std::getline(std::wcin, confirmation);
        
        if (confirmation != L"YES") {
            std::wcout << L"Удаление отменено.\n";
            return;
        }
        
        std::string cmd;
        if (repoName.find(L'/') == std::wstring::npos && !username.empty()) {
            cmd = "gh repo delete " + wstringToString(username) + "/" + wstringToString(repoName) + " --yes";
        } else {
            cmd = "gh repo delete " + wstringToString(repoName) + " --yes";
        }
        
        std::string result = executeCommand(cmd);
        std::wcout << stringToWstring(result) << L"\n";
        std::wcout << L"Репозиторий успешно удален!\n";
    }

    // Создать issue
    void createIssue() {
        if (!isAuthenticated && !authenticate()) return;
        
        if (!isGitRepository()) {
            std::wcout << L"Текущая директория не является Git репозиторием.\n";
            return;
        }
        
        std::wstring title, body;
        std::wcout << L"Введите заголовок задачи: ";
        std::getline(std::wcin, title);
        
        if (title.empty()) {
            std::wcout << L"Заголовок задачи не может быть пустым!\n";
            return;
        }
        
        std::wcout << L"Введите описание задачи (завершите ввод пустой строкой):\n";
        std::wstring line;
        while (true) {
            std::getline(std::wcin, line);
            if (line.empty()) break;
            body += line + L"\n";
        }
        
        std::string cmd = "gh issue create --title \"" + wstringToString(title) + "\"";
        if (!body.empty()) {
            cmd += " --body \"" + wstringToString(body) + "\"";
        }
        
        std::string result = executeCommand(cmd);
        std::wcout << stringToWstring(result) << L"\n";
        std::wcout << L"Задача успешно создана!\n";
    }
    
    // Создать файл .gitignore
    void createGitignore() {
        std::wcout << L"Выберите тип проекта для .gitignore:\n";
        std::wcout << L"1. C++\n";
        std::wcout << L"2. Python\n";
        std::wcout << L"3. JavaScript/Node.js\n";
        std::wcout << L"4. Java\n";
        std::wcout << L"5. Пользовательский\n";
        std::wcout << L"Ваш выбор: ";
        
        int choice;
        std::wcin >> choice;
        std::wcin.ignore();
        
        std::string templateName;
        switch (choice) {
            case 1: templateName = "c++"; break;
            case 2: templateName = "python"; break;
            case 3: templateName = "node"; break;
            case 4: templateName = "java"; break;
            case 5: 
                createCustomGitignore();
                return;
            default:
                std::wcout << L"Неверный выбор.\n";
                return;
        }
        
        std::string cmd = "gh gitignore list";
        std::string result = executeCommand(cmd);
        
        if (result.find(templateName) == std::string::npos) {
            templateName = templateName == "c++" ? "C++" : 
                          (templateName == "python" ? "Python" : 
                          (templateName == "node" ? "Node" : "Java"));
        }
        
        cmd = "gh gitignore " + templateName + " > .gitignore";
        executeCommand(cmd);
        
        std::wcout << L"Файл .gitignore создан для " << stringToWstring(templateName) << L"!\n";
    }
    
    // Создать пользовательский файл .gitignore
    void createCustomGitignore() {
        std::wcout << L"Введите шаблоны для игнорирования (по одному на строку, пустая строка для завершения):\n";
        
        std::ofstream file(".gitignore");
        file << "# Custom .gitignore file\n\n";
        
        std::wstring line;
        while (true) {
            std::getline(std::wcin, line);
            if (line.empty()) break;
            file << wstringToString(line) << "\n";
        }
        
        file.close();
        std::wcout << L"Пользовательский файл .gitignore создан!\n";
    }

    // Показать текущую ветку
    void showCurrentBranch() {
        std::string workDir = "";
        
        if (!isGitRepository()) {
            workDir = requestGitDirectory();
            if (workDir.empty()) {
                return; // Пользователь отменил операцию или ввел 'home'
            }
        }
        
        std::string result;
        if (workDir.empty()) {
            result = executeCommand("git branch --show-current");
        } else {
            result = executeCommandInDirectory("git branch --show-current", workDir);
        }
        
        // Проверяем, есть ли ветки в репозитории
        if (result.empty() || result.find_first_not_of("\r\n\t ") == std::string::npos) {
            // В новом репозитории может не быть веток, пока не сделан первый коммит
            std::wcout << L"В репозитории пока нет веток. Необходимо сделать первый коммит.\n";
            std::wcout << L"Хотите создать первый коммит? (д/н): ";
            wchar_t choice;
            std::wcin >> choice;
            std::wcin.ignore();
            
            if (choice == L'д' || choice == L'Д') {
                // Создаем пустой README.md для первого коммита
                std::string createReadmeCmd = "echo # New Repository > README.md";
                std::string addCmd = "git add README.md";
                std::string commitCmd = "git commit -m \"Initial commit\"";
                
                if (workDir.empty()) {
                    executeCommand(createReadmeCmd);
                    executeCommand(addCmd);
                    result = executeCommand(commitCmd);
                } else {
                    executeCommandInDirectory(createReadmeCmd, workDir);
                    executeCommandInDirectory(addCmd, workDir);
                    result = executeCommandInDirectory(commitCmd, workDir);
                }
                
                std::wcout << stringToWstring(result) << L"\n";
                std::wcout << L"Первый коммит создан. Текущая ветка: master\n";
            } else {
                std::wcout << L"Операция отменена. Создайте первый коммит для работы с ветками.\n";
            }
        } else {
            std::wcout << L"Текущая ветка: " << stringToWstring(result) << L"\n";
        }
    }
    
    // Просмотреть список веток
    void listBranches() {
        std::string workDir = "";
        
        if (!isGitRepository()) {
            workDir = requestGitDirectory();
            if (workDir.empty()) {
                return; // Пользователь отменил операцию или ввел 'home'
            }
        }
        
        // Проверяем, есть ли коммиты в репозитории
        std::string revParseCmd = "git rev-parse --verify HEAD 2>nul";
        std::string revParseResult;
        
        if (workDir.empty()) {
            revParseResult = executeCommand(revParseCmd);
        } else {
            revParseResult = executeCommandInDirectory(revParseCmd, workDir);
        }
        
        if (revParseResult.empty() || revParseResult.find("fatal") != std::string::npos) {
            // В репозитории нет коммитов
            std::wcout << L"В репозитории пока нет веток. Необходимо сделать первый коммит.\n";
            std::wcout << L"Хотите создать первый коммит? (д/н): ";
            wchar_t choice;
            std::wcin >> choice;
            std::wcin.ignore();
            
            if (choice == L'д' || choice == L'Д') {
                // Создаем пустой README.md для первого коммита
                std::string createReadmeCmd = "echo # New Repository > README.md";
                std::string addCmd = "git add README.md";
                std::string commitCmd = "git commit -m \"Initial commit\"";
                
                if (workDir.empty()) {
                    executeCommand(createReadmeCmd);
                    executeCommand(addCmd);
                    std::string result = executeCommand(commitCmd);
                    std::wcout << stringToWstring(result) << L"\n";
                } else {
                    executeCommandInDirectory(createReadmeCmd, workDir);
                    executeCommandInDirectory(addCmd, workDir);
                    std::string result = executeCommandInDirectory(commitCmd, workDir);
                    std::wcout << stringToWstring(result) << L"\n";
                }
                
                std::wcout << L"Первый коммит создан. Теперь можно просмотреть ветки.\n";
            } else {
                std::wcout << L"Операция отменена. Создайте первый коммит для работы с ветками.\n";
                return;
            }
        }
        
        std::wcout << L"Локальные ветки:\n";
        std::string result;
        if (workDir.empty()) {
            result = executeCommand("git branch");
        } else {
            result = executeCommandInDirectory("git branch", workDir);
        }
        
        std::wcout << stringToWstring(result) << L"\n";
        
        std::wcout << L"Удаленные ветки:\n";
        if (workDir.empty()) {
            result = executeCommand("git branch -r");
        } else {
            result = executeCommandInDirectory("git branch -r", workDir);
        }
        
        if (result.empty() || result.find_first_not_of("\r\n\t ") == std::string::npos) {
            std::wcout << L"Удаленных веток не найдено. Возможно, репозиторий не связан с удаленным.\n";
        } else {
            std::wcout << stringToWstring(result) << L"\n";
        }
    }
    
    // Создать новую ветку
    void createBranch() {
        std::string workDir = "";
        
        if (!isGitRepository()) {
            workDir = requestGitDirectory();
            if (workDir.empty()) {
                return; // Пользователь отменил операцию или ввел 'home'
            }
        }
        
        // Проверяем, есть ли коммиты в репозитории
        if (!hasCommits(workDir)) {
            if (!createInitialCommit(workDir)) {
                return; // Пользователь отказался создавать коммит
            }
        }
        
        // Проверяем, настроен ли удаленный репозиторий
        std::string remoteCmd = "git remote -v";
        std::string remoteResult;
        
        if (workDir.empty()) {
            remoteResult = executeCommand(remoteCmd);
        } else {
            remoteResult = executeCommandInDirectory(remoteCmd, workDir);
        }
        
        if (remoteResult.empty() || remoteResult.find("origin") == std::string::npos) {
            // Удаленный репозиторий не настроен
            std::wcout << L"Удаленный репозиторий не настроен. Хотите создать репозиторий на GitHub? (д/н): ";
            wchar_t createRepoChoice;
            std::wcin >> createRepoChoice;
            std::wcin.ignore();
            
            if (createRepoChoice == L'д' || createRepoChoice == L'Д') {
                // Запрашиваем имя репозитория
                std::wcout << L"Введите имя для нового репозитория на GitHub (или 'home' для отмены): ";
                std::wstring repoName;
                std::getline(std::wcin, repoName);
                
                if (checkForHomeCommand(repoName)) {
                    return;
                }
                
                if (repoName.empty()) {
                    // Используем имя текущей директории
                    std::filesystem::path repoPath = workDir.empty() ? 
                                                   std::filesystem::current_path() : 
                                                   std::filesystem::path(workDir);
                    repoName = stringToWstring(repoPath.filename().string());
                }
                
                // Создаем репозиторий на GitHub
                std::wcout << L"Создание репозитория '" << repoName << L"' на GitHub...\n";
                
                // Запрашиваем тип репозитория
                std::wcout << L"Сделать репозиторий приватным? (д/н): ";
                wchar_t privateChoice;
                std::wcin >> privateChoice;
                std::wcin.ignore();
                
                bool isPrivate = (privateChoice == L'д' || privateChoice == L'Д');
                
                // Формируем команду для создания репозитория
                std::string createRepoCmd = "gh repo create " + wstringToString(repoName) + 
                                          " --" + (isPrivate ? "private" : "public");
                
                std::string result;
                result = executeCommand(createRepoCmd);
                std::wcout << stringToWstring(result) << L"\n";
                
                if (result.find("https://github.com/") != std::string::npos || 
                    result.find("Created repository") != std::string::npos) {
                    std::wcout << L"Репозиторий успешно создан на GitHub!\n";
                    
                    // Связываем локальный и удаленный репозитории
                    std::string remoteAddCmd = "git remote add origin https://github.com/" + 
                                             wstringToString(username) + "/" + wstringToString(repoName) + ".git";
                    
                    if (workDir.empty()) {
                        result = executeCommand(remoteAddCmd);
                    } else {
                        result = executeCommandInDirectory(remoteAddCmd, workDir);
                    }
                    
                    std::wcout << L"Локальный репозиторий связан с удаленным!\n";
                } else {
                    std::wcout << L"Не удалось создать репозиторий на GitHub. Продолжаем без связывания с удаленным репозиторием.\n";
                }
            }
        }
        
        std::wstring branchName;
        std::wcout << L"Введите имя новой ветки (или 'home' для возврата в меню): ";
        std::getline(std::wcin, branchName);
        
        if (checkForHomeCommand(branchName)) {
            return;
        }
        
        if (branchName.empty()) {
            std::wcout << L"Имя ветки не может быть пустым!\n";
            return;
        }
        
        std::string cmd = "git branch " + wstringToString(branchName);
        std::string result;
        
        if (workDir.empty()) {
            result = executeCommand(cmd);
        } else {
            result = executeCommandInDirectory(cmd, workDir);
        }
        
        if (result.empty()) {
            std::wcout << L"Ветка '" << branchName << L"' успешно создана!\n";
            
            // Спрашиваем, хочет ли пользователь отправить ветку на GitHub
            std::wcout << L"Хотите отправить ветку на GitHub? (д/н): ";
            wchar_t pushChoice;
            std::wcin >> pushChoice;
            std::wcin.ignore();
            
            if (pushChoice == L'д' || pushChoice == L'Д') {
                // Сначала переключаемся на ветку
                std::string checkoutCmd = "git checkout " + wstringToString(branchName);
                
                if (workDir.empty()) {
                    result = executeCommand(checkoutCmd);
                } else {
                    result = executeCommandInDirectory(checkoutCmd, workDir);
                }
                
                // Затем отправляем ветку на GitHub
                std::string pushCmd = "git push -u origin " + wstringToString(branchName);
                
                if (workDir.empty()) {
                    result = executeCommand(pushCmd);
                } else {
                    result = executeCommandInDirectory(pushCmd, workDir);
                }
                
                std::wcout << stringToWstring(result) << L"\n";
                std::wcout << L"Ветка отправлена на GitHub!\n";
            }
        } else {
            std::wcout << stringToWstring(result) << L"\n";
        }
    }
    
    // Переключиться на другую ветку
    void switchBranch() {
        std::string workDir = "";
        
        if (!isGitRepository()) {
            workDir = requestGitDirectory();
            if (workDir.empty()) {
                return; // Пользователь отменил операцию или ввел 'home'
            }
        }
        
        // Проверяем, есть ли коммиты в репозитории
        if (!hasCommits(workDir)) {
            if (!createInitialCommit(workDir)) {
                return; // Пользователь отказался создавать коммит
            }
        }
        
        std::string branches;
        if (workDir.empty()) {
            branches = executeCommand("git branch");
        } else {
            branches = executeCommandInDirectory("git branch", workDir);
        }
        
        std::wcout << L"Доступные ветки:\n" << stringToWstring(branches) << L"\n";
        
        std::wstring branchName;
        std::wcout << L"Введите имя ветки для переключения (или 'home' для возврата в меню): ";
        std::getline(std::wcin, branchName);
        
        if (checkForHomeCommand(branchName)) {
            return;
        }
        
        if (branchName.empty()) {
            std::wcout << L"Имя ветки не может быть пустым!\n";
            return;
        }
        
        std::string cmd = "git checkout " + wstringToString(branchName);
        std::string result;
        
        if (workDir.empty()) {
            result = executeCommand(cmd);
        } else {
            result = executeCommandInDirectory(cmd, workDir);
        }
        
        std::wcout << stringToWstring(result) << L"\n";
    }

    // Загрузить файлы из директории в репозиторий
    void uploadDirectoryFiles() {
        if (!isGitRepository()) {
            std::wcout << L"Текущая директория не является Git репозиторием.\n";
            return;
        }
        
        std::wstring sourcePath;
        std::wcout << L"Введите путь к директории для загрузки: ";
        std::getline(std::wcin, sourcePath);
        
        if (checkForHomeCommand(sourcePath)) {
            return;
        }
        
        if (!std::filesystem::exists(wstringToString(sourcePath))) {
            std::wcout << L"Указанная директория не существует!\n";
            return;
        }
        
        // Показываем список файлов в директории
        std::wcout << L"Файлы в директории " << sourcePath << L":\n";
        
        std::vector<std::filesystem::path> allFiles;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(wstringToString(sourcePath))) {
                if (entry.is_regular_file()) {
                    allFiles.push_back(entry.path());
                    std::wcout << allFiles.size() << L". " << entry.path().wstring() << L"\n";
                }
            }
        } catch (const std::exception& e) {
            std::wcout << L"Ошибка при чтении директории: " << stringToWstring(e.what()) << L"\n";
            return;
        }
        
        if (allFiles.empty()) {
            std::wcout << L"В указанной директории нет файлов.\n";
            return;
        }
        
        // Запрашиваем выбор файлов
        std::wcout << L"Выберите файлы для загрузки (введите номера через запятую, 'all' для всех, или 'home' для отмены): ";
        std::wstring selection;
        std::getline(std::wcin, selection);
        
        if (checkForHomeCommand(selection)) {
            return;
        }
        
        std::vector<std::filesystem::path> selectedFiles;
        
        if (selection == L"all" || selection == L"ALL" || selection == L"все" || selection == L"ВСЕ") {
            selectedFiles = allFiles;
        } else {
            std::wstringstream ss(selection);
            std::wstring item;
            
            while (std::getline(ss, item, L',')) {
                try {
                    int index = std::stoi(item);
                    if (index > 0 && index <= static_cast<int>(allFiles.size())) {
                        selectedFiles.push_back(allFiles[index - 1]);
                    } else {
                        std::wcout << L"Игнорирование недопустимого индекса: " << index << L"\n";
                    }
                } catch (const std::exception&) {
                    std::wcout << L"Игнорирование недопустимого ввода: " << item << L"\n";
                }
            }
        }
        
        if (selectedFiles.empty()) {
            std::wcout << L"Не выбрано ни одного файла для загрузки.\n";
            return;
        }
        
        std::wcout << L"Выбрано " << selectedFiles.size() << L" файлов.\n";
        
        // Запрашиваем структуру директорий в репозитории
        std::wcout << L"Сохранить структуру директорий? (д/н): ";
        wchar_t preserveStructure;
        std::wcin >> preserveStructure;
        std::wcin.ignore();
        
        bool keepStructure = (preserveStructure == L'д' || preserveStructure == L'Д');
        
        std::wstring targetPath;
        std::wcout << L"Введите целевой путь в репозитории (пустое значение для корня): ";
        std::getline(std::wcin, targetPath);
        
        if (checkForHomeCommand(targetPath)) {
            return;
        }
        
        std::string targetPathStr = wstringToString(targetPath);
        
        // Создаём целевую директорию, если она не существует и путь не пустой
        if (!targetPath.empty() && !std::filesystem::exists(targetPathStr)) {
            std::string mkdirCmd = "mkdir \"" + targetPathStr + "\" 2>nul";
            executeCommand(mkdirCmd);
            std::wcout << L"Создана директория: " << targetPath << L"\n";
        }
        
        // Копирование файлов
        int copiedCount = 0;
        for (const auto& src : selectedFiles) {
            std::filesystem::path relativePath;
            
            if (keepStructure) {
                // Получаем относительный путь от исходной директории
                relativePath = std::filesystem::relative(src, wstringToString(sourcePath));
            } else {
                // Используем только имя файла
                relativePath = src.filename();
            }
            
            // Формируем полный целевой путь
            std::filesystem::path destPath;
            if (targetPath.empty()) {
                destPath = relativePath.string();
            } else {
                destPath = targetPathStr + "/" + relativePath.string();
            }
            
            // Создаем промежуточные директории, если нужно
            std::filesystem::path destDir = destPath.parent_path();
            if (!destDir.empty() && !std::filesystem::exists(destDir)) {
                std::filesystem::create_directories(destDir);
            }
            
            try {
                std::filesystem::copy_file(src, destPath, 
                                          std::filesystem::copy_options::overwrite_existing);
                std::wcout << L"Файл скопирован: " << src.wstring() << L" -> " << destPath.wstring() << L"\n";
                copiedCount++;
            } catch (const std::exception& e) {
                std::wcout << L"Ошибка при копировании файла " << src.wstring() << L": " 
                          << stringToWstring(e.what()) << L"\n";
            }
        }
        
        std::wcout << L"Скопировано " << copiedCount << L" из " << selectedFiles.size() << L" файлов.\n";
        
        if (copiedCount == 0) {
            std::wcout << L"Не удалось скопировать ни одного файла. Операция отменена.\n";
            return;
        }
        
        std::wstring commitMessage;
        std::wcout << L"Введите сообщение коммита: ";
        std::getline(std::wcin, commitMessage);
        
        if (checkForHomeCommand(commitMessage)) {
            return;
        }
        
        if (commitMessage.empty()) {
            commitMessage = L"Загрузка файлов из директории " + sourcePath;
        }
        
        // Добавление файлов в Git
        std::string gitCmd = "git add .";
        executeCommand(gitCmd);
        std::wcout << L"Файлы добавлены в индекс Git.\n";
        
        // Создание коммита
        gitCmd = "git commit -m \"" + wstringToString(commitMessage) + "\"";
        std::string result = executeCommand(gitCmd);
        
        std::wcout << stringToWstring(result) << L"\n";
        
        // Спрашиваем, хочет ли пользователь отправить изменения на GitHub
        std::wcout << L"Хотите отправить изменения на GitHub? (д/н): ";
        wchar_t pushChoice;
        std::wcin >> pushChoice;
        std::wcin.ignore();
        
        if (pushChoice == L'д' || pushChoice == L'Д') {
            std::string pushCmd = "git push";
            result = executeCommand(pushCmd);
            std::wcout << stringToWstring(result) << L"\n";
            
            if (result.find("error") != std::string::npos || result.find("fatal") != std::string::npos) {
                std::wcout << L"Произошла ошибка при отправке изменений на GitHub.\n";
            } else {
                std::wcout << L"Файлы успешно загружены в репозиторий и отправлены на GitHub!\n";
            }
        } else {
            std::wcout << L"Файлы успешно загружены в локальный репозиторий!\n";
        }
    }

    // Создать и переключиться на новую ветку
    void createAndSwitchBranch() {
        std::string workDir = "";
        
        if (!isGitRepository()) {
            workDir = requestGitDirectory();
            if (workDir.empty()) {
                return; // Пользователь отменил операцию или ввел 'home'
            }
        }
        
        // Проверяем, есть ли коммиты в репозитории
        if (!hasCommits(workDir)) {
            if (!createInitialCommit(workDir)) {
                return; // Пользователь отказался создавать коммит
            }
        }
        
        std::wstring branchName;
        std::wcout << L"Введите имя новой ветки (или 'home' для возврата в меню): ";
        std::getline(std::wcin, branchName);
        
        if (checkForHomeCommand(branchName)) {
            return;
        }
        
        if (branchName.empty()) {
            std::wcout << L"Имя ветки не может быть пустым!\n";
            return;
        }
        
        std::string cmd = "git checkout -b " + wstringToString(branchName);
        std::string result;
        
        if (workDir.empty()) {
            result = executeCommand(cmd);
        } else {
            result = executeCommandInDirectory(cmd, workDir);
        }
        
        std::wcout << stringToWstring(result) << L"\n";
        std::wcout << L"Ветка '" << branchName << L"' создана и установлена как текущая!\n";
    }
    
    // Слить ветки
    void mergeBranches() {
        std::string workDir = "";
        
        if (!isGitRepository()) {
            workDir = requestGitDirectory();
            if (workDir.empty()) {
                return; // Пользователь отменил операцию или ввел 'home'
            }
        }
        
        // Проверяем, есть ли коммиты в репозитории
        if (!hasCommits(workDir)) {
            if (!createInitialCommit(workDir)) {
                return; // Пользователь отказался создавать коммит
            }
        }
        
        // Проверяем, есть ли несохраненные изменения
        std::string statusCmd = "git status --porcelain";
        std::string statusResult;
        
        if (workDir.empty()) {
            statusResult = executeCommand(statusCmd);
        } else {
            statusResult = executeCommandInDirectory(statusCmd, workDir);
        }
        
        if (!statusResult.empty()) {
            std::wcout << L"В репозитории есть несохраненные изменения:\n";
            std::wcout << stringToWstring(statusResult) << L"\n";
            std::wcout << L"Рекомендуется сохранить или отменить изменения перед слиянием веток.\n";
            std::wcout << L"Хотите продолжить слияние? (д/н): ";
            
            wchar_t continueChoice;
            std::wcin >> continueChoice;
            std::wcin.ignore();
            
            if (continueChoice != L'д' && continueChoice != L'Д') {
                std::wcout << L"Операция отменена.\n";
                return;
            }
        }
        
        // Показать текущую ветку
        std::string currentBranchCmd = "git branch --show-current";
        std::string currentBranch;
        
        if (workDir.empty()) {
            currentBranch = executeCommand(currentBranchCmd);
        } else {
            currentBranch = executeCommandInDirectory(currentBranchCmd, workDir);
        }
        
        currentBranch.erase(currentBranch.find_last_not_of("\r\n") + 1);
        std::wcout << L"Текущая ветка: " << stringToWstring(currentBranch) << L"\n";
        
        // Показать список веток
        std::string branchesCmd = "git branch";
        std::string branches;
        
        if (workDir.empty()) {
            branches = executeCommand(branchesCmd);
        } else {
            branches = executeCommandInDirectory(branchesCmd, workDir);
        }
        
        std::wcout << L"Доступные ветки:\n" << stringToWstring(branches) << L"\n";
        
        // Показать удаленные ветки
        std::string remoteBranchesCmd = "git branch -r";
        std::string remoteBranches;
        
        if (workDir.empty()) {
            remoteBranches = executeCommand(remoteBranchesCmd);
        } else {
            remoteBranches = executeCommandInDirectory(remoteBranchesCmd, workDir);
        }
        
        if (!remoteBranches.empty() && remoteBranches.find_first_not_of("\r\n\t ") != std::string::npos) {
            std::wcout << L"Удаленные ветки:\n" << stringToWstring(remoteBranches) << L"\n";
        }
        
        // Запрашиваем ветку для слияния
        std::wstring sourceBranch;
        std::wcout << L"Введите имя ветки для слияния с текущей (или 'home' для возврата в меню): ";
        std::getline(std::wcin, sourceBranch);
        
        if (checkForHomeCommand(sourceBranch)) {
            return;
        }
        
        if (sourceBranch.empty()) {
            std::wcout << L"Имя ветки не может быть пустым!\n";
            return;
        }
        
        // Проверяем, существует ли указанная ветка
        std::string branchExistsCmd = "git rev-parse --verify " + wstringToString(sourceBranch) + " 2>nul";
        std::string branchExistsResult;
        
        if (workDir.empty()) {
            branchExistsResult = executeCommand(branchExistsCmd);
        } else {
            branchExistsResult = executeCommandInDirectory(branchExistsCmd, workDir);
        }
        
        if (branchExistsResult.empty()) {
            // Проверяем, может быть это удаленная ветка
            branchExistsCmd = "git rev-parse --verify origin/" + wstringToString(sourceBranch) + " 2>nul";
            
            if (workDir.empty()) {
                branchExistsResult = executeCommand(branchExistsCmd);
            } else {
                branchExistsResult = executeCommandInDirectory(branchExistsCmd, workDir);
            }
            
            if (branchExistsResult.empty()) {
                std::wcout << L"Ветка '" << sourceBranch << L"' не существует!\n";
                return;
            } else {
                // Это удаленная ветка, спрашиваем, хочет ли пользователь создать локальную ветку
                std::wcout << L"'" << sourceBranch << L"' - это удаленная ветка. Хотите создать локальную ветку и затем выполнить слияние? (д/н): ";
                
                wchar_t createLocalChoice;
                std::wcin >> createLocalChoice;
                std::wcin.ignore();
                
                if (createLocalChoice == L'д' || createLocalChoice == L'Д') {
                    std::string createLocalCmd = "git checkout -b " + wstringToString(sourceBranch) + " origin/" + wstringToString(sourceBranch);
                    std::string createLocalResult;
                    
                    if (workDir.empty()) {
                        createLocalResult = executeCommand(createLocalCmd);
                    } else {
                        createLocalResult = executeCommandInDirectory(createLocalCmd, workDir);
                    }
                    
                    std::wcout << stringToWstring(createLocalResult) << L"\n";
                    
                    // Теперь нужно переключиться обратно на исходную ветку
                    std::string switchBackCmd = "git checkout " + currentBranch;
                    
                    if (workDir.empty()) {
                        executeCommand(switchBackCmd);
                    } else {
                        executeCommandInDirectory(switchBackCmd, workDir);
                    }
                } else {
                    std::wcout << L"Операция отменена.\n";
                    return;
                }
            }
        }
        
        // Спрашиваем о стратегии слияния
        std::wcout << L"Выберите стратегию слияния:\n";
        std::wcout << L"1. Обычное слияние (merge)\n";
        std::wcout << L"2. Перебазирование (rebase)\n";
        std::wcout << L"3. Слияние с созданием нового коммита (--no-ff)\n";
        std::wcout << L"Ваш выбор (или 'home' для отмены): ";
        
        std::wstring mergeStrategyStr;
        std::getline(std::wcin, mergeStrategyStr);
        
        if (checkForHomeCommand(mergeStrategyStr)) {
            return;
        }
        
        int mergeStrategy;
        try {
            mergeStrategy = std::stoi(mergeStrategyStr);
        } catch (const std::exception&) {
            std::wcout << L"Неверный ввод. Используется обычное слияние.\n";
            mergeStrategy = 1;
        }
        
        std::string mergeCmd;
        switch (mergeStrategy) {
            case 2:
                mergeCmd = "git rebase " + wstringToString(sourceBranch);
                break;
            case 3:
                mergeCmd = "git merge --no-ff " + wstringToString(sourceBranch);
                break;
            default:
                mergeCmd = "git merge " + wstringToString(sourceBranch);
                break;
        }
        
        std::wcout << L"Выполняется слияние ветки '" << sourceBranch << L"' в '" 
                  << stringToWstring(currentBranch) << L"'...\n";
        
        std::string result;
        if (workDir.empty()) {
            result = executeCommand(mergeCmd);
        } else {
            result = executeCommandInDirectory(mergeCmd, workDir);
        }
        
        std::wcout << stringToWstring(result) << L"\n";
        
        if (result.find("CONFLICT") != std::string::npos) {
            std::wcout << L"Возникли конфликты при слиянии!\n";
            std::wcout << L"Выберите действие:\n";
            std::wcout << L"1. Отменить слияние\n";
            std::wcout << L"2. Разрешить конфликты вручную (откроется редактор)\n";
            std::wcout << L"3. Принять изменения из текущей ветки (--ours)\n";
            std::wcout << L"4. Принять изменения из сливаемой ветки (--theirs)\n";
            std::wcout << L"Ваш выбор (или 'home' для отмены): ";
            
            std::wstring conflictChoiceStr;
            std::getline(std::wcin, conflictChoiceStr);
            
            if (checkForHomeCommand(conflictChoiceStr)) {
                // Отменяем слияние перед выходом
                std::string abortCmd = (mergeStrategy == 2) ? "git rebase --abort" : "git merge --abort";
                
                if (workDir.empty()) {
                    executeCommand(abortCmd);
                } else {
                    executeCommandInDirectory(abortCmd, workDir);
                }
                
                std::wcout << L"Слияние отменено.\n";
                return;
            }
            
            int conflictChoice;
            try {
                conflictChoice = std::stoi(conflictChoiceStr);
            } catch (const std::exception&) {
                std::wcout << L"Неверный ввод. Слияние будет отменено.\n";
                conflictChoice = 1;
            }
            
            std::string conflictCmd;
            switch (conflictChoice) {
                case 2:
                    // Открываем редактор для ручного разрешения конфликтов
                    conflictCmd = "git mergetool";
                    break;
                case 3:
                    // Принимаем изменения из текущей ветки
                    conflictCmd = "git checkout --ours .";
                    break;
                case 4:
                    // Принимаем изменения из сливаемой ветки
                    conflictCmd = "git checkout --theirs .";
                    break;
                default:
                    // Отменяем слияние
                    conflictCmd = (mergeStrategy == 2) ? "git rebase --abort" : "git merge --abort";
                    break;
            }
            
            if (workDir.empty()) {
                result = executeCommand(conflictCmd);
            } else {
                result = executeCommandInDirectory(conflictCmd, workDir);
            }
            
            std::wcout << stringToWstring(result) << L"\n";
            
            if (conflictChoice == 1) {
                std::wcout << L"Слияние отменено.\n";
            } else if (conflictChoice == 2) {
                std::wcout << L"После разрешения конфликтов вручную, не забудьте выполнить:\n";
                std::wcout << L"1. git add . (добавить разрешенные файлы)\n";
                std::wcout << L"2. git " << (mergeStrategy == 2 ? L"rebase --continue" : L"merge --continue") << L" (завершить слияние)\n";
            } else if (conflictChoice == 3 || conflictChoice == 4) {
                // Добавляем файлы и завершаем слияние
                std::string addFilesCmd = "git add .";
                
                if (workDir.empty()) {
                    executeCommand(addFilesCmd);
                } else {
                    executeCommandInDirectory(addFilesCmd, workDir);
                }
                
                std::string continueCmd = (mergeStrategy == 2) ? "git rebase --continue" : "git commit -m \"Merge branch '" + wstringToString(sourceBranch) + "' into " + currentBranch + "\"";
                
                if (workDir.empty()) {
                    result = executeCommand(continueCmd);
                } else {
                    result = executeCommandInDirectory(continueCmd, workDir);
                }
                
                std::wcout << stringToWstring(result) << L"\n";
                std::wcout << L"Слияние завершено.\n";
            }
        } else {
            std::wcout << L"Слияние выполнено успешно!\n";
            
            // Спрашиваем, хочет ли пользователь отправить изменения на удаленный репозиторий
            std::wcout << L"Хотите отправить изменения на удаленный репозиторий? (д/н): ";
            
            wchar_t pushChoice;
            std::wcin >> pushChoice;
            std::wcin.ignore();
            
            if (pushChoice == L'д' || pushChoice == L'Д') {
                std::string pushCmd = "git push";
                
                if (workDir.empty()) {
                    result = executeCommand(pushCmd);
                } else {
                    result = executeCommandInDirectory(pushCmd, workDir);
                }
                
                std::wcout << stringToWstring(result) << L"\n";
            }
        }
    }
    
    // Удалить ветку
    void deleteBranch() {
        std::string workDir = "";
        
        if (!isGitRepository()) {
            // Используем диалог выбора директории вместо ручного ввода
            std::wcout << L"Текущая директория не является Git репозиторием.\n";
            std::wcout << L"Выберите директорию с Git репозиторием...\n";
            std::wstring path = openFolderDialog(L"Выберите директорию с Git репозиторием");
            
            if (path.empty()) {
                std::wcout << L"Выбор директории отменен.\n";
                return;
            }
            
            workDir = wstringToString(path);
            
            // Проверяем, является ли директория Git репозиторием
            if (!isGitRepository(workDir)) {
                std::wcout << L"Выбранная директория не является Git репозиторием!\n";
                return;
            }
        }
        
        // Проверяем, настроен ли удаленный репозиторий
        std::string remoteCmd = "git remote -v";
        std::string remoteResult;
        
        if (workDir.empty()) {
            remoteResult = executeCommand(remoteCmd);
        } else {
            remoteResult = executeCommandInDirectory(remoteCmd, workDir);
        }
        
        bool hasRemote = !(remoteResult.empty() || remoteResult.find("origin") == std::string::npos);
        
        // Показать список веток
        std::string branches;
        if (workDir.empty()) {
            branches = executeCommand("git branch");
        } else {
            branches = executeCommandInDirectory("git branch", workDir);
        }
        
        std::wcout << L"Доступные ветки:\n" << stringToWstring(branches) << L"\n";
        
        std::wstring branchName;
        std::wcout << L"Введите имя ветки для удаления (или 'home' для возврата в меню): ";
        std::getline(std::wcin, branchName);
        
        if (checkForHomeCommand(branchName)) {
            return;
        }
        
        if (branchName.empty()) {
            std::wcout << L"Имя ветки не может быть пустым!\n";
            return;
        }
        
        // Проверить, не является ли ветка текущей
        std::string currentBranch;
        if (workDir.empty()) {
            currentBranch = executeCommand("git branch --show-current");
        } else {
            currentBranch = executeCommandInDirectory("git branch --show-current", workDir);
        }
        
        // Удаляем символы новой строки из конца строки
        if (!currentBranch.empty()) {
            size_t pos = currentBranch.find_last_not_of("\r\n");
            if (pos != std::string::npos) {
                currentBranch.erase(pos + 1);
            } else {
                currentBranch.clear();
            }
        }
        
        if (wstringToString(branchName) == currentBranch) {
            std::wcout << L"Невозможно удалить текущую ветку. Пожалуйста, переключитесь на другую ветку!\n";
            return;
        }
        
        std::wcout << L"Выберите тип удаления:\n";
        std::wcout << L"1. Безопасное удаление (только если ветка слита)\n";
        std::wcout << L"2. Принудительное удаление (удалит даже неслитые изменения)\n";
        std::wcout << L"Ваш выбор (или 'home' для возврата в меню): ";
        
        std::wstring choiceStr;
        std::getline(std::wcin, choiceStr);
        
        if (checkForHomeCommand(choiceStr)) {
            return;
        }
        
        int choice;
        try {
            choice = std::stoi(choiceStr);
        } catch (const std::exception&) {
            std::wcout << L"Неверный ввод. Отмена операции.\n";
            return;
        }
        
        std::string deleteFlag = (choice == 2) ? "-D" : "-d";
        std::string cmd = "git branch " + deleteFlag + " " + wstringToString(branchName);
        std::string result;
        
        if (workDir.empty()) {
            result = executeCommand(cmd);
        } else {
            result = executeCommandInDirectory(cmd, workDir);
        }
        
        std::wcout << stringToWstring(result) << L"\n";
        
        // Если ветка успешно удалена локально и есть удаленный репозиторий, спрашиваем про удаление на GitHub
        if (result.find("error") == std::string::npos && result.find("fatal") == std::string::npos && hasRemote) {
            std::wcout << L"Хотите удалить ветку на GitHub? (д/н): ";
            wchar_t remoteChoice;
            std::wcin >> remoteChoice;
            std::wcin.ignore();
            
            if (remoteChoice == L'д' || remoteChoice == L'Д') {
                std::string pushCmd = "git push origin --delete " + wstringToString(branchName);
                
                if (workDir.empty()) {
                    result = executeCommand(pushCmd);
                } else {
                    result = executeCommandInDirectory(pushCmd, workDir);
                }
                
                std::wcout << stringToWstring(result) << L"\n";
                std::wcout << L"Ветка удалена на GitHub!\n";
            }
        }
    }

    // Показать главное меню
    void showMainMenu() {
        int choice;
        std::wstring input;
        do {
            std::wcout << L"\n===== Меню GitHub Manager =====" << std::endl;
            std::wcout << L"1. Создать полный проект (репозиторий + локальный проект)" << std::endl;
            std::wcout << L"2. Создать репозиторий GitHub" << std::endl;
            std::wcout << L"3. Инициализировать локальный репозиторий" << std::endl;
            std::wcout << L"4. Связать локальный и удаленный репозитории" << std::endl;
            std::wcout << L"5. Создать файл README.md" << std::endl;
            std::wcout << L"6. Добавить файлы в индекс" << std::endl;
            std::wcout << L"7. Создать коммит" << std::endl;
            std::wcout << L"8. Отправить изменения на GitHub" << std::endl;
            std::wcout << L"9. Сохранить имя пользователя" << std::endl;
            std::wcout << L"10. Открыть GitHub в браузере" << std::endl;
            std::wcout << L"11. Настроить информацию Git" << std::endl;
            std::wcout << L"12. Просмотреть список репозиториев" << std::endl;
            std::wcout << L"13. Клонировать репозиторий" << std::endl;
            std::wcout << L"14. Удалить репозиторий" << std::endl;
            std::wcout << L"15. Создать задачу (issue)" << std::endl;
            std::wcout << L"16. Загрузить файлы из директории в репозиторий" << std::endl;
            std::wcout << L"17. Загрузить выбранные файлы в репозиторий" << std::endl;
            std::wcout << L"18. Создать файл .gitignore" << std::endl;
            std::wcout << L"\n===== Операции с ветками =====" << std::endl;
            std::wcout << L"19. Показать текущую ветку" << std::endl;
            std::wcout << L"20. Просмотреть список веток" << std::endl;
            std::wcout << L"21. Создать новую ветку" << std::endl;
            std::wcout << L"22. Переключиться на другую ветку" << std::endl;
            std::wcout << L"23. Создать и переключиться на новую ветку" << std::endl;
            std::wcout << L"24. Слить ветки" << std::endl;
            std::wcout << L"25. Удалить ветку" << std::endl;
            std::wcout << L"26. Отправить ветку на GitHub" << std::endl;
            std::wcout << L"27. Посмотреть историю ветки" << std::endl;
            std::wcout << L"28. Переименовать ветку" << std::endl;
            std::wcout << L"\n0. Выход" << std::endl;
            std::wcout << L"Выберите опцию: ";
            
            std::getline(std::wcin, input);
            
            // Проверка на ввод "home" или подобное
            if (checkForHomeCommand(input)) {
                continue; // Просто показываем меню снова
            }
            
            try {
                choice = std::stoi(input);
            } catch (const std::exception&) {
                std::wcout << L"Неверный ввод. Пожалуйста, введите число.\n";
                continue;
            }
            
            switch (choice) {
                case 0:
                    std::wcout << L"Выход из программы...\n";
                    break;
                case 1:
                    createFullProject();
                    break;
                case 2:
                    createGitHubRepository();
                    break;
                case 3:
                    initLocalRepository();
                    break;
                case 4:
                    linkRepositories();
                    break;
                case 5:
                    createReadmeFile();
                    break;
                case 6:
                    addToIndex();
                    break;
                case 7:
                    createCommit();
                    break;
                case 8:
                    pushChanges();
                    break;
                case 9:
                    saveUsername();
                    break;
                case 10:
                    openGitHubInBrowser();
                    break;
                case 11:
                    configureGitInfo();
                    break;
                case 12:
                    listRepositories();
                    break;
                case 13:
                    cloneRepository();
                    break;
                case 14:
                    deleteRepository();
                    break;
                case 15:
                    createIssue();
                    break;
                case 16:
                    uploadDirectoryFiles();
                    break;
                case 17:
                    uploadSelectedFiles();
                    break;
                case 18:
                    createGitignore();
                    break;
                case 19:
                    showCurrentBranch();
                    break;
                case 20:
                    listBranches();
                    break;
                case 21:
                    createBranch();
                    break;
                case 22:
                    switchBranch();
                    break;
                case 23:
                    createAndSwitchBranch();
                    break;
                case 24:
                    mergeBranches();
                    break;
                case 25:
                    deleteBranch();
                    break;
                case 26:
                    pushBranch();
                    break;
                case 27:
                    viewBranchHistory();
                    break;
                case 28:
                    renameBranch();
                    break;
                default:
                    std::wcout << L"Неверный выбор. Пожалуйста, выберите опцию из меню.\n";
            }
        } while (choice != 0);
    }

    // Отправить ветку на GitHub
    void pushBranch() {
        std::string workDir = "";
        
        if (!isGitRepository()) {
            workDir = requestGitDirectory();
            if (workDir.empty()) {
                return; // Пользователь отменил операцию или ввел 'home'
            }
        }
        
        // Проверяем, есть ли коммиты в репозитории
        if (!hasCommits(workDir)) {
            if (!createInitialCommit(workDir)) {
                return; // Пользователь отказался создавать коммит
            }
        }
        
        // Показать текущую ветку
        std::string currentBranch;
        if (workDir.empty()) {
            currentBranch = executeCommand("git branch --show-current");
        } else {
            currentBranch = executeCommandInDirectory("git branch --show-current", workDir);
        }
        
        currentBranch.erase(currentBranch.find_last_not_of("\r\n") + 1);
        
        std::wstring branchName;
        std::wcout << L"Введите имя ветки для отправки (пустое значение для текущей ветки '" 
                  << stringToWstring(currentBranch) << L"', или 'home' для возврата в меню): ";
        std::getline(std::wcin, branchName);
        
        if (checkForHomeCommand(branchName)) {
            return;
        }
        
        if (branchName.empty()) {
            branchName = stringToWstring(currentBranch);
        }
        
        std::wcout << L"Выполняется отправка ветки '" << branchName << L"' на GitHub...\n";
        
        std::string cmd = "git push -u origin " + wstringToString(branchName);
        std::string result;
        
        if (workDir.empty()) {
            result = executeCommand(cmd);
        } else {
            result = executeCommandInDirectory(cmd, workDir);
        }
        
        std::wcout << stringToWstring(result) << L"\n";
        
        if (result.find("error") != std::string::npos || result.find("fatal") != std::string::npos) {
            std::wcout << L"Произошла ошибка при отправке ветки!\n";
        } else {
            std::wcout << L"Ветка '" << branchName << L"' успешно отправлена на GitHub!\n";
        }
    }
    
    // Посмотреть историю ветки
    void viewBranchHistory() {
        std::string workDir = "";
        
        if (!isGitRepository()) {
            workDir = requestGitDirectory();
            if (workDir.empty()) {
                return; // Пользователь отменил операцию или ввел 'home'
            }
        }
        
        // Проверяем, есть ли коммиты в репозитории
        if (!hasCommits(workDir)) {
            if (!createInitialCommit(workDir)) {
                return; // Пользователь отказался создавать коммит
            }
        }
        
        std::wstring branchName;
        std::wcout << L"Введите имя ветки для просмотра истории (пустое значение для текущей ветки, или 'home' для возврата в меню): ";
        std::getline(std::wcin, branchName);
        
        if (checkForHomeCommand(branchName)) {
            return;
        }
        
        std::wstring count;
        std::wcout << L"Введите количество коммитов для отображения (пустое значение для всех, или 'home' для возврата в меню): ";
        std::getline(std::wcin, count);
        
        if (checkForHomeCommand(count)) {
            return;
        }
        
        std::string cmd = "git log";
        
        if (!branchName.empty()) {
            cmd += " " + wstringToString(branchName);
        }
        
        if (!count.empty()) {
            cmd += " -n " + wstringToString(count);
        }
        
        cmd += " --oneline --graph --decorate";
        
        std::string result;
        if (workDir.empty()) {
            result = executeCommand(cmd + " | cat");
        } else {
            result = executeCommandInDirectory(cmd + " | cat", workDir);
        }
        
        std::wcout << stringToWstring(result) << L"\n";
    }
    
    // Переименовать ветку
    void renameBranch() {
        std::string workDir = "";
        
        if (!isGitRepository()) {
            workDir = requestGitDirectory();
            if (workDir.empty()) {
                return; // Пользователь отменил операцию или ввел 'home'
            }
        }
        
        // Проверяем, есть ли коммиты в репозитории
        if (!hasCommits(workDir)) {
            if (!createInitialCommit(workDir)) {
                return; // Пользователь отказался создавать коммит
            }
        }
        
        // Показать текущую ветку
        std::string currentBranch;
        if (workDir.empty()) {
            currentBranch = executeCommand("git branch --show-current");
        } else {
            currentBranch = executeCommandInDirectory("git branch --show-current", workDir);
        }
        
        currentBranch.erase(currentBranch.find_last_not_of("\r\n") + 1);
        std::wcout << L"Текущая ветка: " << stringToWstring(currentBranch) << L"\n";
        
        std::wstring oldName;
        std::wcout << L"Введите имя ветки для переименования (пустое значение для текущей ветки, или 'home' для возврата в меню): ";
        std::getline(std::wcin, oldName);
        
        if (checkForHomeCommand(oldName)) {
            return;
        }
        
        if (oldName.empty()) {
            oldName = stringToWstring(currentBranch);
        }
        
        std::wstring newName;
        std::wcout << L"Введите новое имя для ветки '" << oldName << L"' (или 'home' для возврата в меню): ";
        std::getline(std::wcin, newName);
        
        if (checkForHomeCommand(newName)) {
            return;
        }
        
        if (newName.empty()) {
            std::wcout << L"Новое имя ветки не может быть пустым!\n";
            return;
        }
        
        std::string cmd;
        if (wstringToString(oldName) == currentBranch) {
            // Переименовать текущую ветку
            cmd = "git branch -m " + wstringToString(newName);
        } else {
            // Переименовать другую ветку
            cmd = "git branch -m " + wstringToString(oldName) + " " + wstringToString(newName);
        }
        
        std::string result;
        if (workDir.empty()) {
            result = executeCommand(cmd);
        } else {
            result = executeCommandInDirectory(cmd, workDir);
        }
        
        if (result.empty()) {
            std::wcout << L"Ветка успешно переименована из '" << oldName << L"' в '" << newName << L"'!\n";
        } else {
            std::wcout << stringToWstring(result) << L"\n";
        }
    }
    
    // Загрузить выбранные файлы в репозиторий
    void uploadSelectedFiles() {
        std::string workDir = "";
        
        // Проверяем, находимся ли мы в Git репозитории
        if (!isGitRepository()) {
            std::wcout << L"Текущая директория не является Git репозиторием.\n";
            std::wcout << L"Хотите указать путь к существующему репозиторию или создать новый? (с - существующий, н - новый, home - отмена): ";
            wchar_t choice;
            std::wcin >> choice;
            std::wcin.ignore();
            
            if (choice == L'с' || choice == L'С') {
                workDir = requestGitDirectory();
                if (workDir.empty()) {
                    return; // Пользователь отменил операцию или ввел 'home'
                }
            } else if (choice == L'н' || choice == L'Н') {
                std::wcout << L"Введите путь для создания нового репозитория: ";
                std::wstring path;
                std::getline(std::wcin, path);
                
                if (checkForHomeCommand(path)) {
                    return;
                }
                
                std::string pathStr = wstringToString(path);
                
                // Создаем директорию, если она не существует
                if (!std::filesystem::exists(pathStr)) {
                    std::filesystem::create_directories(pathStr);
                    std::wcout << L"Директория создана: " << path << L"\n";
                }
                
                // Инициализируем Git репозиторий
                std::string initCmd = "git init";
                std::string result = executeCommandInDirectory(initCmd, pathStr);
                std::wcout << stringToWstring(result) << L"\n";
                std::wcout << L"Git репозиторий инициализирован в директории: " << path << L"\n";
                
                workDir = pathStr;
            } else {
                return; // Отмена операции
            }
        }
        
        // Запрашиваем исходную директорию с файлами
        std::wcout << L"Введите путь к директории с исходными файлами (или 'home' для возврата в меню): ";
        std::wstring sourceDirPath;
        std::getline(std::wcin, sourceDirPath);
        
        if (checkForHomeCommand(sourceDirPath)) {
            return;
        }
        
        std::string sourceDirStr = wstringToString(sourceDirPath);
        
        if (!std::filesystem::exists(sourceDirStr)) {
            std::wcout << L"Указанная директория не существует!\n";
            return;
        }
        
        // Показываем список файлов в директории
        std::wcout << L"Файлы в директории " << sourceDirPath << L":\n";
        
        std::vector<std::filesystem::path> allFiles;
        try {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(sourceDirStr)) {
                if (entry.is_regular_file()) {
                    allFiles.push_back(entry.path());
                    std::wcout << allFiles.size() << L". " << entry.path().wstring() << L"\n";
                }
            }
        } catch (const std::exception& e) {
            std::wcout << L"Ошибка при чтении директории: " << stringToWstring(e.what()) << L"\n";
            return;
        }
        
        if (allFiles.empty()) {
            std::wcout << L"В указанной директории нет файлов.\n";
            return;
        }
        
        // Запрашиваем выбор файлов
        std::wcout << L"Выберите файлы для загрузки (введите номера через запятую, 'all' для всех, или 'home' для отмены): ";
        std::wstring selection;
        std::getline(std::wcin, selection);
        
        if (checkForHomeCommand(selection)) {
            return;
        }
        
        std::vector<std::filesystem::path> selectedFiles;
        
        if (selection == L"all" || selection == L"ALL" || selection == L"все" || selection == L"ВСЕ") {
            selectedFiles = allFiles;
        } else {
            std::wstringstream ss(selection);
            std::wstring item;
            
            while (std::getline(ss, item, L',')) {
                try {
                    int index = std::stoi(item);
                    if (index > 0 && index <= static_cast<int>(allFiles.size())) {
                        selectedFiles.push_back(allFiles[index - 1]);
                    } else {
                        std::wcout << L"Игнорирование недопустимого индекса: " << index << L"\n";
                    }
                } catch (const std::exception&) {
                    std::wcout << L"Игнорирование недопустимого ввода: " << item << L"\n";
                }
            }
        }
        
        if (selectedFiles.empty()) {
            std::wcout << L"Не выбрано ни одного файла для загрузки.\n";
            return;
        }
        
        std::wcout << L"Выбрано " << selectedFiles.size() << L" файлов.\n";
        
        // Запрашиваем структуру директорий в репозитории
        std::wcout << L"Сохранить структуру директорий? (д/н): ";
        wchar_t preserveStructure;
        std::wcin >> preserveStructure;
        std::wcin.ignore();
        
        bool keepStructure = (preserveStructure == L'д' || preserveStructure == L'Д');
        
        // Запрашиваем целевую директорию в репозитории
        std::wstring targetPath;
        std::wcout << L"Введите целевой путь в репозитории (пустое значение для корня): ";
        std::getline(std::wcin, targetPath);
        
        std::string targetPathStr = wstringToString(targetPath);
        
        // Создаем целевую директорию, если она не существует
        if (!targetPath.empty()) {
            std::string fullTargetPath = workDir.empty() ? 
                                        targetPathStr : 
                                        workDir + "/" + targetPathStr;
            
            if (!std::filesystem::exists(fullTargetPath)) {
                std::filesystem::create_directories(fullTargetPath);
                std::wcout << L"Создана директория: " << stringToWstring(fullTargetPath) << L"\n";
            }
        }
        
        // Копирование файлов
        int copiedCount = 0;
        for (const auto& src : selectedFiles) {
            std::filesystem::path relativePath;
            
            if (keepStructure) {
                // Получаем относительный путь от исходной директории
                relativePath = std::filesystem::relative(src, sourceDirStr);
            } else {
                // Используем только имя файла
                relativePath = src.filename();
            }
            
            // Формируем полный целевой путь
            std::filesystem::path destPath;
            if (targetPath.empty()) {
                destPath = relativePath.string();
            } else {
                destPath = targetPathStr + "/" + relativePath.string();
            }
            
            // Создаем промежуточные директории, если нужно
            std::filesystem::path destDir = destPath.parent_path();
            if (!destDir.empty() && !std::filesystem::exists(destDir)) {
                std::filesystem::create_directories(destDir);
            }
            
            try {
                std::filesystem::copy_file(src, destPath, 
                                          std::filesystem::copy_options::overwrite_existing);
                std::wcout << L"Файл скопирован: " << src.wstring() << L" -> " << destPath.wstring() << L"\n";
                copiedCount++;
            } catch (const std::exception& e) {
                std::wcout << L"Ошибка при копировании файла " << src.wstring() << L": " 
                          << stringToWstring(e.what()) << L"\n";
            }
        }
        
        std::wcout << L"Скопировано " << copiedCount << L" из " << selectedFiles.size() << L" файлов.\n";
        
        if (copiedCount == 0) {
            std::wcout << L"Не удалось скопировать ни одного файла. Операция отменена.\n";
            return;
        }
        
        // Добавление файлов в Git
        std::string addCmd = "git add .";
        std::string result;
        
        if (workDir.empty()) {
            result = executeCommand(addCmd);
        } else {
            result = executeCommandInDirectory(addCmd, workDir);
        }
        
        // Запрашиваем сообщение коммита
        std::wcout << L"Введите сообщение для коммита (или 'home' для отмены): ";
        std::wstring commitMessage;
        std::getline(std::wcin, commitMessage);
        
        if (checkForHomeCommand(commitMessage)) {
            return;
        }
        
        if (commitMessage.empty()) {
            commitMessage = L"Добавлены новые файлы";
        }
        
        // Создаем коммит
        std::string commitCmd = "git commit -m \"" + wstringToString(commitMessage) + "\"";
        
        if (workDir.empty()) {
            result = executeCommand(commitCmd);
        } else {
            result = executeCommandInDirectory(commitCmd, workDir);
        }
        
        std::wcout << stringToWstring(result) << L"\n";
        
        // Спрашиваем, хочет ли пользователь отправить изменения на GitHub
        std::wcout << L"Хотите отправить изменения на GitHub? (д/н): ";
        wchar_t pushChoice;
        std::wcin >> pushChoice;
        std::wcin.ignore();
        
        if (pushChoice == L'д' || pushChoice == L'Д') {
            // Проверяем, настроен ли удаленный репозиторий
            std::string remoteCmd = "git remote -v";
            std::string remoteResult;
            
            if (workDir.empty()) {
                remoteResult = executeCommand(remoteCmd);
            } else {
                remoteResult = executeCommandInDirectory(remoteCmd, workDir);
            }
            
            if (remoteResult.empty() || remoteResult.find("origin") == std::string::npos) {
                // Удаленный репозиторий не настроен
                std::wcout << L"Удаленный репозиторий не настроен. Хотите создать репозиторий на GitHub? (д/н): ";
                wchar_t createRepoChoice;
                std::wcin >> createRepoChoice;
                std::wcin.ignore();
                
                if (createRepoChoice == L'д' || createRepoChoice == L'Д') {
                    // Запрашиваем имя репозитория
                    std::wcout << L"Введите имя для нового репозитория на GitHub (или 'home' для отмены): ";
                    std::wstring repoName;
                    std::getline(std::wcin, repoName);
                    
                    if (checkForHomeCommand(repoName)) {
                        return;
                    }
                    
                    if (repoName.empty()) {
                        // Используем имя текущей директории
                        std::filesystem::path repoPath = workDir.empty() ? 
                                                       std::filesystem::current_path() : 
                                                       std::filesystem::path(workDir);
                        repoName = stringToWstring(repoPath.filename().string());
                    }
                    
                    // Создаем репозиторий на GitHub
                    std::wcout << L"Создание репозитория '" << repoName << L"' на GitHub...\n";
                    
                    // Запрашиваем тип репозитория
                    std::wcout << L"Сделать репозиторий приватным? (д/н): ";
                    wchar_t privateChoice;
                    std::wcin >> privateChoice;
                    std::wcin.ignore();
                    
                    bool isPrivate = (privateChoice == L'д' || privateChoice == L'Д');
                    
                    // Формируем команду для создания репозитория
                    std::string createRepoCmd = "gh repo create " + wstringToString(repoName) + 
                                              " --" + (isPrivate ? "private" : "public") + 
                                              " --source=. --remote=origin";
                    
                    std::string createRepoResult;
                    if (workDir.empty()) {
                        createRepoResult = executeCommand(createRepoCmd);
                    } else {
                        createRepoResult = executeCommandInDirectory(createRepoCmd, workDir);
                    }
                    
                    std::wcout << stringToWstring(createRepoResult) << L"\n";
                    
                    if (createRepoResult.find("Created repository") != std::string::npos ||
                        createRepoResult.find("HTTP 201") != std::string::npos) {
                        std::wcout << L"Репозиторий успешно создан на GitHub!\n";
                    } else {
                        std::wcout << L"Не удалось создать репозиторий на GitHub. Убедитесь, что GitHub CLI (gh) установлен и вы авторизованы.\n";
                        return;
                    }
                } else {
                    std::wcout << L"Введите URL удаленного репозитория (или 'home' для отмены): ";
                    std::wstring remoteUrl;
                    std::getline(std::wcin, remoteUrl);
                    
                    if (checkForHomeCommand(remoteUrl)) {
                        return;
                    }
                    
                    if (remoteUrl.empty()) {
                        std::wcout << L"URL не может быть пустым. Отмена операции.\n";
                        return;
                    }
                    
                    // Добавляем удаленный репозиторий
                    std::string addRemoteCmd = "git remote add origin " + wstringToString(remoteUrl);
                    
                    if (workDir.empty()) {
                        result = executeCommand(addRemoteCmd);
                    } else {
                        result = executeCommandInDirectory(addRemoteCmd, workDir);
                    }
                    
                    if (!result.empty()) {
                        std::wcout << stringToWstring(result) << L"\n";
                        std::wcout << L"Не удалось добавить удаленный репозиторий.\n";
                        return;
                    }
                }
            }
            
            // Отправляем изменения на GitHub
            std::string pushCmd = "git push -u origin master";
            
            // Проверяем, какая ветка текущая
            std::string currentBranchCmd = "git branch --show-current";
            std::string currentBranch;
            
            if (workDir.empty()) {
                currentBranch = executeCommand(currentBranchCmd);
            } else {
                currentBranch = executeCommandInDirectory(currentBranchCmd, workDir);
            }
            
            currentBranch.erase(currentBranch.find_last_not_of("\r\n") + 1);
            
            if (!currentBranch.empty() && currentBranch != "master") {
                pushCmd = "git push -u origin " + currentBranch;
            }
            
            if (workDir.empty()) {
                result = executeCommand(pushCmd);
            } else {
                result = executeCommandInDirectory(pushCmd, workDir);
            }
            
            std::wcout << stringToWstring(result) << L"\n";
            
            if (result.find("error") != std::string::npos || result.find("fatal") != std::string::npos) {
                std::wcout << L"Произошла ошибка при отправке изменений на GitHub.\n";
            } else {
                std::wcout << L"Изменения успешно отправлены на GitHub!\n";
            }
        }
    }
};

int main() {
    // Настройка локали для корректного отображения русских символов
    std::locale::global(std::locale(""));
    
    GitHubManager manager;
    manager.showMainMenu();
    
    return 0;
} 
#include <windows.h>
#include <psapi.h>
#include <commctrl.h>
#include <iostream>
#include <string>
#include <vector>
#include <string>
#include <tlhelp32.h>
#include <tchar.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "kernel32.lib")


LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void ListProcesses(HWND hwnd);
void TerminateSelectedProcess(DWORD processId);
void SuspendResumeSelectedProcess(DWORD processId, bool suspend);

std::vector<std::wstring> processListTextLines;
HMENU hMenu;

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    INITCOMMONCONTROLSEX iccex;
    iccex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    iccex.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&iccex);

    WNDCLASS wc = {};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"MemoryMonitorApp";
    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0,
        L"MemoryMonitorApp",
        L"Мониторинг системной памяти",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL) {
        return 0;
    }

    HMENU hMenu = CreatePopupMenu();
    AppendMenu(hMenu, MF_STRING, 101, L"Приостановить");
    AppendMenu(hMenu, MF_STRING, 102, L"Возобновить");
    AppendMenu(hMenu, MF_STRING, 103, L"Завершить");

    SetMenu(hwnd, hMenu);

    CreateWindow(
        L"BUTTON",
        L"Обновить",
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        10, 10, 80, 30,
        hwnd, (HMENU)1, hInstance, NULL);

    HWND hList = CreateWindowEx(
        0,
        WC_LISTVIEW,
        L"",
        WS_VISIBLE | WS_CHILD | LVS_REPORT | LVS_EDITLABELS,
        10, 50, 760, 500,
        hwnd,
        (HMENU)2,
        GetModuleHandle(NULL),
        NULL);

    DWORD dwStyle = GetWindowLong(hList, GWL_STYLE);
    dwStyle |= LVS_REPORT;
    SetWindowLong(hList, GWL_STYLE, dwStyle);

    LVCOLUMN lvc;
    lvc.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;

    lvc.iSubItem = 0;
    wchar_t pidText[] = L"PID";
    lvc.pszText = pidText;
    lvc.cx = 100;
    ListView_InsertColumn(hList, 0, &lvc);

    lvc.iSubItem = 1;
    wchar_t memoryText[] = L"Занято памяти (KB)";
    lvc.pszText = memoryText;
    lvc.cx = 200;
    ListView_InsertColumn(hList, 1, &lvc);

    lvc.iSubItem = 2;
    wchar_t processText[] = L"Название процесса";
    lvc.pszText = processText;
    lvc.cx = 400;
    ListView_InsertColumn(hList, 2, &lvc);

    ShowWindow(hwnd, nCmdShow);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    processListTextLines.clear();

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    HWND hList;
    int selectedIdx;

    switch (uMsg) {
    case WM_CREATE: {
        hMenu = CreatePopupMenu();
        AppendMenu(hMenu, MF_STRING, 101, L"Приостановить");
        AppendMenu(hMenu, MF_STRING, 102, L"Возобновить");
        AppendMenu(hMenu, MF_STRING, 103, L"Завершить");
        break;
    }

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case 1:
            ListProcesses(hwnd);
            break;

        case 101: // Приостановить
            hList = GetDlgItem(hwnd, 2);
            selectedIdx = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (selectedIdx != -1) {
                WCHAR buffer[256];
                ListView_GetItemText(hList, selectedIdx, 0, buffer, 256);

                std::wstring wstr(buffer);
                std::string str(wstr.begin(), wstr.end());

                DWORD processId = std::stoi(str);
                SuspendResumeSelectedProcess(processId, true);
            }
            break;

        case 102: // Возобновить
            hList = GetDlgItem(hwnd, 2);
            selectedIdx = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (selectedIdx != -1) {
                WCHAR buffer[256];
                ListView_GetItemText(hList, selectedIdx, 0, buffer, 256);

                std::wstring wstr(buffer);
                std::string str(wstr.begin(), wstr.end());

                DWORD processId = std::stoi(str);
                SuspendResumeSelectedProcess(processId, false);
            }
            break;

        case 103: // Завершить
            hList = GetDlgItem(hwnd, 2);
            selectedIdx = ListView_GetNextItem(hList, -1, LVNI_SELECTED);
            if (selectedIdx != -1) {
                WCHAR buffer[256];   
                ListView_GetItemText(hList, selectedIdx, 0, buffer, 256);

                std::wstring wstr(buffer);
                std::string str(wstr.begin(), wstr.end());

                DWORD processId = std::stoi(str);                
                TerminateSelectedProcess(processId);
            }
            break;
        }
        break;

    case WM_CONTEXTMENU:
        if ((HWND)wParam == GetDlgItem(hwnd, 2)) {
            POINT pt;
            GetCursorPos(&pt);
            TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL);
        }
        break;

    case WM_DESTROY:
        processListTextLines.clear();
        PostQuitMessage(0);
        break;

    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }

    return 0;
}

void TerminateSelectedProcess(DWORD processId) {
    HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, processId);
    if (hProcess != NULL) {
        TerminateProcess(hProcess, 0);
        CloseHandle(hProcess);
    }
}

void SuspendResumeSelectedProcess(DWORD processId, bool suspend) {
    HANDLE hProcess = OpenProcess(PROCESS_SUSPEND_RESUME, FALSE, processId);
    if (hProcess != NULL) {
        HANDLE hThreadSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, processId);
        if (hThreadSnapshot != INVALID_HANDLE_VALUE) {
            THREADENTRY32 threadEntry;
            threadEntry.dwSize = sizeof(THREADENTRY32);

            if (Thread32First(hThreadSnapshot, &threadEntry)) {
                do {
                    if (threadEntry.th32OwnerProcessID == processId) {
                        HANDLE hThread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, threadEntry.th32ThreadID);
                        if (hThread != NULL) {
                            if (suspend) {
                                SuspendThread(hThread);
                            }
                            else {
                                ResumeThread(hThread);
                            }
                            CloseHandle(hThread);
                        }
                    }
                } while (Thread32Next(hThreadSnapshot, &threadEntry));
            }
            CloseHandle(hThreadSnapshot);
        }

        CloseHandle(hProcess);
    }
}

void ListProcesses(HWND hwnd) {
    HWND hList = GetDlgItem(hwnd, 2);

    // Очищаем предыдущий список процессов
    ListView_DeleteAllItems(hList);

    std::vector<DWORD> processes(1024);
    DWORD bytesNeeded;

    if (!EnumProcesses(processes.data(), processes.size() * sizeof(DWORD), &bytesNeeded)) {
        DWORD error = GetLastError();
        std::cerr << "EnumProcesses не удалось выполнить с ошибкой: " << error << "\n";
        std::cerr << "Требуемый размер буфера: " << bytesNeeded << " байт\n";
        return;
    }

    DWORD processCount = bytesNeeded / sizeof(DWORD);
    processes.resize(processCount);

    // Очищаем и заново выделяем память для строк
    processListTextLines.clear();
    size_t totalStringLength = processCount * 3;  // 3 строки на каждый процесс
    processListTextLines.resize(totalStringLength);

    LVITEM lvi = { 0 };
    lvi.mask = LVIF_TEXT;

    for (DWORD i = 0; i < processCount; ++i) {
        HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);

        if (process != NULL) {
            PROCESS_MEMORY_COUNTERS_EX pmc = {};
            if (GetProcessMemoryInfo(process, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
                TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
                DWORD dwSize = sizeof(szProcessName) / sizeof(TCHAR);
                if (QueryFullProcessImageName(process, 0, szProcessName, &dwSize)) {
                    processListTextLines[i * 3] = std::to_wstring(processes[i]);
                    processListTextLines[i * 3 + 1] = std::to_wstring(pmc.WorkingSetSize / 1024);
                    processListTextLines[i * 3 + 2] = szProcessName;

                    lvi.iItem = i;
                    lvi.iSubItem = 0;
                    lvi.pszText = const_cast<LPWSTR>(processListTextLines[i * 3].c_str());
                    int itemIndex = ListView_InsertItem(hList, &lvi);

                    lvi.iSubItem = 1;
                    lvi.pszText = const_cast<LPWSTR>(processListTextLines[i * 3 + 1].c_str());
                    ListView_SetItemText(hList, itemIndex, 1, lvi.pszText);

                    lvi.iSubItem = 2;
                    lvi.pszText = const_cast<LPWSTR>(processListTextLines[i * 3 + 2].c_str());
                    ListView_SetItemText(hList, itemIndex, 2, lvi.pszText);
                    UpdateWindow(hList);
                }
                else {
                    DWORD error = GetLastError();
                    std::cerr << "QueryFullProcessImageName не удалось выполнить с ошибкой: " << error << "\n";
                    std::cerr << "ID процесса: " << processes[i] << "\n";
                }
            }
            else {
                DWORD error = GetLastError();
                std::cerr << "GetProcessMemoryInfo не удалось выполнить с ошибкой: " << error << "\n";
                std::cerr << "ID процесса: " << processes[i] << "\n";
            }

            CloseHandle(process);
        }
        else {
            DWORD error = GetLastError();
            std::cerr << "OpenProcess не удалось выполнить с ошибкой: " << error << "\n";
            std::cerr << "ID процесса: " << processes[i] << "\n";
        }

        // Обновим lvi перед следующей вставкой
        ZeroMemory(&lvi, sizeof(lvi));
        lvi.mask = LVIF_TEXT;
    }
}

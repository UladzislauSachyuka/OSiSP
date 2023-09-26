#include <windows.h>
#include <vector>
#include <utility>

HINSTANCE hInst;                // экземпляр текущего приложения
HWND hWnd;                      // Дескриптор главного окна приложения
bool isDrawing = false;         // Флаг, указывающий, идет ли в данный момент процесс рисования (нажата ли кнопка мыши)
bool isRectangleMode = false;   // Флаг, определяющий текущий режим рисования (круг или прямоугольник)
POINT startPoint;               // Точка начала рисования
POINT endPoint;                 // Точка окончания рисования
RECT currentRectangle;          // Прямоугольник, описывающий текущую фигуру
bool currentIsRectangle;        // Флаг, указывающий, является ли текущая фигура прямоугольником
std::vector<std::pair<RECT, bool>> figures; // Вектор фигур с информацией о режиме (RECT) и типе (круг или прямоугольник)
bool isDragging = false; // Флаг, указывающий, происходит ли перетаскивание фигуры
int selectedFigureIndex = -1; // Индекс выбранной фигуры в векторе figures
bool isResizing = false;        // Флаг, указывающий, происходит ли изменение размера фигуры

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM); // функция которая обрабатывает сообщения окна
void DrawFigures(HDC hdc);     
void DrawEllipse(HDC hdc, const RECT& rect);          
bool IsCursorOnBorder(const POINT& cursor, const RECT& rect);

// Основная функция WinMain, точка входа в приложение
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance; // присваивается переданный экземпляр приложения

    WNDCLASSEX wcex;
    wcex.cbSize = sizeof(WNDCLASSEX); 
    wcex.style = CS_HREDRAW | CS_VREDRAW; 
    wcex.lpfnWndProc = WndProc; 
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance; 
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION); 
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);   
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); 
    wcex.lpszMenuName = NULL; 
    wcex.lpszClassName = L"GraphicsEditor"; 
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION); 

    if (!RegisterClassEx(&wcex)) 
    {
        MessageBox(NULL, L"Не удалось зарегистрировать класс окна", L"Ошибка", MB_ICONERROR);
        return 1;
    }

    hWnd = CreateWindow(L"GraphicsEditor", L"Графический редактор",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600, NULL, NULL, hInstance, NULL);

    if (!hWnd) 
    {
        MessageBox(NULL, L"Не удалось создать окно", L"Ошибка", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hWnd, nCmdShow); 
    UpdateWindow(hWnd); 

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg); // Преобразование сообщений клавиш в символы
        DispatchMessage(&msg); // Отправка сообщения на обработку
    }

    return (int)msg.wParam; 
}

// Функция обработки сообщений
LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps; // Структура для информации о рисовании
        HDC hdc = BeginPaint(hWnd, &ps); // Получение контекста устройства для рисования

        DrawFigures(hdc); // Вызов функции отрисовки фигур

        EndPaint(hWnd, &ps); // Завершение рисования и освобождение контекста
    }
    break;

    case WM_LBUTTONDOWN:
        isDrawing = true;
        startPoint.x = LOWORD(lParam); // Получение координаты X при нажатии левой кнопки мыши
        startPoint.y = HIWORD(lParam); // Получение координаты Y при нажатии левой кнопки мыши
        endPoint = startPoint;
        currentIsRectangle = isRectangleMode; // Присвоение флага режима рисования текущей фигуре

        for (int i = 0; i < figures.size(); ++i)
        {
            if (PtInRect(&figures[i].first, startPoint))
            {
                isDragging = true; // Начинаем перетаскивание
                selectedFigureIndex = i; // Запоминаем индекс выбранной фигуры

                // Проверяем, если курсор находится на границе фигуры, то начинаем изменение размера
                if (IsCursorOnBorder(startPoint, figures[i].first))
                {
                    isResizing = true;
                }

                break;
            }
        }

        break;

    case WM_MOUSEMOVE:
        if (isDrawing)
        {
            endPoint.x = LOWORD(lParam); // Обновление координаты X при движении мыши
            endPoint.y = HIWORD(lParam); // Обновление координаты Y при движении мыши
            /*int length = abs(endPoint.x - startPoint.x) < abs(endPoint.y - startPoint.y) ? abs(endPoint.x - startPoint.x) : abs(endPoint.y - startPoint.y);
            if (endPoint.x < startPoint.x)
            {
                endPoint.x = startPoint.x - length;
            }
            else
            {
                endPoint.x = startPoint.x + length;
            }

            if (endPoint.y < startPoint.y)
            {
                endPoint.y = startPoint.y - length;
            }
            else
            {
                endPoint.y = startPoint.y + length;
            }*/

            currentRectangle.left = startPoint.x; 
            currentRectangle.top = startPoint.y;
            currentRectangle.right = endPoint.x;  
            currentRectangle.bottom = endPoint.y; 

            HDC hdc = GetDC(hWnd); // Получаем контекст устройства для окна
            DrawFigures(hdc); // Отрисовываем все фигуры
            if (isRectangleMode) 
            {
                HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0)); // Создание красной кисти
                SelectObject(hdc, hBrush); // Выбор созданной кисти
                Rectangle(hdc, currentRectangle.left, currentRectangle.top, currentRectangle.right, currentRectangle.bottom);
                DeleteObject(hBrush); // Удаление созданной кисти
            }
            else 
                DrawEllipse(hdc, currentRectangle); 
            ReleaseDC(hWnd, hdc); // Освобождаем контекст устройства
        }
        if (isDragging && selectedFigureIndex != -1)
        {
            endPoint.x = LOWORD(lParam);
            endPoint.y = HIWORD(lParam);

            // Обновляем координаты выбранной фигуры для перетаскивания
            if (!isResizing)
            {
                // Перетаскивание фигуры
                int deltaX = endPoint.x - startPoint.x;
                int deltaY = endPoint.y - startPoint.y;
                figures[selectedFigureIndex].first.left += deltaX;
                figures[selectedFigureIndex].first.right += deltaX;
                figures[selectedFigureIndex].first.top += deltaY;
                figures[selectedFigureIndex].first.bottom += deltaY;
            }
            else
            {
                // Изменение размера фигуры
                figures[selectedFigureIndex].first.right = endPoint.x;
                figures[selectedFigureIndex].first.bottom = endPoint.y;
            }

            startPoint = endPoint; // Обновляем начальную точку
            InvalidateRect(hWnd, NULL, TRUE); // Запрос на перерисовку окна
        }
        break;

    case WM_LBUTTONUP:   // при отпускании левой кнопки
        if (isDrawing)
        {
            isDrawing = false;
            endPoint.x = LOWORD(lParam); // Получение координаты X при отпускании левой кнопки мыши
            endPoint.y = HIWORD(lParam); // Получение координаты Y при отпускании левой кнопки мыши
            /*int length = abs(endPoint.x - startPoint.x) < abs(endPoint.y - startPoint.y) ? abs(endPoint.x - startPoint.x) : abs(endPoint.y - startPoint.y);
            if (endPoint.x < startPoint.x)
            {
                endPoint.x = startPoint.x - length;
            }
            else
            {
                endPoint.x = startPoint.x + length;
            }

            if (endPoint.y < startPoint.y)
            {
                endPoint.y = startPoint.y - length;
            }
            else
            {
                endPoint.y = startPoint.y + length;
            }*/
            currentRectangle.left = startPoint.x; 
            currentRectangle.top = startPoint.y;  
            //currentRectangle.right = endPoint.x;  
            //currentRectangle.bottom = endPoint.y; 
            currentRectangle.right = abs(endPoint.y - startPoint.y) * 2 + startPoint.x;
            currentRectangle.bottom = endPoint.y;
            figures.push_back(std::make_pair(currentRectangle, currentIsRectangle)); // Добавление фигуры в вектор
            InvalidateRect(hWnd, NULL, TRUE); // Запрос на перерисовку окна
        }
        if (isDragging) 
        {
            isDragging = false; // Завершаем перетаскивание
            isResizing = false; // Завершаем изменение размера
            selectedFigureIndex = -1; // Отменяем выбор фигуры
        }
        break;

    case WM_KEYDOWN:
        if (wParam == VK_SPACE)
        {
            isRectangleMode = !isRectangleMode; // Переключение режима рисования
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0); 
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam); 
    }
    return 0;
}

// Функция для отрисовки фигур
void DrawFigures(HDC hdc)
{
    for (int i = 0; i < figures.size(); ++i)
    {
        if (figures[i].second) // Если фигура - прямоугольник
        {
            HBRUSH hBrush = CreateSolidBrush(RGB(255, 0, 0)); // Создание красной кисти
            SelectObject(hdc, hBrush); // Выбор созданной кисти
            Rectangle(hdc, figures[i].first.left, figures[i].first.top, figures[i].first.right, figures[i].first.bottom);
            DeleteObject(hBrush); 
        }
        else // Если фигура - круг
        {
            DrawEllipse(hdc, figures[i].first); 
        }
    }
}

// Функция для отрисовки круга
void DrawEllipse(HDC hdc, const RECT& rect)
{
    HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 255)); // Создание синей кисти
    SelectObject(hdc, hBrush); // Выбор созданной кисти
    Ellipse(hdc, rect.left, rect.top, rect.right, rect.bottom); 
    DeleteObject(hBrush); 
}

// Функция для проверки, находится ли курсор на границе фигуры
bool IsCursorOnBorder(const POINT& cursor, const RECT& rect)
{
    const int borderSize = 15; // Размер области для изменения размера
    if (cursor.x >= rect.left - borderSize && cursor.x <= rect.left + borderSize)
        return true; // Левая граница
    if (cursor.x >= rect.right - borderSize && cursor.x <= rect.right + borderSize)
        return true; // Правая граница
    if (cursor.y >= rect.top - borderSize && cursor.y <= rect.top + borderSize)
        return true; // Верхняя граница
    if (cursor.y >= rect.bottom - borderSize && cursor.y <= rect.bottom + borderSize)
        return true; // Нижняя граница
    return false;
}
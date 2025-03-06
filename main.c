#include <windows.h>
#include <gl/gl.h>
#include <stdbool.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb-master/stb_easy_font.h"
#include "stb-master/stb_image.h"

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
void EnableOpenGL(HWND hwnd, HDC*, HGLRC*);
void DisableOpenGL(HWND hwnd, HDC hdc, HGLRC hglrc);
void Render();

GLuint bgTexture = 0;  // Переменная для текстуры фона
bool showBackground = false; // Флаг, указывающий, показывать ли фон
GLuint playerTexture = 0; // Текстура спрайта игрока
bool showPlayer = false; // Флаг, показывающий, отображать ли спрайт

float playerPosX = 100.0f; // Позиция игрока по оси X
float playerPosY = 200.0f; // Позиция игрока по оси Y

// Добавим глобальные переменные
bool isMoving = false; // Флаг движения
bool isFacingRight = true; // Направление персонажа
float currentFrame = 0.0f; // Текущий кадр анимации
float animationSpeed = 0.2f; // Скорость анимации
float moveSpeed = 5.0f; // Скорость движения

GLuint startBgTexture = 0;  // Текстура начального фона

bool playButtonVisible = true;
bool pauseMenuVisible = false;

float velocityY = 0.0f;        // Вертикальная скорость
float gravity = 0.5f;          // Сила гравитации
float jumpForce = -12.0f;      // Сила прыжка (отрицательная, так как ось Y направлена вниз)
bool isJumping = false;        // Флаг прыжка
float groundLevel = 600.0f - 80.0f;    // 600 (высота окна) - 80 (высота спрайта)

GLuint LoadTexture(const char *filename)
{
    int width, height, channels;
    unsigned char *image = stbi_load(filename, &width, &height, &channels, 4); // Загружаем с 4 каналами (RGBA)

    if (!image) {
        printf("Ошибка загрузки изображения: %s\n", filename);
        return 0;
    }

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

    stbi_image_free(image);  // Освобождаем память

    return texture; // Возвращаем ID текстуры
}

// Структура кнопки
typedef struct
{
    char *name;      // Динамическая строка
    float vert[8];   // 4 вершины по 2 координаты
    float buffer[50 * 10]; // Буфер для координат символов
    int num_quads;   // Количество квадов для текста
    float textPosX, textPosY, textS; // Позиция и масштаб текста
} Button;

// Глобальный массив кнопок
Button *btn = NULL;
int btnCnt = 0;

// Добавление кнопки
int AddButton(const char *name, float x, float y, float textS, float padding)
{
    btnCnt++;
    btn = (Button*)realloc(btn, sizeof(Button) * btnCnt);
    if (!btn) return -1;

    Button *b = &btn[btnCnt - 1];

    // Выделяем память для имени кнопки
    b->name = (char*)malloc(strlen(name) + 1);
    if (!b->name) return -1;
    strcpy(b->name, name);

    // Получаем ширину и высоту текста
    float textWidth = stb_easy_font_width(name) * textS;
    float textHeight = stb_easy_font_height(name) * textS;

    // Рассчитываем размеры кнопки с дополнительным запасом
    float width = textWidth + padding * 3;
    float height = textHeight + padding * 3; // Увеличиваем высоту для запаса

    // Устанавливаем координаты кнопки
    float *vert = b->vert;
    vert[0] = vert[6] = x;
    vert[2] = vert[4] = x + width;
    vert[1] = vert[3] = y;
    vert[5] = vert[7] = y + height;

    // Генерируем текст для рендеринга
    b->num_quads = stb_easy_font_print(0, 0, name, 0, b->buffer, sizeof(b->buffer));

    // Вычисляем координаты текста
    b->textPosX = x + (width - textWidth) / 2.0; // Центрируем текст по X
    b->textPosY = y + (height - textHeight) / 2.0 + textS * 1.5; // Двигаем текст вверх

    b->textS = textS;

    return btnCnt - 1;
}

void FreeButtons()
{
    for (int i = 0; i < btnCnt; i++)
    {
        free(btn[i].name);
    }
    free(btn);
    btn = NULL;
    btnCnt = 0;
}

// Отрисовка кнопки
void ShowButton(int buttonId)
{
    if (buttonId < 0 || buttonId >= btnCnt) return;

    Button btn1 = btn[buttonId];

    glVertexPointer(2, GL_FLOAT, 0, btn1.vert);
    glEnableClientState(GL_VERTEX_ARRAY);

    glColor3f(0.2, 1, 0.2); // Цвет кнопки
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    glColor3f(0.5, 0.5, 0.5); // Обводка
    glLineWidth(1);
    glDrawArrays(GL_LINE_LOOP, 0, 4);

    glDisableClientState(GL_VERTEX_ARRAY);

    glPushMatrix();
    glColor3f(0.5, 0.5, 0.5);
    glTranslatef(btn1.textPosX, btn1.textPosY, 0);
    glScalef(btn1.textS, btn1.textS, 1);

    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(2, GL_FLOAT, 16, btn1.buffer);
    glDrawArrays(GL_QUADS, 0, btn1.num_quads * 4);
    glDisableClientState(GL_VERTEX_ARRAY);

    glPopMatrix();
}

void OnButtonClick(int buttonId)
{
    if (strcmp(btn[buttonId].name, "Play") == 0) {
        bgTexture = LoadTexture("background.png");
        playerTexture = LoadTexture("player_spritesheet.png");
        if (playerTexture == 0) {
            printf("Ошибка загрузки текстуры спрайта.\n");
        }
        showBackground = true;
        showPlayer = true;

        // Скрываем кнопку Play и показываем игровые кнопки
        playButtonVisible = false;
        pauseMenuVisible = true;

        // Удаляем старые кнопки и создаем новые
        FreeButtons();
        AddButton("Pause", 20, 20, 1, 10);
        AddButton("Reset", 20, 80, 1, 10);
        AddButton("Jump", 20, 140, 1, 10);
        AddButton("Start", 20, 200, 1, 10);
        AddButton("Exit", 20, 260, 1, 10);
    }
    else if (strcmp(btn[buttonId].name, "Exit") == 0) {
        PostQuitMessage(0);
    }
    else if (strcmp(btn[buttonId].name, "Pause") == 0) {
        printf("Game paused\n");
    }
    else if (strcmp(btn[buttonId].name, "Reset") == 0) {
        printf("Game reset requested\n");
    }
    else if (strcmp(btn[buttonId].name, "Jump") == 0) {
        // Прыгаем только если мы на земле
        if (playerPosY >= groundLevel) {
            velocityY = jumpForce;
            isJumping = true;
            printf("Jump executed\n");
        }
    }
    else if (strcmp(btn[buttonId].name, "Start") == 0) {
        printf("Unpausing game\n");
    }
}

void CheckMouseClick(float mouseX, float mouseY)
{
    for (int i = 0; i < btnCnt; i++) {
        float *vert = btn[i].vert;
        if (mouseX >= vert[0] && mouseX <= vert[4] && mouseY >= vert[1] && mouseY <= vert[5]) {
            OnButtonClick(i);
        }
    }
}

// Отображение фона
void ShowBackground()
{
    glEnable(GL_TEXTURE_2D);

    // Устанавливаем белый цвет (1,1,1) перед отрисовкой текстуры
    glColor3f(1.0f, 1.0f, 1.0f);

    // Выбираем текстуру в зависимости от состояния игры
    if (showPlayer) {
        glBindTexture(GL_TEXTURE_2D, bgTexture);
    } else {
        glBindTexture(GL_TEXTURE_2D, startBgTexture);
    }

    glBegin(GL_QUADS);
        glTexCoord2f(0, 0); glVertex2f(0, 0);
        glTexCoord2f(1, 0); glVertex2f(800, 0);
        glTexCoord2f(1, 1); glVertex2f(800, 600);
        glTexCoord2f(0, 1); glVertex2f(0, 600);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}

// Отображение спрайта игрока
void ShowPlayer()
{
    if (!showPlayer) return;

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glBindTexture(GL_TEXTURE_2D, playerTexture);

    float frameWidth = 800.0f / 7.0f;
    float frameHeight = 80.0f;

    // Вычисляем координаты текстуры для текущего кадра
    float texLeft = (int)currentFrame * (1.0f / 7.0f);
    float texRight = texLeft + (1.0f / 7.0f);

    // Отражаем текстуру по горизонтали если персонаж смотрит влево
    if (!isFacingRight) {
        float temp = texLeft;
        texLeft = texRight;
        texRight = temp;
    }

    glBegin(GL_QUADS);
        glTexCoord2f(texLeft, 0.0f);  glVertex2f(playerPosX, playerPosY);
        glTexCoord2f(texRight, 0.0f); glVertex2f(playerPosX + frameWidth, playerPosY);
        glTexCoord2f(texRight, 1.0f); glVertex2f(playerPosX + frameWidth, playerPosY + frameHeight);
        glTexCoord2f(texLeft, 1.0f);  glVertex2f(playerPosX, playerPosY + frameHeight);
    glEnd();

    glDisable(GL_BLEND);
    glDisable(GL_TEXTURE_2D);

    // Обновляем анимацию только если персонаж движется
    if (isMoving) {
        currentFrame += animationSpeed;
        if (currentFrame >= 7.0f) currentFrame = 0.0f;
    }
    else {
        currentFrame = 0;
    }
}

// Функция отрисовки сцены
void Render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    ShowBackground();

    // Отображаем кнопки только если они должны быть видимы
    if (playButtonVisible || pauseMenuVisible) {
        for (int i = 0; i < btnCnt; i++) ShowButton(i);
    }

    ShowPlayer();

    SwapBuffers(wglGetCurrentDC());
}

// Функция для инициализации OpenGL
void EnableOpenGL(HWND hwnd, HDC *hDC, HGLRC *hRC)
{
    PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
        PFD_TYPE_RGBA, 24, 0, 0, 0, 0, 0, 0, 0, 32, 0, 0, 0, 0, 0, 0, 0
    };

    *hDC = GetDC(hwnd);
    int pf = ChoosePixelFormat(*hDC, &pfd);
    SetPixelFormat(*hDC, pf, &pfd);
    *hRC = wglCreateContext(*hDC);
    wglMakeCurrent(*hDC, *hRC);
}

// Функция для деинициализации OpenGL
void DisableOpenGL(HWND hwnd, HDC hdc, HGLRC hglrc)
{
    wglMakeCurrent(NULL, NULL);
    wglDeleteContext(hglrc);
    ReleaseDC(hwnd, hdc);
}

// Основная программа
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    WNDCLASSEX wcex;
    HWND hwnd;
    HDC hDC;
    HGLRC hRC;
    MSG msg;
    BOOL bQuit = FALSE;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_OWNDC;
    wcex.lpfnWndProc = WindowProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = "GLSample";
    wcex.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wcex)) return 0;

    hwnd = CreateWindowEx(0, "GLSample", "OpenGL Buttons", WS_OVERLAPPEDWINDOW,
                          CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
                          NULL, NULL, hInstance, NULL);

    ShowWindow(hwnd, nCmdShow);
    EnableOpenGL(hwnd, &hDC, &hRC);

    // Установка 2D-координат
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 800, 600, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    // Загружаем начальный фон
    startBgTexture = LoadTexture("start_background.png");

    // Добавление кнопок
    AddButton("Play", 20, 20, 1, 10);
    playButtonVisible = true;
    pauseMenuVisible = false;

    while (!bQuit)
    {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) bQuit = TRUE;
            else {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        else
        {
            // Обработка движения
            isMoving = false;
            if (GetAsyncKeyState(VK_LEFT) & 0x8000) {
                playerPosX -= moveSpeed;
                isMoving = true;
                isFacingRight = false;
            }
            if (GetAsyncKeyState(VK_RIGHT) & 0x8000) {
                playerPosX += moveSpeed;
                isMoving = true;
                isFacingRight = true;
            }

            // Обработка прыжка на пробел
            if (GetAsyncKeyState(VK_SPACE) & 0x8000) {
                // Прыгаем только если мы на земле
                if (playerPosY >= groundLevel) {
                    velocityY = jumpForce;
                    isJumping = true;
                }
            }

            // Применяем гравитацию
            velocityY += gravity;
            playerPosY += velocityY;

            // Проверяем столкновение с землёй
            if (playerPosY >= groundLevel) {
                playerPosY = groundLevel;
                velocityY = 0;
                isJumping = false;
            }

            // Ограничение движения в пределах окна
            if (playerPosX < 0) playerPosX = 0;
            if (playerPosX > 800 - (800.0f/7.0f)) playerPosX = 800 - (800.0f/7.0f); // Учитываем ширину одного кадра
            if (playerPosY < 0) {
                playerPosY = 0;
                velocityY = 0;
            }
            if (playerPosY > groundLevel) {
                playerPosY = groundLevel;
                velocityY = 0;
                isJumping = false;
            }

            Render();
            Sleep(1);
        }
    }

    DisableOpenGL(hwnd, hDC, hRC);
    DestroyWindow(hwnd);
    FreeButtons();
    return msg.wParam;
}

// Обработчик событий окна
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_CLOSE:
            PostQuitMessage(0);
            break;

        case WM_DESTROY:
            return 0;

        case WM_KEYDOWN:
            {
                switch (wParam)
                {
                    case VK_ESCAPE:
                        PostQuitMessage(0);
                        break;
                }
            }
            break;

        case WM_LBUTTONDOWN:
            {
                float mouseX = LOWORD(lParam);
                float mouseY = HIWORD(lParam);
                CheckMouseClick(mouseX, mouseY);
            }
            break;

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

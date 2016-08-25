/*
* Copyright(c) 2015 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include "glrotate.h"
#include <Windows.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <GL/glew.h>
#include <gl/GL.h>
#include <strsafe.h>
#include "..\etw\jytrace.h"
#include <fstream>
#include <string>
#include <vector>
#include "..\include\libcore.h"
#include "..\include\libcamera.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "glew32.lib")

#if _DEBUG
#pragma comment(lib, "../out/libcore.lib")
#endif

#define CAMERA_FRIENDLY_NAME    L"Integrated Camera"
#define MAX_LOADSTRING          100
#define WIDTH                   640
#define HEIGHT                  480
#define RGB32_VGA_SIZE          WIDTH * HEIGHT * 4

HWND hWnd;
HINSTANCE hInst;
WCHAR szTitle[MAX_LOADSTRING];
WCHAR szWindowClass[MAX_LOADSTRING];
HDC hDC;
HGLRC hRC;
GLuint shader_program, vao, vbo, ebo, texture_id;
GLint attrib_position, uniform_angle;
GLfloat angle = 180.0f;
BOOL bIncrement = TRUE;
BYTE pFrame[RGB32_VGA_SIZE];

GLuint LoadShaders(const char *vertexFilePath, const char *fragmentFilePath)
{
    GLuint vertexShaderID = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

    std::string vertexShaderCode;
    std::ifstream vertexShaderStream(vertexFilePath, std::ios::in);

    if (vertexShaderStream.is_open())
    {
        std::string line = "";

        while (getline(vertexShaderStream, line))
            vertexShaderCode += "\n" + line;

        vertexShaderStream.close();
    }
    else
    {
        EventWriteInfoW(MD, FL, FN, L"Cannot open vextex shader.");
        return 0;
    }

    std::string fragmentShaderCode;
    std::ifstream fragmentShaderStream(fragmentFilePath, std::ios::in);

    if (fragmentShaderStream.is_open())
    {
        std::string line = "";

        while (getline(fragmentShaderStream, line))
            fragmentShaderCode += "\n" + line;

        fragmentShaderStream.close();
    }

    GLint result = GL_FALSE;
    int infoLogLength;

    EventWriteAnsiStrInfo(MD, FL, FN, L"Compile vertex shader", vertexFilePath);

    char const *vertexSourcePointer = vertexShaderCode.c_str();

    glShaderSource(vertexShaderID, 1, &vertexSourcePointer, NULL);
    glCompileShader(vertexShaderID);
    glGetShaderiv(vertexShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(vertexShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (infoLogLength > 0)
    {
        std::vector<char> vertexShaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(vertexShaderID, infoLogLength, NULL, &vertexShaderErrorMessage[0]);
        EventWriteAnsiStrInfo(MD, FL, FN, L"Compile vertex shader result", &vertexShaderErrorMessage[0]);
    }

    EventWriteAnsiStrInfo(MD, FL, FN, L"Compile fragment shader", fragmentFilePath);

    char const *fragmentSourcePointer = fragmentShaderCode.c_str();

    glShaderSource(fragmentShaderID, 1, &fragmentSourcePointer, NULL);
    glCompileShader(fragmentShaderID);
    glGetShaderiv(fragmentShaderID, GL_COMPILE_STATUS, &result);
    glGetShaderiv(fragmentShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (infoLogLength > 0)
    {
        std::vector<char> fragmentShaderErrorMessage(infoLogLength + 1);
        glGetShaderInfoLog(fragmentShaderID, infoLogLength, NULL, &fragmentShaderErrorMessage[0]);
        EventWriteAnsiStrInfo(MD, FL, FN, L"Compile fragment shader result", &fragmentShaderErrorMessage[0]);
    }

    EventWriteInfoW(MD, FL, FN, L"Linking shader program.");

    GLuint programId = glCreateProgram();

    EventWriteNumberInfo(MD, FL, FN, L"Program Id", programId);

    glAttachShader(programId, vertexShaderID);
    glAttachShader(programId, fragmentShaderID);
    glLinkProgram(programId);
    glGetProgramiv(programId, GL_LINK_STATUS, &result);
    glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);

    if (infoLogLength > 0)
    {
        std::vector<char> programErrorMessage(infoLogLength + 1);
        glGetProgramInfoLog(programId, infoLogLength, NULL, &programErrorMessage[0]);
        EventWriteAnsiStrInfo(MD, FL, FN, L"Link shader result", &programErrorMessage[0]);
    }

    glDetachShader(programId, vertexShaderID);
    glDetachShader(programId, fragmentShaderID);
    glDeleteShader(vertexShaderID);
    glDeleteShader(fragmentShaderID);

    return programId;
}

GLvoid ReleaseGLWindow(GLvoid)
{
    if (hRC)
    {
        wglMakeCurrent(NULL, NULL);
        wglDeleteContext(hRC);
        hRC = NULL;
    }

    if (hDC && !ReleaseDC(hWnd, hDC)) hDC = NULL;
    if (hWnd && !DestroyWindow(hWnd)) hWnd = NULL;
    if (!UnregisterClass(szWindowClass, hInst)) hInst = NULL;
}

BOOL InitializeGL()
{
    GLuint pixelFormat;

    static PIXELFORMATDESCRIPTOR pfd = {
        sizeof(PIXELFORMATDESCRIPTOR), 1, PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER, PFD_TYPE_RGBA, 32,
        0,0,0,0,0,0,0,0,0,0,0,0,0,24,0,0,
        PFD_MAIN_PLANE,
        0,0,0,0
    };

    if (!(hDC = GetDC(hWnd)))
    {
        EventWriteLastError(MD, FL, FN, L"GetDC", GetLastError());
        ReleaseGLWindow();
        return FALSE;
    }

    if (!(pixelFormat = ChoosePixelFormat(hDC, &pfd)))
    {
        EventWriteLastError(MD, FL, FN, L"ChoosePixelFormat", GetLastError());
        ReleaseGLWindow();
        return FALSE;
    }

    if (!SetPixelFormat(hDC, pixelFormat, &pfd))
    {
        EventWriteLastError(MD, FL, FN, L"SetPixelFormat", GetLastError());
        ReleaseGLWindow();
        return FALSE;
    }

    if (!(hRC = wglCreateContext(hDC)))
    {
        EventWriteLastError(MD, FL, FN, L"wglCreateContext", GetLastError());
        ReleaseGLWindow();
        return FALSE;
    }

    if (!wglMakeCurrent(hDC, hRC))
    {
        EventWriteLastError(MD, FL, FN, L"wglMakeCurrent", GetLastError());
        ReleaseGLWindow();
        return FALSE;
    }

    return TRUE;
}

BOOL Initialize(HINSTANCE hInstance, int nCmdShow)
{
    hInst = hInstance;

    hWnd = CreateWindow(
        szWindowClass,
        L"OpenGL-Win32",
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        0,
        640,
        480,
        NULL,
        NULL,
        hInstance,
        NULL);

    if (!hWnd) return FALSE;

    RECT rcClient;
    GetClientRect(hWnd, &rcClient);

    RECT rcWin;
    GetWindowRect(hWnd, &rcWin);

    int widthOverhead = (rcWin.right - rcWin.left) - rcClient.right;
    int heightOverhead = (rcWin.bottom - rcWin.top) - rcClient.bottom;

    int nX = 640 + widthOverhead;
    int nY = 480 + heightOverhead;

    MoveWindow(hWnd, 0, 0, nX, nY, FALSE);
    MoveWindow(hWnd, (GetSystemMetrics(SM_CXSCREEN) - nX) / 2, (GetSystemMetrics(SM_CYSCREEN) - nY) / 2, nX, nY, FALSE);

    InitializeGL();

    ShowWindow(hWnd, SW_SHOW);
    SetForegroundWindow(hWnd);
    SetFocus(hWnd);

    wchar_t szVersion[MAX_PATH];
    MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)(char*)glGetString(GL_VERSION), -1, szVersion, MAX_PATH);
    EventWriteWideStrInfo(MD, FL, FN, L"OpenGL Version", szVersion);

    wchar_t szWinText[MAX_PATH];
    StringCchPrintf(szWinText, MAX_PATH, L"OpenGL-Win32 (OpenGL Version %s)", szVersion);
    SetWindowText(hWnd, szWinText);

    if (glewInit() != GLEW_OK)
    {
        EventWriteErrorW(MD, FL, FN, L"Failed to initialize GLEW");
    }

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    GLfloat vertices_position[12] = {
        -0.8f, 0.8f, 0.0f,
        0.8f, 0.8f, 0.0f,
        0.8f, -0.8f, 0.0f,
        -0.8f, -0.8f, 0.0f
    };

    GLfloat texture_coord[8] = {
        0.0, 0.0,
        1.0, 0.0,
        1.0, 1.0,
        0.0, 1.0,
    };

    GLuint indices[6] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices_position) + sizeof(texture_coord), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices_position), vertices_position);
    glBufferSubData(GL_ARRAY_BUFFER, sizeof(vertices_position), sizeof(texture_coord), texture_coord);

    GLuint elements[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Change path to the location of your shaders.
    shader_program = LoadShaders(
        "C:/Development/vs2015-projects/OpenGL-Win32/shaders/cam-vertex.c",
        "C:/Development/vs2015-projects/OpenGL-Win32/shaders/cam-fragment.c");

    uniform_angle = glGetUniformLocation(shader_program, "angle");

    if (uniform_angle == -1)
        EventWriteInfoW(MD, FL, FN, L"Could not bind uniform_angle.");
    else
        glUniform1f(uniform_angle, angle);

    attrib_position = glGetAttribLocation(shader_program, "position");

    if (attrib_position == -1)
    {
        EventWriteInfoW(MD, FL, FN, L"Could not bind attrib_position.");
    }
    else
    {
        glVertexAttribPointer(attrib_position, 3, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(attrib_position);
    }

    return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            SwapBuffers(hdc);
            EndPaint(hWnd, &ps);
            break;
        }

        case WM_KEYDOWN:

            if (wParam == VK_UP) {
                EventWriteInfoW(MD, FL, FN, L"VK_UP.");

                if (!bIncrement) bIncrement = TRUE;

                if (bIncrement)
                    if (angle >= 360.0f)
                        bIncrement = FALSE;

                if (bIncrement)
                    angle += 1.0f;
            }

            if (wParam == VK_DOWN) {
                EventWriteInfoW(MD, FL, FN, L"VK_DOWN.");

                if (bIncrement) bIncrement = FALSE;

                if (!bIncrement)
                    if (angle <= 0.0f)
                        bIncrement = TRUE;

                if (!bIncrement)
                    angle -= 1.0f;
            }

            break;

        case WM_CAMERA_BUFFER_READY:
        {
            glClear(GL_COLOR_BUFFER_BIT);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glUniform1f(uniform_angle, angle);

            glTexImage2D(
                GL_TEXTURE_2D,				// type of texture
                0,							// pyramid level (for mip-mapping) - 0 is the top level
                GL_RGB,						// internal colour format to convert to
                WIDTH,						// image width  i.e. 640 for Kinect in standard mode
                HEIGHT,						// image height i.e. 480 for Kinect in standard mode
                0,							// border width in pixels (can either be 1 or 0)
                GL_BGRA,						// input image format (i.e. GL_RGB, GL_RGBA, GL_BGR etc.)
                GL_UNSIGNED_BYTE,			// image data type
                pFrame);						// the actual image data itself

            glUseProgram(shader_program);

            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

            SwapBuffers(hDC);

            break;
        }

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default: return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_GLROTATE));
    wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = szWindowClass;
    wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

HRESULT CALLBACK MfFrameCallback(HRESULT hrStatus, DWORD dwStreamIndex, DWORD dwStreamFlags, LONGLONG llTimestamp, IMFSample *pSample)
{
    HRESULT hr = S_OK;

    if (SUCCEEDED(hrStatus))
    {
        DWORD dwBufferCount = 0;

        if (pSample != NULL)
        {
            hr = pSample->GetBufferCount(&dwBufferCount);

            // EventWriteNumberInfo(MD, FL, FN, L"Buffer count", dwBufferCount);

            if (dwBufferCount == 1)
            {
                IMFMediaBuffer *pMediaBuffer;

                hr = pSample->ConvertToContiguousBuffer(&pMediaBuffer);

                DWORD dwBufferSize = 0;

                hr = pMediaBuffer->GetCurrentLength(&dwBufferSize);

                // EventWriteNumberInfo(MD, FL, FN, L"Buffer size", dwBufferSize);

                IBufferLock *pBufferLock;

                hr = CreateBufferLockInstance(pMediaBuffer, &pBufferLock);

                if (SUCCEEDED(hr) && pBufferLock)
                {
                    BYTE *pData = NULL;
                    LONG lStride = 0;

                    hr = pBufferLock->LockBuffer(WIDTH * 2, HEIGHT, &pData, &lStride);

                    if (SUCCEEDED(hr))
                    {
                        FromYUY2ToRGB32(pFrame, pData, WIDTH, HEIGHT);
                        PostMessage(hWnd, WM_CAMERA_BUFFER_READY, 0, 0);
                        pBufferLock->UnlockBuffer();
                    }

                    pBufferLock->Release();
                }

                if (pMediaBuffer) pMediaBuffer->Release();
            }
        }
    }

    return S_OK;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    EventRegisterJyTrace();

    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_GLROTATE, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    if (!Initialize(hInstance, nCmdShow)) return FALSE;

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_GLROTATE));

    ICameraMf *pCamMf = nullptr;
    HRESULT hr;
    MSG m;

    hr = CreateCameraMfInstance(&pCamMf);

    if (SUCCEEDED(hr) && pCamMf)
    {
        pCamMf->Initialize(640, 480, MfFrameCallback);
        pCamMf->StartRenderAsync(CAMERA_FRIENDLY_NAME);
    }

    while (GetMessage(&m, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(m.hwnd, hAccelTable, &m))
        {
            TranslateMessage(&m);
            DispatchMessage(&m);
        }
    }

    ReleaseGLWindow();

    glDeleteProgram(shader_program);
    glDeleteTextures(1, &texture_id);
    glDeleteBuffers(1, &ebo);
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);

    if (SUCCEEDED(hr) && pCamMf)
    {
        pCamMf->StopRenderAsync();
        pCamMf->Release();
    }

    EventUnregisterJyTrace();

    return (int)m.wParam;
}
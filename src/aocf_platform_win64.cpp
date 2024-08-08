#include <aocf/aocf.h>
#include <aocf/aocf_platform.h>
#include <aocf/aocf_gl.h>

namespace AOCF
{
    enum RenderAPIName
    {
        NONE = 0,
        OPENGL = 1,
    };

    struct OpenGL
    {
        HGLRC rc;
        unsigned int versionMajor;
        unsigned int versionMinor;
        PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB;
        PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB;
        int pixelFormatAttribs[16];
        int contextAttribs[16];
    };

    struct RenderAPIInfo
    {
        RenderAPIName name;
        OpenGL gl;
    };

    RenderAPIInfo renderApiInfo;

    struct Window
    {
        HWND handle;
        bool shouldClose;
        HDC dc;
        //
    };

    const int AOCF_CLOSE_WINDOW = WM_USER + 1;

    LRESULT aocfWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CLOSE:
            PostMessageA(hwnd, AOCF_CLOSE_WINDOW, 0, 0);
            break;
        default:
        {
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
        }
        return 0;
    }

    bool Platform::initOpenGL(int32 major, int32 minor, int32 colorBits, int32 depthBits)
    {
        Window *dummyWindow = createWindow(0, 0, "");

        PIXELFORMATDESCRIPTOR pfd = {
            sizeof(PIXELFORMATDESCRIPTOR), //  size of this pfd
            1,
            PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            PFD_TYPE_RGBA,
            (BYTE)depthBits,
            0, 0, 0, 0, 0, 0,
            0,
            0,
            0,
            0, 0, 0, 0,
            (BYTE)colorBits,
            0,
            0,
            PFD_MAIN_PLANE,
            0,
            0, 0, 0};

        int pixelFormat = ChoosePixelFormat(dummyWindow->dc, &pfd);
        if (!pixelFormat)
        {
            // LOGERROR("Unable to allocate a pixel format");
            destroyWindow(dummyWindow);
            return false;
        }
        if (!SetPixelFormat(dummyWindow->dc, pixelFormat, &pfd))
        {
            // LOGERROR("Unable to set a pixel format");
            destroyWindow(dummyWindow);
            return false;
        }

        HGLRC rc = wglCreateContext(dummyWindow->dc);
        if (!rc)
        {
            // LOGERROR("Unable to create a valid OpenGL context");
            destroyWindow(dummyWindow);
            return false;
        }

        if (!wglMakeCurrent(dummyWindow->dc, rc))
        {
            // LOGERROR("Unable to set OpenGL context current");
            destroyWindow(dummyWindow);
            return false;
        }

        const int PixelFormatAttribList[] = {
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_COLOR_BITS_ARB, colorBits,
            WGL_DEPTH_BITS_ARB, depthBits,
            0};

        const int ContextAttribList[] = {
            WGL_CONTEXT_MAJOR_VERSION_ARB, major,
            WGL_CONTEXT_MINOR_VERSION_ARB, minor,
            WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0};

        renderApiInfo.name = RenderAPIName::OPENGL;
        renderApiInfo.gl.versionMajor = major;
        renderApiInfo.gl.versionMinor = minor;
        renderApiInfo.gl.wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
        renderApiInfo.gl.wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
        memcpy(renderApiInfo.gl.pixelFormatAttribs, PixelFormatAttribList, sizeof(PixelFormatAttribList));
        memcpy(renderApiInfo.gl.contextAttribs, ContextAttribList, sizeof(ContextAttribList));

        wglMakeCurrent(0, 0);
        wglDeleteContext(rc);
        destroyWindow(dummyWindow);

        // LoadGLFunctions
        getOpenGLFunctionPointers();

        return true;
    }

    Window *Platform::createWindow(int32 width, int32 height, const char *title)
    {
        const char *aocfWindowClass = "AOCF_WINDOW_CLASS";
        HINSTANCE hInstance = GetModuleHandle(NULL);
        WNDCLASSEXA wc = {};

        if (!GetClassInfoExA(hInstance, aocfWindowClass, &wc))
        {
            wc.cbSize = sizeof(WNDCLASSEXA);
            wc.style = CS_HREDRAW | CS_VREDRAW;
            wc.lpfnWndProc = aocfWindowProc;
            wc.hInstance = hInstance;
            wc.hCursor = LoadCursor(NULL, IDC_ARROW);
            wc.hbrBackground = (HBRUSH)COLOR_WINDOW;
            wc.lpszClassName = aocfWindowClass;

            if (!RegisterClassExA(&wc))
            {
                return nullptr;
            }
        }

        HWND windowHandle = CreateWindowExA(
            0,
            aocfWindowClass,
            title,
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT, CW_USEDEFAULT,
            width, height,
            NULL, NULL, hInstance, NULL);

        if (!windowHandle)
        {
            return nullptr;
        }

        Window *window = new Window();
        window->handle = windowHandle;
        window->shouldClose = false;
        window->dc = GetDC(windowHandle);

        if (renderApiInfo.name == RenderAPIName::OPENGL)
        {
            int pixelFormat;
            int numPixelFormats = 0;
            PIXELFORMATDESCRIPTOR pfd;

            const int *pixelFormatAttribList = (const int *)renderApiInfo.gl.pixelFormatAttribs;
            const int *contextAttribList = (const int *)renderApiInfo.gl.contextAttribs;

            renderApiInfo.gl.wglChoosePixelFormatARB(window->dc, pixelFormatAttribList, nullptr, 1, &pixelFormat, (UINT *)&numPixelFormats);
            if (numPixelFormats <= 0)
            {
                // LOGERROR("Unable to get a valid pixel format");
                return nullptr;
            }

            if (!SetPixelFormat(window->dc, pixelFormat, &pfd))
            {
                // LOGERROR("Unable to set a pixel format");
                return nullptr;
            }

            HGLRC rc = renderApiInfo.gl.wglCreateContextAttribsARB(window->dc, 0, contextAttribList);
            if (!rc)
            {
                // LOGERROR("Unable to create a valid OpenGL context");
                return nullptr;
            }

            renderApiInfo.gl.rc = rc;
            if (!wglMakeCurrent(window->dc, rc))
            {
                // LOGERROR("Unable to set OpenGL context current");
                return nullptr;
            }

            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        }

        return window;
    }

    void Platform::destroyWindow(Window *window)
    {
        if (window)
        {
            DestroyWindow(window->handle);
            delete window;
        }
    }

    void Platform::pollEvents(Window *window)
    {
        MSG msg = {};
        HWND hwnd = window->handle;

        while (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE))
        {
            if (msg.message == AOCF_CLOSE_WINDOW)
            {
                window->shouldClose = true;
                return;
            }

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    void Platform::swapBuffers(Window *window)
    {
        SwapBuffers(window->dc);
    }

    bool Platform::getShouldClose(Window *window)
    {
        return window->shouldClose;
    }

    void Platform::setShouldClose(Window *window, bool shouldClose)
    {
        window->shouldClose = shouldClose;
    }

    int Platform::getTime()
    {
        return GetTickCount();
    }

}
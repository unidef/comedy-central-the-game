#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <windowsx.h>
#include <objidl.h>
#include <gl/GL.h>
#include <gdiplus.h>
#include <commctrl.h>
#include <mmsystem.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <random>
#include <sstream>
#include <string>
#include <vector>

#include "LogoNodeTree.h"

#pragma comment(lib, "opengl32.lib")
#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "comctl32.lib")
#pragma comment(lib, "winmm.lib")

#ifndef GL_VERTEX_SHADER
#define GL_VERTEX_SHADER 0x8B31
#endif

#ifndef GL_FRAGMENT_SHADER
#define GL_FRAGMENT_SHADER 0x8B30
#endif

#ifndef GL_COMPILE_STATUS
#define GL_COMPILE_STATUS 0x8B81
#endif

#ifndef GL_LINK_STATUS
#define GL_LINK_STATUS 0x8B82
#endif

#ifndef GL_INFO_LOG_LENGTH
#define GL_INFO_LOG_LENGTH 0x8B84
#endif

using Gdiplus::Bitmap;
using Gdiplus::BitmapData;
using Gdiplus::ImageLockModeRead;
using Gdiplus::Rect;
using Gdiplus::Status;

using Clock = std::chrono::steady_clock;
using Seconds = std::chrono::duration<float>;

typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int);
typedef char GLchar;
typedef GLuint(APIENTRY* PFNGLCREATESHADERPROC)(GLenum type);
typedef void(APIENTRY* PFNGLSHADERSOURCEPROC)(GLuint shader, GLsizei count, const GLchar* const* string, const GLint* length);
typedef void(APIENTRY* PFNGLCOMPILESHADERPROC)(GLuint shader);
typedef void(APIENTRY* PFNGLGETSHADERIVPROC)(GLuint shader, GLenum pname, GLint* params);
typedef void(APIENTRY* PFNGLGETSHADERINFOLOGPROC)(GLuint shader, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
typedef GLuint(APIENTRY* PFNGLCREATEPROGRAMPROC)();
typedef void(APIENTRY* PFNGLATTACHSHADERPROC)(GLuint program, GLuint shader);
typedef void(APIENTRY* PFNGLLINKPROGRAMPROC)(GLuint program);
typedef void(APIENTRY* PFNGLGETPROGRAMIVPROC)(GLuint program, GLenum pname, GLint* params);
typedef void(APIENTRY* PFNGLGETPROGRAMINFOLOGPROC)(GLuint program, GLsizei maxLength, GLsizei* length, GLchar* infoLog);
typedef void(APIENTRY* PFNGLUSEPROGRAMPROC)(GLuint program);
typedef GLint(APIENTRY* PFNGLGETUNIFORMLOCATIONPROC)(GLuint program, const GLchar* name);
typedef void(APIENTRY* PFNGLUNIFORM1FPROC)(GLint location, GLfloat v0);
typedef void(APIENTRY* PFNGLUNIFORM2FPROC)(GLint location, GLfloat v0, GLfloat v1);
typedef void(APIENTRY* PFNGLUNIFORM3FPROC)(GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
typedef void(APIENTRY* PFNGLDELETEPROGRAMPROC)(GLuint program);
typedef void(APIENTRY* PFNGLDELETESHADERPROC)(GLuint shader);

struct Texture
{
    GLuint id = 0;
    int width = 1;
    int height = 1;
    bool loadedFromFile = false;
};

struct Fragment
{
    float u0 = 0.0f;
    float v0 = 0.0f;
    float u1 = 0.0f;
    float v1 = 0.0f;
    float offsetX = 0.0f;
    float offsetY = 0.0f;
    float offsetZ = 0.0f;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float velocityZ = 0.0f;
    float rotation = 0.0f;
    float rotationSpeed = 0.0f;
};

struct Color3
{
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
};

struct DeformState
{
    float amount = 0.0f;
    float freqX = 0.0f;
    float freqY = 0.0f;
    float twist = 0.0f;
    float bulge = 0.0f;
    float shear = 0.0f;
};

struct LogoOrb
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float velocityZ = 0.0f;
    float scale = 1.0f;
    float targetScale = 1.0f;
    float spinAngle = 0.0f;
    float spinRate = 0.0f;
    float gravityPolarity = 1.0f;
    float gravityStrength = 18000.0f;
    int hitCount = 0;
    Color3 baseColor{ 1.0f, 1.0f, 1.0f };
    Color3 lightColor{ 1.0f, 1.0f, 1.0f };
    bool exploding = false;
    bool imploding = false;
    float explosionAge = 0.0f;
    float implodeAge = 0.0f;
    float pulseTimer = 0.0f;
    std::vector<Fragment> pixelFragments;
};

struct CartmanEntity
{
    bool active = false;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float velocityZ = 0.0f;
    float scale = 0.34f;
    float targetScale = 0.34f;
    float spin = 0.0f;
    int logosEaten = 0;
    int invertAfterLogos = 4;
    bool colorsInverted = false;
};

struct GradientCircle
{
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float radius = 0.0f;
    float targetRadius = 0.0f;
    float age = 0.0f;
    float lifetime = 1.0f;
    Color3 innerColor{};
    Color3 outerColor{};
};

struct ResolutionOption
{
    int width = 1280;
    int height = 720;
};

enum class OptionsControlId : int
{
    ResolutionCombo = 1001,
    ApplyResolutionButton = 1002,
    SoundCheck = 1003,
    VSyncCheck = 1004,
    TripleBufferCheck = 1005,
    SpawnRateSlider = 1006,
    GravitySlider = 1007,
    LaserSlider = 1008,
    ChaosSlider = 1009,
    CloseButton = 1010,
    SpawnValue = 1104,
    GravityValue = 1105,
    LaserValue = 1106,
    ChaosValue = 1107
};

class Game
{
public:
    explicit Game(HINSTANCE instance) : instance_(instance), rng_(std::random_device{}())
    {
        currentBackgroundTop_ = RandomGradientColor();
        currentBackgroundBottom_ = RandomGradientColor();
        targetBackgroundTop_ = RandomGradientColor();
        targetBackgroundBottom_ = RandomGradientColor();
        backgroundGradientTimer_ = RandomRange(1.6f, 3.4f);
        targetSpinRate_ = RandomRange(-28.0f, 28.0f);
        nextLogoSpawnTimer_ = RandomRange(0.73f, 1.47f) * logoSpawnRateScale_;
        nextGradientCircleTimer_ = RandomRange(2.8f, 5.2f);
        targetLogoEatCount_ = RandomInt(20, 100);
    }

    ~Game()
    {
        Cleanup();
    }

    bool Initialize(int nCmdShow)
    {
        if (!InitializeWindow(nCmdShow))
        {
            return false;
        }

        if (!InitializeOpenGL())
        {
            return false;
        }

        LoadShaderFunctions();
        shaderEnabled_ = CreateDeformShaderProgram();
        laserShaderEnabled_ = CreateLaserShaderProgram();

        if (!InitializeGdiPlus())
        {
            return false;
        }

        InitializeCommonControls();
        RefreshResolutionOptions();
        ApplyPresentationSettings();

        LoadLogoTexture();
        LoadCartmanTexture();
        LoadDrawnTexture();
        CreateFontDisplayLists();
        UpdateWindowTitle();
        ShowWindow(window_, nCmdShow);
        UpdateWindow(window_);
        return true;
    }

    int Run()
    {
        running_ = true;
        auto lastFrame = Clock::now();
        auto lastFpsSample = lastFrame;

        MSG message{};
        while (running_)
        {
            while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE))
            {
                if (message.message == WM_QUIT)
                {
                    running_ = false;
                    break;
                }

                TranslateMessage(&message);
                DispatchMessageW(&message);
            }

            if (!running_)
            {
                break;
            }

            const auto now = Clock::now();
            float deltaTime = Seconds(now - lastFrame).count();
            lastFrame = now;
            deltaTime = std::clamp(deltaTime, 0.0f, 0.05f);

            Update(deltaTime);
            Render();

            ++frameCounter_;
            const float fpsElapsed = Seconds(now - lastFpsSample).count();
            if (fpsElapsed >= 0.5f)
            {
                fps_ = static_cast<float>(frameCounter_) / fpsElapsed;
                frameCounter_ = 0;
                lastFpsSample = now;
                UpdateWindowTitle();
            }
        }

        return static_cast<int>(message.wParam);
    }

    LRESULT HandleMessage(UINT message, WPARAM wParam, LPARAM lParam)
    {
        switch (message)
        {
        case WM_ERASEBKGND:
            return 1;
        case WM_SIZE:
            clientWidth_ = std::max(1, static_cast<int>(LOWORD(lParam)));
            clientHeight_ = std::max(1, static_cast<int>(HIWORD(lParam)));
            return 0;
        case WM_LBUTTONDOWN:
            if (HandleExitButtonClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
            {
                return 0;
            }

            if (HandleOptionsButtonClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
            {
                return 0;
            }

            if (HandleCartmanMouseDown(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)))
            {
                return 0;
            }

            HandleClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONDBLCLK:
            HandleCartmanDoubleClick(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONUP:
            HandleCartmanMouseUp();
            return 0;
        case WM_MOUSEMOVE:
            HandleCartmanMouseMove(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_KEYDOWN:
            if (wParam == 'A')
            {
                spinLeftHeld_ = true;
                return 0;
            }

            if (wParam == 'D')
            {
                spinRightHeld_ = true;
                return 0;
            }

            if (wParam == 'W')
            {
                moveUpHeld_ = true;
                return 0;
            }

            if (wParam == 'S')
            {
                moveDownHeld_ = true;
                return 0;
            }

            if (wParam == VK_LEFT || wParam == VK_RIGHT || wParam == VK_UP || wParam == VK_DOWN)
            {
                AdjustLaserControls(wParam);
                ApplyArrowKeyChaos();
                return 0;
            }

            if (wParam >= '0' && wParam <= '9')
            {
                ApplyNumberDeformation(static_cast<int>(wParam - '0'));
                return 0;
            }

            if (wParam >= VK_NUMPAD0 && wParam <= VK_NUMPAD9)
            {
                ApplyNumberDeformation(static_cast<int>(wParam - VK_NUMPAD0));
                return 0;
            }

            if (wParam == VK_ESCAPE)
            {
                running_ = false;
                DestroyWindow(window_);
                return 0;
            }

            if (wParam == VK_F11)
            {
                ToggleFullscreen();
                return 0;
            }

            if (wParam == VK_SPACE)
            {
                spaceHeld_ = true;
                return 0;
            }
            break;
        case WM_SYSKEYDOWN:
            if (wParam == VK_RETURN && (HIWORD(lParam) & KF_ALTDOWN) != 0)
            {
                ToggleFullscreen();
                return 0;
            }
            break;
        case WM_KEYUP:
            if (wParam == 'A')
            {
                spinLeftHeld_ = false;
                return 0;
            }

            if (wParam == 'D')
            {
                spinRightHeld_ = false;
                return 0;
            }

            if (wParam == 'W')
            {
                moveUpHeld_ = false;
                return 0;
            }

            if (wParam == 'S')
            {
                moveDownHeld_ = false;
                return 0;
            }

            if (wParam == VK_SPACE)
            {
                spaceHeld_ = false;
                return 0;
            }
            break;
        case WM_CLOSE:
            running_ = false;
            DestroyWindow(window_);
            return 0;
        case WM_DESTROY:
            running_ = false;
            PostQuitMessage(0);
            return 0;
        case WM_CAPTURECHANGED:
            draggingCartman_ = false;
            return 0;
        }

        return DefWindowProcW(window_, message, wParam, lParam);
    }

private:
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* game = reinterpret_cast<Game*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            game = reinterpret_cast<Game*>(createStruct->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(game));
            if (game != nullptr)
            {
                game->window_ = hwnd;
            }
        }

        if (game != nullptr)
        {
            return game->HandleMessage(message, wParam, lParam);
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    bool InitializeWindow(int nCmdShow)
    {
        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
        windowClass.lpfnWndProc = WindowProc;
        windowClass.hInstance = instance_;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_HAND);
        windowClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        windowClass.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
        windowClass.lpszClassName = L"CCClickGameWindow";

        if (RegisterClassExW(&windowClass) == 0)
        {
            return false;
        }

        RECT desiredRect{ 0, 0, clientWidth_, clientHeight_ };
        AdjustWindowRect(&desiredRect, WS_OVERLAPPEDWINDOW, FALSE);

        window_ = CreateWindowExW(
            0,
            windowClass.lpszClassName,
            L"CC Click Game",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            desiredRect.right - desiredRect.left,
            desiredRect.bottom - desiredRect.top,
            nullptr,
            nullptr,
            instance_,
            this);

        return window_ != nullptr;
    }

    bool InitializeOpenGL()
    {
        deviceContext_ = GetDC(window_);
        if (deviceContext_ == nullptr)
        {
            return false;
        }

        PIXELFORMATDESCRIPTOR pixelFormat{};
        pixelFormat.nSize = sizeof(pixelFormat);
        pixelFormat.nVersion = 1;
        pixelFormat.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
        pixelFormat.iPixelType = PFD_TYPE_RGBA;
        pixelFormat.cColorBits = 32;
        pixelFormat.cDepthBits = 24;
        pixelFormat.cStencilBits = 8;
        pixelFormat.iLayerType = PFD_MAIN_PLANE;

        const int chosenFormat = ChoosePixelFormat(deviceContext_, &pixelFormat);
        if (chosenFormat == 0)
        {
            return false;
        }

        if (!SetPixelFormat(deviceContext_, chosenFormat, &pixelFormat))
        {
            return false;
        }

        renderingContext_ = wglCreateContext(deviceContext_);
        if (renderingContext_ == nullptr)
        {
            return false;
        }

        if (!wglMakeCurrent(deviceContext_, renderingContext_))
        {
            return false;
        }

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_TEXTURE_2D);
        glDisable(GL_DEPTH_TEST);
        glClearColor(0.04f, 0.04f, 0.07f, 1.0f);

        swapIntervalProc_ = reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(wglGetProcAddress("wglSwapIntervalEXT"));

        return true;
    }

    bool InitializeGdiPlus()
    {
        Gdiplus::GdiplusStartupInput startupInput;
        return Gdiplus::GdiplusStartup(&gdiPlusToken_, &startupInput, nullptr) == Gdiplus::Ok;
    }

    void LoadLogoTexture()
    {
        const auto assetPath = FindAssetPath(L"cc.png");
        if (!assetPath.empty() && LoadTextureFromPng(assetPath, logoTexture_))
        {
            assetStatus_ = L"Loaded cc.png";
            return;
        }

        CreateFallbackTexture(logoTexture_);
        assetStatus_ = L"cc.png not found - using fallback texture";
    }

    void LoadCartmanTexture()
    {
        const auto assetPath = FindAssetPath(L"cartman.png");
        if (!assetPath.empty() && LoadTextureFromPng(assetPath, cartmanTexture_))
        {
            return;
        }

        CreateCartmanFallbackTexture(cartmanTexture_);
    }

    void LoadDrawnTexture()
    {
        const auto assetPath = FindAssetPath(L"drawn.png");
        if (!assetPath.empty() && LoadTextureFromPng(assetPath, drawnTexture_))
        {
            return;
        }

        CreateFallbackTexture(drawnTexture_);
    }

    void CreateProceduralBackgroundTexture()
    {
        constexpr int textureSize = 128;
        std::vector<unsigned char> pixels(textureSize * textureSize * 4);
        for (int y = 0; y < textureSize; ++y)
        {
            for (int x = 0; x < textureSize; ++x)
            {
                const int index = (y * textureSize + x) * 4;
                const float nx = static_cast<float>(x) / static_cast<float>(textureSize);
                const float ny = static_cast<float>(y) / static_cast<float>(textureSize);
                const float rings = std::sin((nx * 19.0f) + (ny * 23.0f)) * 0.5f + 0.5f;
                const float waves = std::cos((nx * 41.0f) - (ny * 17.0f)) * 0.5f + 0.5f;
                const float checker = ((x / 8 + y / 8) % 2 == 0) ? 0.25f : 0.85f;
                pixels[index + 0] = static_cast<unsigned char>(60 + rings * 120 + checker * 40);
                pixels[index + 1] = static_cast<unsigned char>(40 + waves * 110 + checker * 30);
                pixels[index + 2] = static_cast<unsigned char>(85 + (rings * waves) * 150);
                pixels[index + 3] = 255;
            }
        }

        proceduralBackgroundTexture_.width = textureSize;
        proceduralBackgroundTexture_.height = textureSize;
        if (proceduralBackgroundTexture_.id == 0)
        {
            glGenTextures(1, &proceduralBackgroundTexture_.id);
        }

        glBindTexture(GL_TEXTURE_2D, proceduralBackgroundTexture_.id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureSize, textureSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    }

    void AdjustLaserControls(WPARAM key)
    {
        if (key == VK_LEFT)
        {
            laserDirectionOffset_ -= 7.0f;
        }
        else if (key == VK_RIGHT)
        {
            laserDirectionOffset_ += 7.0f;
        }
        else if (key == VK_UP)
        {
            laserVelocity_ = std::min(laserVelocity_ + 0.12f, 4.0f);
        }
        else if (key == VK_DOWN)
        {
            laserVelocity_ = std::max(laserVelocity_ - 0.12f, 0.2f);
        }
    }

    int RandomInt(int minimum, int maximum)
    {
        std::uniform_int_distribution<int> distribution(minimum, maximum);
        return distribution(rng_);
    }

    std::filesystem::path FindAssetPath(const wchar_t* fileName) const
    {
        std::vector<std::filesystem::path> candidates;
        candidates.emplace_back(fileName);
        candidates.emplace_back(std::filesystem::path(L"..") / L".." / L"ccgame_zoomzoomzoom" / fileName);

        wchar_t executablePath[MAX_PATH]{};
        GetModuleFileNameW(nullptr, executablePath, MAX_PATH);
        std::filesystem::path exeDirectory = std::filesystem::path(executablePath).parent_path();
        candidates.push_back(exeDirectory / fileName);
        candidates.push_back(exeDirectory / L".." / L".." / L".." / L"ccgame_zoomzoomzoom" / fileName);

        auto walk = exeDirectory;
        for (int i = 0; i < 4; ++i)
        {
            walk = walk.parent_path();
            if (walk.empty())
            {
                break;
            }

            candidates.push_back(walk / fileName);
        }

        for (const auto& candidate : candidates)
        {
            if (!candidate.empty() && std::filesystem::exists(candidate))
            {
                return candidate;
            }
        }

        return {};
    }

    bool LoadTextureFromPng(const std::filesystem::path& pngPath, Texture& texture, bool invertColors = false)
    {
        Bitmap bitmap(pngPath.c_str());
        if (bitmap.GetLastStatus() != Gdiplus::Ok)
        {
            return false;
        }

        texture.width = static_cast<int>(bitmap.GetWidth());
        texture.height = static_cast<int>(bitmap.GetHeight());

        Rect rect(0, 0, texture.width, texture.height);
        BitmapData data{};
        const Status status = bitmap.LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &data);
        if (status != Gdiplus::Ok)
        {
            return false;
        }

        std::vector<unsigned char> rgba(texture.width * texture.height * 4);
        for (int y = 0; y < texture.height; ++y)
        {
            const auto* sourceRow = static_cast<const unsigned char*>(data.Scan0) + (y * data.Stride);
            auto* targetRow = rgba.data() + (y * texture.width * 4);
            for (int x = 0; x < texture.width; ++x)
            {
                const unsigned char red = sourceRow[x * 4 + 2];
                const unsigned char green = sourceRow[x * 4 + 1];
                const unsigned char blue = sourceRow[x * 4 + 0];
                targetRow[x * 4 + 0] = invertColors ? static_cast<unsigned char>(255 - red) : red;
                targetRow[x * 4 + 1] = invertColors ? static_cast<unsigned char>(255 - green) : green;
                targetRow[x * 4 + 2] = invertColors ? static_cast<unsigned char>(255 - blue) : blue;
                targetRow[x * 4 + 3] = sourceRow[x * 4 + 3];
            }
        }

        bitmap.UnlockBits(&data);

        if (texture.id == 0)
        {
            glGenTextures(1, &texture.id);
        }

        glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, rgba.data());

        texture.loadedFromFile = true;
        return true;
    }

    void CreateFallbackTexture(Texture& texture)
    {
        constexpr std::array<unsigned char, 4 * 4 * 4> pixels{
            240,  60,  90, 255, 255, 210,  80, 255, 240,  60,  90, 255, 255, 210,  80, 255,
            255, 210,  80, 255, 255, 255, 255, 255, 255, 210,  80, 255, 255, 255, 255, 255,
            240,  60,  90, 255, 255, 210,  80, 255, 240,  60,  90, 255, 255, 210,  80, 255,
            255, 210,  80, 255, 255, 255, 255, 255, 255, 210,  80, 255, 255, 255, 255, 255
        };

        texture.width = 4;
        texture.height = 4;

        if (texture.id == 0)
        {
            glGenTextures(1, &texture.id);
        }

        glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    }

    void CreateCartmanFallbackTexture(Texture& texture, bool invertColors = false)
    {
        constexpr std::array<unsigned char, 4 * 4 * 4> pixels{
            50, 110, 240, 255, 240, 210,  70, 255, 240, 210,  70, 255,  50, 110, 240, 255,
            50, 110, 240, 255, 255, 220, 170, 255, 255, 220, 170, 255,  50, 110, 240, 255,
            150,  70,  40, 255, 220,  45,  45, 255, 220,  45,  45, 255, 150,  70,  40, 255,
             60,  40,  30, 255,  60,  40,  30, 255,  60,  40,  30, 255,  60,  40,  30, 255
        };
        std::array<unsigned char, 4 * 4 * 4> finalPixels = pixels;

        if (invertColors)
        {
            for (size_t i = 0; i < finalPixels.size(); i += 4)
            {
                finalPixels[i + 0] = static_cast<unsigned char>(255 - finalPixels[i + 0]);
                finalPixels[i + 1] = static_cast<unsigned char>(255 - finalPixels[i + 1]);
                finalPixels[i + 2] = static_cast<unsigned char>(255 - finalPixels[i + 2]);
            }
        }

        texture.width = 4;
        texture.height = 4;

        if (texture.id == 0)
        {
            glGenTextures(1, &texture.id);
        }

        glBindTexture(GL_TEXTURE_2D, texture.id);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texture.width, texture.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, finalPixels.data());
    }

    void CreateFontDisplayLists()
    {
        if (fontBase_ != 0)
        {
            glDeleteLists(fontBase_, 256);
            fontBase_ = 0;
        }

        fontBase_ = glGenLists(256);

        HFONT font = CreateFontW(
            -24,
            0,
            0,
            0,
            FW_BOLD,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY,
            FF_DONTCARE,
            L"Segoe UI");

        const HGDIOBJ previousObject = SelectObject(deviceContext_, font);
        wglUseFontBitmapsW(deviceContext_, 0, 255, fontBase_);
        SelectObject(deviceContext_, previousObject);
        DeleteObject(font);
    }

    void Update(float deltaTime)
    {
        elapsedSeconds_ += deltaTime;

        backgroundGradientTimer_ -= deltaTime;
        if (backgroundGradientTimer_ <= 0.0f)
        {
            targetBackgroundTop_ = RandomGradientColor();
            targetBackgroundBottom_ = RandomGradientColor();
            backgroundGradientTimer_ = RandomRange(1.8f, 4.2f);
        }

        const float backgroundLerp = std::clamp(deltaTime * 0.7f, 0.0f, 1.0f);
        currentBackgroundTop_.r += (targetBackgroundTop_.r - currentBackgroundTop_.r) * backgroundLerp;
        currentBackgroundTop_.g += (targetBackgroundTop_.g - currentBackgroundTop_.g) * backgroundLerp;
        currentBackgroundTop_.b += (targetBackgroundTop_.b - currentBackgroundTop_.b) * backgroundLerp;
        currentBackgroundBottom_.r += (targetBackgroundBottom_.r - currentBackgroundBottom_.r) * backgroundLerp;
        currentBackgroundBottom_.g += (targetBackgroundBottom_.g - currentBackgroundBottom_.g) * backgroundLerp;
        currentBackgroundBottom_.b += (targetBackgroundBottom_.b - currentBackgroundBottom_.b) * backgroundLerp;

        UpdateGradientCircles(deltaTime);
        if (proceduralBackgroundTimer_ > 0.0f)
        {
            proceduralBackgroundTimer_ = (std::max)(0.0f, proceduralBackgroundTimer_ - deltaTime);
        }

        nextGradientCircleTimer_ -= deltaTime;
        if (nextGradientCircleTimer_ <= 0.0f)
        {
            SpawnGradientCircle();
            nextGradientCircleTimer_ = RandomRange(3.0f, 6.0f);
        }

        if (!gameOver_)
        {
            const float controlSpinRate = (spinRightHeld_ ? 220.0f : 0.0f) - (spinLeftHeld_ ? 220.0f : 0.0f);
            const float flyInput = (moveDownHeld_ ? 1.0f : 0.0f) - (moveUpHeld_ ? 1.0f : 0.0f);

            const float deformationLerp = std::clamp(deltaTime * 10.0f, 0.0f, 1.0f);
            scaleX_ += (targetScaleX_ - scaleX_) * deformationLerp;
            scaleY_ += (targetScaleY_ - scaleY_) * deformationLerp;
            rotation_ += (targetRotation_ - rotation_) * deformationLerp;

            spinRate_ += (targetSpinRate_ - spinRate_) * std::clamp(deltaTime * 1.5f, 0.0f, 1.0f);
            spinAngle_ += (spinRate_ + controlSpinRate) * deltaTime;

            const float relaxLerp = std::clamp(deltaTime * 1.4f, 0.0f, 1.0f);
            targetScaleX_ += (1.0f - targetScaleX_) * relaxLerp;
            targetScaleY_ += (1.0f - targetScaleY_) * relaxLerp;
            targetRotation_ += (0.0f - targetRotation_) * relaxLerp;

            const float deformRelax = std::clamp(deltaTime * 1.6f, 0.0f, 1.0f);
            deformState_.amount += (0.0f - deformState_.amount) * deformRelax;
            deformState_.freqX += (0.0f - deformState_.freqX) * deformRelax;
            deformState_.freqY += (0.0f - deformState_.freqY) * deformRelax;
            deformState_.twist += (0.0f - deformState_.twist) * deformRelax;
            deformState_.bulge += (0.0f - deformState_.bulge) * deformRelax;
            deformState_.shear += (0.0f - deformState_.shear) * deformRelax;

            jiggleStrength_ = std::max(0.0f, jiggleStrength_ - deltaTime * 0.9f);

            if (flyInput != 0.0f)
            {
                bounceVelocityY_ += flyInput * 780.0f * deltaTime;
                bounceVelocityX_ += std::sin((elapsedSeconds_ * 8.0f) + (flyInput * 1.7f)) * 340.0f * deltaTime;
                jiggleStrength_ = std::min(1.0f, jiggleStrength_ + deltaTime * 1.8f);
                flyDistance_ += (std::abs(bounceVelocityX_) + std::abs(bounceVelocityY_)) * 0.5f * deltaTime;

                if (flyDistance_ >= maxFlyDistance_)
                {
                    TriggerExplosion();
                }
            }

            bounceOffsetX_ += bounceVelocityX_ * deltaTime;
            bounceOffsetY_ += bounceVelocityY_ * deltaTime;

            const LogoRect motionRect = GetLogoRect();
            const float limitX = std::max(0.0f, (clientWidth_ * 0.5f) - motionRect.halfWidth);
            const float limitY = std::max(0.0f, (clientHeight_ * 0.5f) - motionRect.halfHeight);

            if (bounceOffsetX_ > limitX)
            {
                bounceOffsetX_ = limitX;
                bounceVelocityX_ = -std::abs(bounceVelocityX_) * RandomRange(0.82f, 1.18f);
            }
            else if (bounceOffsetX_ < -limitX)
            {
                bounceOffsetX_ = -limitX;
                bounceVelocityX_ = std::abs(bounceVelocityX_) * RandomRange(0.82f, 1.18f);
            }

            if (bounceOffsetY_ > limitY)
            {
                bounceOffsetY_ = limitY;
                bounceVelocityY_ = -std::abs(bounceVelocityY_) * RandomRange(0.82f, 1.18f);
            }
            else if (bounceOffsetY_ < -limitY)
            {
                bounceOffsetY_ = -limitY;
                bounceVelocityY_ = std::abs(bounceVelocityY_) * RandomRange(0.82f, 1.18f);
            }

            bounceVelocityX_ *= 0.998f;
            bounceVelocityY_ *= 0.998f;

            randomBounceTimer_ -= deltaTime;
            if (randomBounceTimer_ <= 0.0f)
            {
                bounceVelocityX_ += RandomRange(-120.0f, 120.0f);
                bounceVelocityY_ += RandomRange(-120.0f, 120.0f);
                targetSpinRate_ = RandomRange(-48.0f, 48.0f);
                randomBounceTimer_ = RandomRange(0.8f, 1.8f);
            }

            if (!cartman_.active && elapsedSeconds_ >= 5.0f)
            {
                cartman_.active = true;
                cartmanHasEntered_ = true;
                cartman_.x = clientWidth_ * 0.18f;
                cartman_.y = clientHeight_ * 0.22f;
                cartman_.z = -140.0f;
                cartman_.velocityX = 60.0f;
                cartman_.velocityY = 30.0f;
                cartman_.velocityZ = RandomRange(-20.0f, 20.0f);
                cartman_.scale = 0.34f;
                cartman_.targetScale = 0.34f;
                cartman_.logosEaten = 0;
                cartman_.colorsInverted = false;
                cartmanDisappearTimer_ = RandomRange(5.0f, 9.0f);
                activeJoke_ = "Cartman entered the arena hungry.";
                jokeTimer_ = 2.5f;
                PlaySystemSound(RandomSystemSoundAlias());
            }

            if (cartmanHasEntered_ && !cartman_.active)
            {
                cartmanReturnTimer_ -= deltaTime;
                if (cartmanReturnTimer_ <= 0.0f)
                {
                    cartman_.active = true;
                    cartman_.x = RandomRange(clientWidth_ * 0.18f, clientWidth_ * 0.82f);
                    cartman_.y = RandomRange(clientHeight_ * 0.18f, clientHeight_ * 0.82f);
                    cartman_.z = RandomRange(-260.0f, 140.0f);
                    cartman_.velocityX = RandomRange(-90.0f, 90.0f);
                    cartman_.velocityY = RandomRange(-90.0f, 90.0f);
                    cartman_.velocityZ = RandomRange(-25.0f, 25.0f);
                    cartman_.scale = std::max(cartman_.scale, 0.34f);
                    cartman_.targetScale = std::max(cartman_.targetScale, 0.34f);
                    cartmanDisappearTimer_ = RandomRange(5.0f, 9.0f);
                    activeJoke_ = "Cartman came back.";
                    jokeTimer_ = 1.8f;
                }
            }

            nextLogoSpawnTimer_ -= deltaTime;
            if (nextLogoSpawnTimer_ <= 0.0f)
            {
                SpawnRandomLogo();
                nextLogoSpawnTimer_ = RandomRange(0.67f, 1.6f);
            }

            UpdateSpawnedLogos(deltaTime);
            UpdateLogoNodeTree();
            ApplyLogoNodeInfluence(deltaTime);
            UpdateCartman(deltaTime);
            UpdateLaser(deltaTime);
            cartmanJiggleStrength_ = std::max(0.0f, cartmanJiggleStrength_ - deltaTime * 1.4f);
        }

        jokeTimer_ = std::max(0.0f, jokeTimer_ - deltaTime);
        const bool hadOuch = ouchTimer_ > 0.0f;
        ouchTimer_ = std::max(0.0f, ouchTimer_ - deltaTime);
        if (hadOuch && ouchTimer_ <= 0.0f)
        {
            UpdateWindowTitle();
        }

        if (exploding_)
        {
            for (auto& fragment : fragments_)
            {
                fragment.offsetX += fragment.velocityX * deltaTime;
                fragment.offsetY += fragment.velocityY * deltaTime;
                fragment.rotation += fragment.rotationSpeed * deltaTime;
                fragment.velocityY += 520.0f * deltaTime;
            }
        }

        if (gameOver_ && elapsedSeconds_ >= gameOverCloseTime_)
        {
            running_ = false;
            DestroyWindow(window_);
        }
    }

    void Render()
    {
        glViewport(0, 0, clientWidth_, clientHeight_);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        glOrtho(0.0, static_cast<double>(clientWidth_), static_cast<double>(clientHeight_), 0.0, -1.0, 1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();

        glClear(GL_COLOR_BUFFER_BIT);

        if (playerWon_)
        {
            DrawVictoryScreen();
            DrawOverlay();
            SwapBuffers(deviceContext_);
            return;
        }

        DrawBackground();

        if (exploding_)
        {
            DrawExplosion();
        }
        else
        {
            DrawGradientCircles();
            DrawSpawnedLogos();
            DrawLogoNodeConnections();
            DrawCartman();
            DrawLogo();
            DrawLaser();
        }

        DrawOverlay();
        if (tripleBufferingEnabled_)
        {
            glFlush();
        }
        SwapBuffers(deviceContext_);
    }

    void DrawBackground() const
    {
        glDisable(GL_TEXTURE_2D);
        glBegin(GL_QUADS);
        glColor3f(currentBackgroundTop_.r, currentBackgroundTop_.g, currentBackgroundTop_.b);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(static_cast<float>(clientWidth_), 0.0f);
        glColor3f(currentBackgroundBottom_.r, currentBackgroundBottom_.g, currentBackgroundBottom_.b);
        glVertex2f(static_cast<float>(clientWidth_), static_cast<float>(clientHeight_));
        glVertex2f(0.0f, static_cast<float>(clientHeight_));
        glEnd();
        glEnable(GL_TEXTURE_2D);

        if (proceduralBackgroundTimer_ > 0.0f && proceduralBackgroundTexture_.id != 0)
        {
            glBindTexture(GL_TEXTURE_2D, proceduralBackgroundTexture_.id);
            glColor4f(1.0f, 1.0f, 1.0f, 0.85f);
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(0.0f, 0.0f);
            glTexCoord2f(5.0f, 0.0f);
            glVertex2f(static_cast<float>(clientWidth_), 0.0f);
            glTexCoord2f(5.0f, 5.0f);
            glVertex2f(static_cast<float>(clientWidth_), static_cast<float>(clientHeight_));
            glTexCoord2f(0.0f, 5.0f);
            glVertex2f(0.0f, static_cast<float>(clientHeight_));
            glEnd();
        }

        glBindTexture(GL_TEXTURE_2D, 0);
    }

    void DrawVictoryScreen() const
    {
        glBindTexture(GL_TEXTURE_2D, drawnTexture_.id);
        glEnable(GL_TEXTURE_2D);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(0.0f, 0.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(static_cast<float>(clientWidth_), 0.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(static_cast<float>(clientWidth_), static_cast<float>(clientHeight_));
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(0.0f, static_cast<float>(clientHeight_));
        glEnd();
    }

    void DrawLogo() const
    {
        const LogoRect logoRect = GetLogoRect();

        glBindTexture(GL_TEXTURE_2D, logoTexture_.id);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        if (shaderEnabled_ && glUseProgram_ != nullptr)
        {
            glUseProgram_(deformProgram_);
            if (deformTimeLocation_ >= 0 && glUniform1f_ != nullptr)
            {
                glUniform1f_(deformTimeLocation_, elapsedSeconds_);
            }

            if (deformAmountLocation_ >= 0 && glUniform1f_ != nullptr)
            {
                glUniform1f_(deformAmountLocation_, deformState_.amount);
            }

            if (deformFreqLocation_ >= 0 && glUniform2f_ != nullptr)
            {
                glUniform2f_(deformFreqLocation_, deformState_.freqX, deformState_.freqY);
            }

            if (deformTwistLocation_ >= 0 && glUniform1f_ != nullptr)
            {
                glUniform1f_(deformTwistLocation_, deformState_.twist);
            }

            if (deformBulgeLocation_ >= 0 && glUniform1f_ != nullptr)
            {
                glUniform1f_(deformBulgeLocation_, deformState_.bulge);
            }

            if (deformShearLocation_ >= 0 && glUniform1f_ != nullptr)
            {
                glUniform1f_(deformShearLocation_, deformState_.shear);
            }

            if (deformHalfSizeLocation_ >= 0 && glUniform2f_ != nullptr)
            {
                glUniform2f_(deformHalfSizeLocation_, logoRect.halfWidth, logoRect.halfHeight);
            }
        }

        glPushMatrix();
        glTranslatef(logoRect.centerX, logoRect.centerY, 0.0f);
        glRotatef(spinAngle_ + rotation_ + std::sin(elapsedSeconds_ * 24.0f) * jiggleStrength_ * 16.0f, 0.0f, 0.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-logoRect.halfWidth, -logoRect.halfHeight);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(logoRect.halfWidth, -logoRect.halfHeight);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(logoRect.halfWidth, logoRect.halfHeight);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-logoRect.halfWidth, logoRect.halfHeight);
        glEnd();
        glPopMatrix();

        if (shaderEnabled_ && glUseProgram_ != nullptr)
        {
            glUseProgram_(0);
        }
    }

    void DrawLaser() const
    {
        if (!spaceHeld_)
        {
            return;
        }

        const LogoRect logoRect = GetLogoRect();
        const float angle = (spinAngle_ + rotation_ + laserDirectionOffset_) * 0.0174532925f;
        const float directionX = std::cos(angle);
        const float directionY = std::sin(angle);
        const float perpendicularX = -directionY;
        const float perpendicularY = directionX;
        const float length = std::sqrt((clientWidth_ * clientWidth_) + (clientHeight_ * clientHeight_));
        const float startX = logoRect.centerX + directionX * (logoRect.halfWidth + 8.0f);
        const float startY = logoRect.centerY + directionY * (logoRect.halfHeight + 8.0f);
        const float endX = startX + directionX * length * 3.0f;
        const float endY = startY + directionY * length * 3.0f;
        const float beamHalfWidth = (6.0f + std::sin(elapsedSeconds_ * 18.0f * laserVelocity_) * 1.6f) * 3.0f;
        const float stereoOffset = 9.0f;
        const float beamDepth = GetLogoDepth();

        glDisable(GL_TEXTURE_2D);
        if (laserShaderEnabled_ && glUseProgram_ != nullptr)
        {
            glUseProgram_(laserProgram_);
            if (laserTimeLocation_ >= 0 && glUniform1f_ != nullptr)
            {
                glUniform1f_(laserTimeLocation_, elapsedSeconds_);
            }
        }

        DrawLaserBeam(startX, startY, endX, endY, beamDepth, perpendicularX, perpendicularY, beamHalfWidth, -stereoOffset, -1.0f);
        DrawLaserBeam(startX, startY, endX, endY, beamDepth, perpendicularX, perpendicularY, beamHalfWidth, stereoOffset, 1.0f);

        if (laserShaderEnabled_ && glUseProgram_ != nullptr)
        {
            glUseProgram_(0);
        }

        glEnable(GL_TEXTURE_2D);
    }

    void DrawLaserBeam(float startX, float startY, float endX, float endY, float beamDepth, float perpendicularX, float perpendicularY, float beamHalfWidth, float stereoOffset, float stereoSign) const
    {
        const float offsetX = perpendicularX * stereoOffset;
        const float offsetY = perpendicularY * stereoOffset;
        const float sideX = perpendicularX * beamHalfWidth;
        const float sideY = perpendicularY * beamHalfWidth;

        if (laserShaderEnabled_ && glUniform1f_ != nullptr && laserStereoLocation_ >= 0)
        {
            glUniform1f_(laserStereoLocation_, stereoSign);
        }

        if (laserShaderEnabled_ && glUniform1f_ != nullptr && laserIntensityLocation_ >= 0)
        {
            glUniform1f_(laserIntensityLocation_, 1.0f);
        }

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(startX + offsetX - sideX, startY + offsetY - sideY);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(endX + offsetX - sideX, endY + offsetY - sideY);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(endX + offsetX + sideX, endY + offsetY + sideY);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(startX + offsetX + sideX, startY + offsetY + sideY);
        glEnd();
    }

    void DrawSpawnedLogos() const
    {
        if (spawnedLogos_.empty())
        {
            return;
        }

        glBindTexture(GL_TEXTURE_2D, logoTexture_.id);

        for (const auto& logo : spawnedLogos_)
        {
            const float textureAspect = static_cast<float>(logoTexture_.height) / static_cast<float>(logoTexture_.width);
            float fitWidth = clientWidth_ * 0.92f;
            float fitHeight = fitWidth * textureAspect;
            if (fitHeight > clientHeight_ * 0.92f)
            {
                fitHeight = clientHeight_ * 0.92f;
                fitWidth = fitHeight / textureAspect;
            }

            const float halfWidth = fitWidth * 0.11f * logo.scale;
            const float halfHeight = fitHeight * 0.11f * logo.scale;

            if (logo.exploding || logo.imploding)
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
                const float fade = logo.imploding ? std::max(0.0f, 1.0f - (logo.implodeAge * 0.95f)) : std::max(0.0f, 1.0f - (logo.explosionAge * 0.75f));
                glColor4f(1.0f, 1.0f, 1.0f, fade);

                const float pieceWidth = (halfWidth * 2.0f) / 6.0f;
                const float pieceHeight = (halfHeight * 2.0f) / 6.0f;
                for (const auto& fragment : logo.pixelFragments)
                {
                    glPushMatrix();
                    glTranslatef(logo.x + fragment.offsetX, logo.y + fragment.offsetY, 0.0f);
                    glRotatef(fragment.rotation, 0.0f, 0.0f, 1.0f);
                    glBegin(GL_QUADS);
                    glTexCoord2f(fragment.u0, fragment.v0);
                    glVertex2f(-pieceWidth * 0.5f, -pieceHeight * 0.5f);
                    glTexCoord2f(fragment.u1, fragment.v0);
                    glVertex2f(pieceWidth * 0.5f, -pieceHeight * 0.5f);
                    glTexCoord2f(fragment.u1, fragment.v1);
                    glVertex2f(pieceWidth * 0.5f, pieceHeight * 0.5f);
                    glTexCoord2f(fragment.u0, fragment.v1);
                    glVertex2f(-pieceWidth * 0.5f, pieceHeight * 0.5f);
                    glEnd();
                    glPopMatrix();
                }

                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                continue;
            }

            glColor4f(logo.baseColor.r, logo.baseColor.g, logo.baseColor.b, 0.92f);
            glPushMatrix();
            glTranslatef(logo.x, logo.y, 0.0f);
            glRotatef(logo.spinAngle, 0.0f, 0.0f, 1.0f);
            glBegin(GL_QUADS);
            glTexCoord2f(0.0f, 0.0f);
            glVertex2f(-halfWidth, -halfHeight);
            glTexCoord2f(1.0f, 0.0f);
            glVertex2f(halfWidth, -halfHeight);
            glTexCoord2f(1.0f, 1.0f);
            glVertex2f(halfWidth, halfHeight);
            glTexCoord2f(0.0f, 1.0f);
            glVertex2f(-halfWidth, halfHeight);
            glEnd();
            glPopMatrix();
        }

        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
    }

    void DrawCartman() const
    {
        if (!cartman_.active)
        {
            return;
        }

        const float textureAspect = static_cast<float>(cartmanTexture_.height) / static_cast<float>(cartmanTexture_.width);
        const float baseWidth = clientWidth_ * 0.10f;
        const float baseHeight = baseWidth * textureAspect;
        const float halfWidth = baseWidth * cartman_.scale * 0.5f;
        const float halfHeight = baseHeight * cartman_.scale * 0.5f;

        glBindTexture(GL_TEXTURE_2D, cartmanTexture_.id);
        glColor4f(1.0f, 1.0f, 1.0f, 0.96f);
        glPushMatrix();
        glTranslatef(
            cartman_.x + std::sin(elapsedSeconds_ * 24.0f) * cartmanJiggleStrength_ * sceneJiggleScale_ * 10.0f,
            cartman_.y + std::cos(elapsedSeconds_ * 27.0f) * cartmanJiggleStrength_ * sceneJiggleScale_ * 10.0f,
            0.0f);
        glRotatef(cartman_.spin + std::sin(elapsedSeconds_ * 31.0f) * cartmanJiggleStrength_ * 18.0f, 0.0f, 0.0f, 1.0f);
        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-halfWidth, -halfHeight);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(halfWidth, -halfHeight);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(halfWidth, halfHeight);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-halfWidth, halfHeight);
        glEnd();
        glPopMatrix();
    }

    void DrawGradientCircles() const
    {
        if (gradientCircles_.empty())
        {
            return;
        }

        glDisable(GL_TEXTURE_2D);
        for (const auto& circle : gradientCircles_)
        {
            const float progress = std::clamp(circle.age / circle.lifetime, 0.0f, 1.0f);
            const float alpha = std::sin(progress * 3.14159265f) * 0.28f;
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(circle.innerColor.r, circle.innerColor.g, circle.innerColor.b, alpha * 0.9f);
            glVertex2f(circle.x, circle.y);

            constexpr int segmentCount = 36;
            for (int i = 0; i <= segmentCount; ++i)
            {
                const float angle = (static_cast<float>(i) / static_cast<float>(segmentCount)) * 6.28318531f;
                const float px = circle.x + std::cos(angle) * circle.radius;
                const float py = circle.y + std::sin(angle) * circle.radius;
                glColor4f(circle.outerColor.r, circle.outerColor.g, circle.outerColor.b, 0.0f);
                glVertex2f(px, py);
            }

            glEnd();
        }
        glEnable(GL_TEXTURE_2D);
    }

    void DrawExplosion() const
    {
        const LogoRect baseRect = GetLogoRect(1.0f, 1.0f, 0.0f);
        const float pieceWidth = (baseRect.halfWidth * 2.0f) / static_cast<float>(fragmentColumns_);
        const float pieceHeight = (baseRect.halfHeight * 2.0f) / static_cast<float>(fragmentRows_);

        glBindTexture(GL_TEXTURE_2D, logoTexture_.id);
        glColor4f(1.0f, 1.0f, 1.0f, 1.0f);

        for (int row = 0; row < fragmentRows_; ++row)
        {
            for (int column = 0; column < fragmentColumns_; ++column)
            {
                const auto& fragment = fragments_[row * fragmentColumns_ + column];
                const float localCenterX = -baseRect.halfWidth + (pieceWidth * (column + 0.5f));
                const float localCenterY = -baseRect.halfHeight + (pieceHeight * (row + 0.5f));
                const float worldX = baseRect.centerX + localCenterX + fragment.offsetX;
                const float worldY = baseRect.centerY + localCenterY + fragment.offsetY;

                glPushMatrix();
                glTranslatef(worldX, worldY, 0.0f);
                glRotatef(fragment.rotation, 0.0f, 0.0f, 1.0f);
                glBegin(GL_QUADS);
                glTexCoord2f(fragment.u0, fragment.v0);
                glVertex2f(-pieceWidth * 0.5f, -pieceHeight * 0.5f);
                glTexCoord2f(fragment.u1, fragment.v0);
                glVertex2f(pieceWidth * 0.5f, -pieceHeight * 0.5f);
                glTexCoord2f(fragment.u1, fragment.v1);
                glVertex2f(pieceWidth * 0.5f, pieceHeight * 0.5f);
                glTexCoord2f(fragment.u0, fragment.v1);
                glVertex2f(-pieceWidth * 0.5f, pieceHeight * 0.5f);
                glEnd();
                glPopMatrix();
            }
        }
    }

    bool HandleCartmanMouseDown(int mouseX, int mouseY)
    {
        if (!cartman_.active || !IsPointInsideCartman(static_cast<float>(mouseX), static_cast<float>(mouseY)))
        {
            return false;
        }

        draggingCartman_ = true;
        cartmanDragOffsetX_ = static_cast<float>(mouseX) - cartman_.x;
        cartmanDragOffsetY_ = static_cast<float>(mouseY) - cartman_.y;
        SetCapture(window_);
        cartmanJiggleStrength_ = std::min(1.0f, cartmanJiggleStrength_ + 0.28f);
        PlaySystemSound(RandomSystemSoundAlias());
        nextCartmanDragSoundTime_ = elapsedSeconds_ + RandomRange(0.18f, 0.42f);
        return true;
    }

    void HandleCartmanMouseUp()
    {
        if (!draggingCartman_)
        {
            return;
        }

        draggingCartman_ = false;
        ReleaseCapture();
    }

    void HandleCartmanMouseMove(int mouseX, int mouseY)
    {
        if (!draggingCartman_ || !cartman_.active)
        {
            return;
        }

        const float previousX = cartman_.x;
        const float previousY = cartman_.y;
        cartman_.x = static_cast<float>(mouseX) - cartmanDragOffsetX_;
        cartman_.y = static_cast<float>(mouseY) - cartmanDragOffsetY_;
        ClampCartmanToWindow();

        cartman_.velocityX = (cartman_.x - previousX) * 14.0f;
        cartman_.velocityY = (cartman_.y - previousY) * 14.0f;
        cartmanJiggleStrength_ = std::min(1.0f, cartmanJiggleStrength_ + 0.16f);

        const float movement = std::abs(cartman_.x - previousX) + std::abs(cartman_.y - previousY);
        if (movement > 2.0f && elapsedSeconds_ >= nextCartmanDragSoundTime_)
        {
            PlaySystemSound(RandomSystemSoundAlias());
            nextCartmanDragSoundTime_ = elapsedSeconds_ + RandomRange(0.16f, 0.40f);
        }
    }

    void HandleCartmanDoubleClick(int mouseX, int mouseY)
    {
        if (!cartman_.active || !IsPointInsideCartman(static_cast<float>(mouseX), static_cast<float>(mouseY)))
        {
            return;
        }

        cartman_.targetScale = std::min(cartman_.targetScale + 0.34f, 3.4f);
        cartmanJiggleStrength_ = std::min(1.0f, cartmanJiggleStrength_ + 0.55f);
        cartman_.velocityX += RandomRange(-120.0f, 120.0f);
        cartman_.velocityY += RandomRange(-120.0f, 120.0f);
        activeJoke_ = "Cartman got bigger. Respect his authoritah.";
        jokeTimer_ = 2.2f;
        PlaySystemSound(RandomSystemSoundAlias());
    }

    bool IsPointInsideCartman(float x, float y) const
    {
        if (!cartman_.active)
        {
            return false;
        }

        const float textureAspect = static_cast<float>(cartmanTexture_.height) / static_cast<float>(cartmanTexture_.width);
        const float halfWidth = clientWidth_ * 0.10f * cartman_.scale * 0.5f;
        const float halfHeight = clientWidth_ * 0.10f * textureAspect * cartman_.scale * 0.5f;
        return x >= (cartman_.x - halfWidth) && x <= (cartman_.x + halfWidth) &&
            y >= (cartman_.y - halfHeight) && y <= (cartman_.y + halfHeight);
    }

    void ClampCartmanToWindow()
    {
        const float textureAspect = static_cast<float>(cartmanTexture_.height) / static_cast<float>(cartmanTexture_.width);
        const float halfWidth = clientWidth_ * 0.10f * cartman_.scale * 0.5f;
        const float halfHeight = clientWidth_ * 0.10f * textureAspect * cartman_.scale * 0.5f;

        cartman_.x = std::clamp(cartman_.x, halfWidth, clientWidth_ - halfWidth);
        cartman_.y = std::clamp(cartman_.y, halfHeight, clientHeight_ - halfHeight);
    }

    void DrawOverlay() const
    {
        glBindTexture(GL_TEXTURE_2D, 0);
        glDisable(GL_TEXTURE_2D);

        DrawExitButton();
        DrawOptionsButton();

        DrawText(16.0f, 28.0f, "FPS: " + FormatFloat(fps_));
        DrawText(16.0f, 56.0f, "Clicks: " + std::to_string(clickCount_));
        DrawText(16.0f, 84.0f, "Controls: Mouse click=deform/eat logo, double-click Cartman=grow, drag Cartman=move.");
        DrawText(16.0f, 112.0f, "A/D spin | W/S fly | Arrows grow/rotate scene | Space shader laser | F11 fullscreen | Esc quit.");
        DrawText(16.0f, 140.0f, "Flight meter: " + std::to_string(static_cast<int>(flyDistance_)) + " / " + std::to_string(static_cast<int>(maxFlyDistance_)));
        DrawText(16.0f, 168.0f, "Cartman ate: " + std::to_string(cartman_.logosEaten));
        DrawText(16.0f, 196.0f, "You ate: " + std::to_string(playerLogoEatCount_) + " / " + std::to_string(targetLogoEatCount_));
        DrawText(16.0f, 224.0f, "Win target: " + std::to_string(targetLogoEatCount_) + " logos");
        DrawCenteredText(static_cast<float>(clientWidth_) * 0.5f, 26.0f, "can you outeat cartman at comedy central?", 1.0f, 0.92f, 0.56f, 1.0f);

        if (!logoTexture_.loadedFromFile)
        {
            DrawText(16.0f, 252.0f, "Place cc.png next to the project or executable to use your logo.", 1.0f, 0.75f, 0.45f);
        }

        for (const auto& logo : spawnedLogos_)
        {
            if (!logo.exploding)
            {
                DrawText(logo.x - 20.0f, logo.y - 18.0f, std::to_string(logo.hitCount), 1.0f, 0.95f, 0.6f, 0.8f);
            }
        }

        if (gameOver_)
        {
            DrawCenteredText(static_cast<float>(clientWidth_) * 0.5f, static_cast<float>(clientHeight_) * 0.5f, "GAME OVER", 1.0f, 0.2f, 0.2f, 2.2f);
            DrawCenteredText(static_cast<float>(clientWidth_) * 0.5f, (static_cast<float>(clientHeight_) * 0.5f) + 54.0f, gameOverJoke_, 0.96f, 0.92f, 0.55f, 1.0f);
        }
        else if (playerWon_)
        {
            DrawCenteredText(static_cast<float>(clientWidth_) * 0.5f, static_cast<float>(clientHeight_) * 0.5f, "you may have won this war..", 1.0f, 0.95f, 0.85f, 1.8f);
        }
        else if (jokeTimer_ > 0.0f && !activeJoke_.empty())
        {
            DrawCenteredText(static_cast<float>(clientWidth_) * 0.5f, static_cast<float>(clientHeight_) - 42.0f, activeJoke_, 0.95f, 0.95f, 0.70f, 0.95f);
        }

        glEnable(GL_TEXTURE_2D);
    }

    void DrawText(float x, float y, const std::string& text, float red = 0.92f, float green = 0.96f, float blue = 1.0f, float scale = 1.0f) const
    {
        if (fontBase_ == 0)
        {
            return;
        }

        glColor3f(red, green, blue);
        glPushMatrix();
        glTranslatef(x, y, 0.0f);
        glScalef(scale, scale, 1.0f);
        glListBase(fontBase_);
        glRasterPos2f(0.0f, 0.0f);
        glCallLists(static_cast<GLsizei>(text.size()), GL_UNSIGNED_BYTE, text.c_str());
        glPopMatrix();
    }

    void DrawCenteredText(float centerX, float centerY, const std::string& text, float red, float green, float blue, float scale) const
    {
        const float approximateWidth = static_cast<float>(text.size()) * 14.0f * scale;
        DrawText(centerX - (approximateWidth * 0.5f), centerY, text, red, green, blue, scale);
    }

    struct LogoRect
    {
        float centerX = 0.0f;
        float centerY = 0.0f;
        float halfWidth = 0.0f;
        float halfHeight = 0.0f;
    };

    LogoRect GetLogoRect(float overrideScaleX = -1.0f, float overrideScaleY = -1.0f, float timeOffset = 0.0f) const
    {
        const float textureAspect = static_cast<float>(logoTexture_.height) / static_cast<float>(logoTexture_.width);
        float fitWidth = clientWidth_ * 0.92f;
        float fitHeight = fitWidth * textureAspect;
        if (fitHeight > clientHeight_ * 0.92f)
        {
            fitHeight = clientHeight_ * 0.92f;
            fitWidth = fitHeight / textureAspect;
        }

        float baseWidth = fitWidth * 0.42f;
        float baseHeight = fitHeight * 0.42f;

        const float activeScaleX = overrideScaleX > 0.0f ? overrideScaleX : scaleX_;
        const float activeScaleY = overrideScaleY > 0.0f ? overrideScaleY : scaleY_;
        const float pulse = 1.0f + std::sin((elapsedSeconds_ + timeOffset) * 4.2f) * 0.03f;
        const float jiggleX = std::sin(elapsedSeconds_ * 18.0f) * jiggleStrength_ * sceneJiggleScale_ * 12.0f;
        const float jiggleY = std::cos(elapsedSeconds_ * 22.0f) * jiggleStrength_ * sceneJiggleScale_ * 12.0f;

        LogoRect rect;
        rect.centerX = clientWidth_ * 0.5f + bounceOffsetX_ + jiggleX;
        rect.centerY = clientHeight_ * 0.5f + bounceOffsetY_ + jiggleY;
        rect.halfWidth = baseWidth * activeScaleX * pulse * 0.5f;
        rect.halfHeight = baseHeight * activeScaleY * pulse * 0.5f;
        return rect;
    }

    float GetLogoDepth() const
    {
        return -120.0f + std::sin(elapsedSeconds_ * 1.15f) * 80.0f;
    }

    void HandleClick(int mouseX, int mouseY)
    {
        if (gameOver_)
        {
            return;
        }

        if (HandleSpawnedLogoClick(mouseX, mouseY))
        {
            return;
        }

        const LogoRect logoRect = GetLogoRect();
        if (mouseX < (logoRect.centerX - logoRect.halfWidth) || mouseX > (logoRect.centerX + logoRect.halfWidth) ||
            mouseY < (logoRect.centerY - logoRect.halfHeight) || mouseY > (logoRect.centerY + logoRect.halfHeight))
        {
            ApplyBackgroundClickChaos();
            PlaySystemSound(L"SystemQuestion");
            return;
        }

        const float localX = std::clamp((mouseX - (logoRect.centerX - logoRect.halfWidth)) / (logoRect.halfWidth * 2.0f), 0.0f, 1.0f);
        const float localY = std::clamp((mouseY - (logoRect.centerY - logoRect.halfHeight)) / (logoRect.halfHeight * 2.0f), 0.0f, 1.0f);
        const bool hugeClick = localX > 0.82f && localY > 0.82f;
        const float sizeBoost = hugeClick ? 1.45f : 1.0f;

        targetScaleX_ = std::clamp(localY * 2.4f * sizeBoost, 0.35f, 3.5f);
        targetScaleY_ = std::clamp(localX * 2.4f * sizeBoost, 0.35f, 3.5f);
        targetRotation_ = (localX - 0.5f) * 42.0f + RandomRange(-14.0f, 14.0f);
        jiggleStrength_ = std::min(1.0f, jiggleStrength_ + 0.35f);
        bounceVelocityX_ += RandomRange(-260.0f, 260.0f);
        bounceVelocityY_ += RandomRange(-260.0f, 260.0f);
        targetSpinRate_ = RandomRange(-96.0f, 96.0f);
        targetBackgroundTop_ = RandomGradientColor();
        targetBackgroundBottom_ = RandomGradientColor();
        backgroundGradientTimer_ = RandomRange(1.5f, 3.2f);
        ++clickCount_;

        PlaySystemSound((clickCount_ % 2 == 0) ? L"SystemAsterisk" : L"SystemExclamation");

        if (clickCount_ >= 8)
        {
            std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
            const float chance = std::min(0.08f + ((clickCount_ - 8) * 0.035f), 0.45f);
            if (distribution(rng_) <= chance)
            {
                TriggerExplosion();
            }
        }
    }

    void TriggerExplosion()
    {
        if (gameOver_)
        {
            return;
        }

        exploding_ = true;
        gameOver_ = true;
        gameOverJoke_ = RandomJoke();
        gameOverCloseTime_ = elapsedSeconds_ + 4.0f;
        BuildFragments();
        PlaySystemSound(L"SystemHand");
    }

    void ApplyNumberDeformation(int number)
    {
        const float digitFactor = static_cast<float>(number) / 9.0f;
        deformState_.amount = RandomRange(0.18f, 0.45f) + digitFactor * 0.55f;
        deformState_.freqX = RandomRange(2.0f, 10.0f);
        deformState_.freqY = RandomRange(2.0f, 10.0f);
        deformState_.twist = RandomRange(-3.8f, 3.8f);
        deformState_.bulge = RandomRange(-1.3f, 1.7f);
        deformState_.shear = RandomRange(-1.1f, 1.1f);

        targetSpinRate_ = RandomRange(-140.0f, 140.0f);
        jiggleStrength_ = std::min(1.0f, jiggleStrength_ + 0.45f);
        activeJoke_ = RandomJoke();
        jokeTimer_ = 3.0f;
        PlaySystemSound(RandomSystemSoundAlias());

        const float painLevel = deformState_.amount + std::abs(deformState_.twist) * 0.24f + std::abs(deformState_.bulge) * 0.2f + std::abs(deformState_.shear) * 0.18f;
        if (painLevel > 1.15f)
        {
            ouchTimer_ = 2.5f;
        }

        MaybeTriggerProceduralBackground();
        UpdateWindowTitle();
    }

    void SpawnRandomLogo()
    {
        if (spawnedLogos_.size() >= 27)
        {
            spawnedLogos_.erase(spawnedLogos_.begin());
        }

        const float marginX = clientWidth_ * 0.14f;
        const float marginY = clientHeight_ * 0.14f;

        LogoOrb logo;
        logo.x = RandomRange(marginX, clientWidth_ - marginX);
        logo.y = RandomRange(marginY, clientHeight_ - marginY);
        logo.z = RandomRange(-360.0f, 220.0f);
        logo.velocityX = RandomRange(-180.0f, 180.0f);
        logo.velocityY = RandomRange(-180.0f, 180.0f);
        logo.velocityZ = RandomRange(-70.0f, 70.0f);
        logo.scale = RandomRange(0.22f, 0.42f);
        logo.targetScale = RandomRange(0.85f, 1.45f);
        logo.spinRate = RandomRange(-120.0f, 120.0f);
        logo.gravityPolarity = spawnReverseGravity_ ? -1.0f : 1.0f;
        logo.gravityStrength = 18000.0f * logoGravityScale_;
        logo.lightColor = RandomGradientColor();
        logo.baseColor = Color3{
            0.55f + logo.lightColor.r * 0.6f,
            0.55f + logo.lightColor.g * 0.6f,
            0.55f + logo.lightColor.b * 0.4f
        };

        spawnedLogos_.push_back(logo);
        spawnReverseGravity_ = !spawnReverseGravity_;
        activeJoke_ = std::string("New CC logo spawned with ") + (logo.gravityPolarity < 0.0f ? "reverse" : "normal") + " gravity.";
        jokeTimer_ = 2.6f;
        PlaySystemSound(RandomSystemSoundAlias());
        MaybeTriggerProceduralBackground();
    }

    void UpdateSpawnedLogos(float deltaTime)
    {
        if (spawnedLogos_.empty())
        {
            if (cartman_.active)
            {
                cartman_.velocityX += RandomRange(-20.0f, 20.0f) * deltaTime;
                cartman_.velocityY += RandomRange(-20.0f, 20.0f) * deltaTime;
            }

            return;
        }

        const float textureAspect = static_cast<float>(logoTexture_.height) / static_cast<float>(logoTexture_.width);
        float fitWidth = clientWidth_ * 0.92f;
        float fitHeight = fitWidth * textureAspect;
        if (fitHeight > clientHeight_ * 0.92f)
        {
            fitHeight = clientHeight_ * 0.92f;
            fitWidth = fitHeight / textureAspect;
        }

        const float polarityWave = std::sin(elapsedSeconds_ * 1.25f);
        const float percussion = 0.55f + std::pow(std::max(0.0f, std::sin(elapsedSeconds_ * 3.8f)), 2.0f) * 1.35f;

        for (auto& logo : spawnedLogos_)
        {
            if (logo.exploding || logo.imploding)
            {
                if (logo.imploding)
                {
                    logo.implodeAge += deltaTime;
                    for (auto& fragment : logo.pixelFragments)
                    {
                        fragment.offsetX -= fragment.velocityX * deltaTime;
                        fragment.offsetY -= fragment.velocityY * deltaTime;
                        fragment.offsetZ -= fragment.velocityZ * deltaTime;
                        fragment.rotation += fragment.rotationSpeed * deltaTime;
                        fragment.velocityY -= 280.0f * deltaTime;
                        fragment.velocityZ -= 140.0f * deltaTime;
                    }
                }

                continue;
            }

            Color3 litColor = logo.baseColor;

            for (const auto& influencer : spawnedLogos_)
            {
                if (&logo == &influencer || influencer.exploding)
                {
                    continue;
                }

                const float dx = influencer.x - logo.x;
                const float dy = influencer.y - logo.y;
                const float dz = influencer.z - logo.z;
                const float distanceSquared = std::max(2200.0f, (dx * dx) + (dy * dy) + (dz * dz));
                const float distance = std::sqrt(distanceSquared);
                const float force = ((influencer.gravityStrength * logoGravityScale_) / distanceSquared) * influencer.gravityPolarity * polarityWave * percussion;
                const float directionX = dx / distance;
                const float directionY = dy / distance;
                const float directionZ = dz / distance;

                logo.velocityX += directionX * force * deltaTime * 3000.0f;
                logo.velocityY += directionY * force * deltaTime * 3000.0f;
                logo.velocityZ += directionZ * force * deltaTime * 2200.0f;

                const float lightInfluence = std::clamp(1.0f - (distance / 420.0f), 0.0f, 1.0f) * 0.45f;
                litColor.r += influencer.lightColor.r * lightInfluence;
                litColor.g += influencer.lightColor.g * lightInfluence;
                litColor.b += influencer.lightColor.b * lightInfluence;
            }

            logo.baseColor.r += (std::clamp(litColor.r, 0.25f, 1.25f) - logo.baseColor.r) * std::clamp(deltaTime * 3.0f, 0.0f, 1.0f);
            logo.baseColor.g += (std::clamp(litColor.g, 0.25f, 1.25f) - logo.baseColor.g) * std::clamp(deltaTime * 3.0f, 0.0f, 1.0f);
            logo.baseColor.b += (std::clamp(litColor.b, 0.25f, 1.25f) - logo.baseColor.b) * std::clamp(deltaTime * 3.0f, 0.0f, 1.0f);
        }

        for (auto& logo : spawnedLogos_)
        {
            if (logo.exploding)
            {
                logo.explosionAge += deltaTime;
                for (auto& fragment : logo.pixelFragments)
                {
                    fragment.offsetX += fragment.velocityX * deltaTime;
                    fragment.offsetY += fragment.velocityY * deltaTime;
                    fragment.offsetZ += fragment.velocityZ * deltaTime;
                    fragment.rotation += fragment.rotationSpeed * deltaTime;
                    fragment.velocityY += 360.0f * deltaTime;
                    fragment.velocityZ += 120.0f * deltaTime;
                }

                continue;
            }

            logo.scale += (logo.targetScale - logo.scale) * std::clamp(deltaTime * 1.35f, 0.0f, 1.0f);
            logo.x += logo.velocityX * deltaTime;
            logo.y += logo.velocityY * deltaTime;
            logo.z += logo.velocityZ * deltaTime;
            logo.spinAngle += logo.spinRate * deltaTime;
            logo.spinRate += RandomRange(-16.0f, 16.0f) * deltaTime;
            logo.velocityX *= 0.999f;
            logo.velocityY *= 0.999f;
            logo.velocityZ *= 0.999f;

            const float halfWidth = fitWidth * 0.11f * logo.scale;
            const float halfHeight = fitHeight * 0.11f * logo.scale;

            if (logo.x < halfWidth)
            {
                logo.x = halfWidth;
                logo.velocityX = std::abs(logo.velocityX);
            }
            else if (logo.x > clientWidth_ - halfWidth)
            {
                logo.x = clientWidth_ - halfWidth;
                logo.velocityX = -std::abs(logo.velocityX);
            }

            if (logo.y < halfHeight)
            {
                logo.y = halfHeight;
                logo.velocityY = std::abs(logo.velocityY);
            }
            else if (logo.y > clientHeight_ - halfHeight)
            {
                logo.y = clientHeight_ - halfHeight;
                logo.velocityY = -std::abs(logo.velocityY);
            }

            if (logo.z < -520.0f)
            {
                logo.z = -520.0f;
                logo.velocityZ = std::abs(logo.velocityZ);
            }
            else if (logo.z > 420.0f)
            {
                logo.z = 420.0f;
                logo.velocityZ = -std::abs(logo.velocityZ);
            }
        }

        for (size_t i = 0; i < spawnedLogos_.size(); ++i)
        {
            if (spawnedLogos_[i].exploding || spawnedLogos_[i].imploding)
            {
                continue;
            }

            for (size_t j = i + 1; j < spawnedLogos_.size();)
            {
                if (spawnedLogos_[j].exploding || spawnedLogos_[j].imploding)
                {
                    ++j;
                    continue;
                }

                const float dx = spawnedLogos_[j].x - spawnedLogos_[i].x;
                const float dy = spawnedLogos_[j].y - spawnedLogos_[i].y;
                const float dz = spawnedLogos_[j].z - spawnedLogos_[i].z;
                const float distance = std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
                const float radiusI = fitWidth * 0.11f * spawnedLogos_[i].scale;
                const float radiusJ = fitWidth * 0.11f * spawnedLogos_[j].scale;
                if (distance < (radiusI + radiusJ) * 0.52f)
                {
                    const bool firstEats = spawnedLogos_[i].scale >= spawnedLogos_[j].scale;
                    LogoOrb& eater = firstEats ? spawnedLogos_[i] : spawnedLogos_[j];
                    const size_t removeIndex = firstEats ? j : i;
                    eater.targetScale = std::min(eater.targetScale + 0.16f, 2.1f);
                    eater.velocityX *= 1.04f;
                    eater.velocityY *= 1.04f;
                    eater.velocityZ *= 1.04f;
                    eater.lightColor = Color3{
                        std::clamp((eater.lightColor.r + spawnedLogos_[removeIndex].lightColor.r) * 0.5f, 0.0f, 1.0f),
                        std::clamp((eater.lightColor.g + spawnedLogos_[removeIndex].lightColor.g) * 0.5f, 0.0f, 1.0f),
                        std::clamp((eater.lightColor.b + spawnedLogos_[removeIndex].lightColor.b) * 0.5f, 0.0f, 1.0f)
                    };
                    spawnedLogos_.erase(spawnedLogos_.begin() + static_cast<std::ptrdiff_t>(removeIndex));
                    if (!firstEats)
                    {
                        if (i == 0)
                        {
                            break;
                        }

                        --i;
                    }
                    continue;
                }

                ++j;
            }
        }

        spawnedLogos_.erase(
            std::remove_if(
                spawnedLogos_.begin(),
                spawnedLogos_.end(),
                [](const LogoOrb& logo)
                {
                    return (logo.exploding && logo.explosionAge >= 1.35f) || (logo.imploding && logo.implodeAge >= 1.15f);
                }),
            spawnedLogos_.end());
    }

    void UpdateCartman(float deltaTime)
    {
        if (!cartman_.active)
        {
            return;
        }

        cartmanDisappearTimer_ -= deltaTime;
        if (cartmanDisappearTimer_ <= 0.0f && !draggingCartman_)
        {
            cartman_.active = false;
            cartmanReturnTimer_ = RandomRange(2.5f, 5.5f);
            activeJoke_ = "Cartman disappeared for a while.";
            jokeTimer_ = 2.0f;
            return;
        }

        const float polarityWave = std::sin((elapsedSeconds_ * 1.25f) + 1.1f);
        const float percussion = 0.55f + std::pow(std::max(0.0f, std::sin(elapsedSeconds_ * 3.8f)), 2.0f) * 1.35f;

        if (!spawnedLogos_.empty())
        {
            size_t nearestIndex = 0;
            float nearestDistanceSquared = std::numeric_limits<float>::max();
            for (size_t i = 0; i < spawnedLogos_.size(); ++i)
            {
                const float dx = spawnedLogos_[i].x - cartman_.x;
                const float dy = spawnedLogos_[i].y - cartman_.y;
                const float dz = spawnedLogos_[i].z - cartman_.z;
                const float distanceSquared = (dx * dx) + (dy * dy) + (dz * dz);
                if (distanceSquared < nearestDistanceSquared)
                {
                    nearestDistanceSquared = distanceSquared;
                    nearestIndex = i;
                }
            }

            const float dx = spawnedLogos_[nearestIndex].x - cartman_.x;
            const float dy = spawnedLogos_[nearestIndex].y - cartman_.y;
            const float dz = spawnedLogos_[nearestIndex].z - cartman_.z;
            const float distance = std::max(1.0f, std::sqrt((dx * dx) + (dy * dy) + (dz * dz)));
            const float interactionForce = 420.0f * polarityWave * percussion;
            cartman_.velocityX += (dx / distance) * interactionForce * deltaTime;
            cartman_.velocityY += (dy / distance) * interactionForce * deltaTime;
            cartman_.velocityZ += (dz / distance) * interactionForce * deltaTime * 0.8f;
        }

        cartman_.targetScale = std::max(0.34f, cartman_.targetScale);
        cartman_.scale += (cartman_.targetScale - cartman_.scale) * std::clamp(deltaTime * 2.0f, 0.0f, 1.0f);
        cartman_.x += cartman_.velocityX * deltaTime;
        cartman_.y += cartman_.velocityY * deltaTime;
        cartman_.z += cartman_.velocityZ * deltaTime;
        cartman_.spin += cartman_.velocityX * 0.015f * deltaTime;
        cartman_.velocityX *= 0.993f;
        cartman_.velocityY *= 0.993f;
        cartman_.velocityZ *= 0.993f;

        const float textureAspect = static_cast<float>(cartmanTexture_.height) / static_cast<float>(cartmanTexture_.width);
        const float halfWidth = clientWidth_ * 0.10f * cartman_.scale * 0.5f;
        const float halfHeight = clientWidth_ * 0.10f * textureAspect * cartman_.scale * 0.5f;

        if (cartman_.x < halfWidth)
        {
            cartman_.x = halfWidth;
            cartman_.velocityX = std::abs(cartman_.velocityX);
        }
        else if (cartman_.x > clientWidth_ - halfWidth)
        {
            cartman_.x = clientWidth_ - halfWidth;
            cartman_.velocityX = -std::abs(cartman_.velocityX);
        }

        if (cartman_.y < halfHeight)
        {
            cartman_.y = halfHeight;
            cartman_.velocityY = std::abs(cartman_.velocityY);
        }
        else if (cartman_.y > clientHeight_ - halfHeight)
        {
            cartman_.y = clientHeight_ - halfHeight;
            cartman_.velocityY = -std::abs(cartman_.velocityY);
        }

        if (cartman_.z < -520.0f)
        {
            cartman_.z = -520.0f;
            cartman_.velocityZ = std::abs(cartman_.velocityZ);
        }
        else if (cartman_.z > 260.0f)
        {
            cartman_.z = 260.0f;
            cartman_.velocityZ = -std::abs(cartman_.velocityZ);
        }

        for (size_t i = 0; i < spawnedLogos_.size();)
        {
            if (spawnedLogos_[i].exploding)
            {
                ++i;
                continue;
            }

            const float dx = spawnedLogos_[i].x - cartman_.x;
            const float dy = spawnedLogos_[i].y - cartman_.y;
            const float dz = spawnedLogos_[i].z - cartman_.z;
            const float distance = std::sqrt((dx * dx) + (dy * dy) + (dz * dz));
            const float logoRadius = clientWidth_ * 0.11f * spawnedLogos_[i].scale;
            if (distance < (halfWidth + logoRadius) * 0.62f)
            {
                cartman_.targetScale = std::min(cartman_.targetScale + spawnedLogos_[i].scale * 0.20f, 2.8f);
                ++cartman_.logosEaten;
                targetLogoEatCount_ = std::clamp(std::max(targetLogoEatCount_, cartman_.logosEaten + RandomInt(3, 8)), 20, 100);
                activeJoke_ = "Cartman ate a CC logo.";
                jokeTimer_ = 1.8f;
                PlaySystemSound(RandomSystemSoundAlias());
                spawnedLogos_.erase(spawnedLogos_.begin() + static_cast<std::ptrdiff_t>(i));
                continue;
            }

            ++i;
        }
    }

    void SpawnGradientCircle()
    {
        GradientCircle circle;
        circle.x = RandomRange(clientWidth_ * 0.1f, clientWidth_ * 0.9f);
        circle.y = RandomRange(clientHeight_ * 0.1f, clientHeight_ * 0.9f);
        circle.z = RandomRange(-1600.0f, -600.0f);
        circle.radius = 8.0f;
        circle.targetRadius = RandomRange(90.0f, 220.0f);
        circle.lifetime = RandomRange(2.8f, 5.2f);
        circle.innerColor = RandomGradientColor();
        circle.outerColor = RandomGradientColor();
        gradientCircles_.push_back(circle);
    }

    void UpdateGradientCircles(float deltaTime)
    {
        for (auto& circle : gradientCircles_)
        {
            circle.age += deltaTime;
            circle.radius += (circle.targetRadius - circle.radius) * std::clamp(deltaTime * 0.65f, 0.0f, 1.0f);
        }

        gradientCircles_.erase(
            std::remove_if(
                gradientCircles_.begin(),
                gradientCircles_.end(),
                [](const GradientCircle& circle)
                {
                    return circle.age >= circle.lifetime;
                }),
            gradientCircles_.end());
    }

    // Rebuild the node tree after the logos move so queries stay cheap.
    void UpdateLogoNodeTree()
    {
        std::vector<LogoNodeInput> inputs;
        inputs.reserve(spawnedLogos_.size());

        for (std::size_t i = 0; i < spawnedLogos_.size(); ++i)
        {
            const auto& logo = spawnedLogos_[i];
            if (logo.exploding || logo.imploding)
            {
                continue;
            }

            inputs.push_back(LogoNodeInput{ i, logo.x, logo.y, logo.scale });
        }

        logoNodeTree_.Rebuild(inputs);
    }

    // Let nearby nodes slightly reinforce each other to create loose logo clustering.
    void ApplyLogoNodeInfluence(float deltaTime)
    {
        if (logoNodeTree_.empty())
        {
            return;
        }

        std::vector<std::size_t> neighborIndices;
        for (std::size_t i = 0; i < spawnedLogos_.size(); ++i)
        {
            auto& logo = spawnedLogos_[i];
            if (logo.exploding || logo.imploding)
            {
                continue;
            }

            neighborIndices.clear();
            logoNodeTree_.GatherWithinRadius(logo.x, logo.y, 140.0f + (logo.scale * 120.0f), neighborIndices);

            float scaleSum = 0.0f;
            float colorSumR = 0.0f;
            float colorSumG = 0.0f;
            float colorSumB = 0.0f;
            std::size_t neighborCount = 0;
            for (const auto neighborIndex : neighborIndices)
            {
                if (neighborIndex == i || neighborIndex >= spawnedLogos_.size())
                {
                    continue;
                }

                const auto& neighbor = spawnedLogos_[neighborIndex];
                if (neighbor.exploding || neighbor.imploding)
                {
                    continue;
                }

                scaleSum += neighbor.scale;
                colorSumR += neighbor.lightColor.r;
                colorSumG += neighbor.lightColor.g;
                colorSumB += neighbor.lightColor.b;
                ++neighborCount;
            }

            if (neighborCount == 0)
            {
                continue;
            }

            const float averageScale = scaleSum / static_cast<float>(neighborCount);
            logo.targetScale += ((averageScale - logo.targetScale) * 0.06f) * deltaTime * 60.0f;
            logo.spinRate += ((static_cast<float>(neighborCount) - 1.0f) * 4.0f) * deltaTime;
            logo.lightColor.r += ((colorSumR / static_cast<float>(neighborCount)) - logo.lightColor.r) * 0.04f;
            logo.lightColor.g += ((colorSumG / static_cast<float>(neighborCount)) - logo.lightColor.g) * 0.04f;
            logo.lightColor.b += ((colorSumB / static_cast<float>(neighborCount)) - logo.lightColor.b) * 0.04f;

            if (neighborCount >= 3)
            {
                logo.pulseTimer = std::max(logo.pulseTimer, 0.5f);
            }
        }
    }

    // Draw a faint nearest-neighbor graph so the logos feel node-based rather than isolated.
    void DrawLogoNodeConnections() const
    {
        if (logoNodeTree_.empty() || spawnedLogos_.empty())
        {
            return;
        }

        glDisable(GL_TEXTURE_2D);
        glBegin(GL_LINES);
        for (std::size_t i = 0; i < spawnedLogos_.size(); ++i)
        {
            const auto& logo = spawnedLogos_[i];
            if (logo.exploding || logo.imploding)
            {
                continue;
            }

            const int nearestIndex = logoNodeTree_.FindNearest(logo.x, logo.y);
            if (nearestIndex < 0 || static_cast<std::size_t>(nearestIndex) == i || static_cast<std::size_t>(nearestIndex) >= spawnedLogos_.size())
            {
                continue;
            }

            const auto& nearest = spawnedLogos_[static_cast<std::size_t>(nearestIndex)];
            if (nearest.exploding || nearest.imploding)
            {
                continue;
            }

            glColor4f(0.75f, 0.92f, 1.0f, 0.14f);
            glVertex2f(logo.x, logo.y);
            glVertex2f(nearest.x, nearest.y);
        }
        glEnd();
        glEnable(GL_TEXTURE_2D);
    }

    // Background clicks perturb the whole scene with a random, per-object effect.
    void ApplyBackgroundClickChaos()
    {
        const int effectType = RandomInt(0, 5);

        targetScaleX_ = std::clamp(targetScaleX_ * RandomRange(0.75f, 1.35f), 0.35f, 3.8f);
        targetScaleY_ = std::clamp(targetScaleY_ * RandomRange(0.75f, 1.35f), 0.35f, 3.8f);
        targetRotation_ += RandomRange(-65.0f, 65.0f);
        bounceVelocityX_ += RandomRange(-120.0f, 120.0f);
        bounceVelocityY_ += RandomRange(-120.0f, 120.0f);
        jiggleStrength_ = std::min(1.0f, jiggleStrength_ + 0.25f);

        for (auto& logo : spawnedLogos_)
        {
            if (logo.exploding || logo.imploding)
            {
                continue;
            }

            switch (effectType)
            {
            case 0:
                logo.targetScale = std::clamp(logo.targetScale * RandomRange(0.72f, 1.32f), 0.16f, 3.4f);
                break;
            case 1:
                logo.pulseTimer = RandomRange(0.6f, 1.8f);
                logo.spinRate += RandomRange(-90.0f, 90.0f);
                break;
            case 2:
                // Break apart visually by using the existing fragment system.
                if (RandomChance(0.5f))
                {
                    ExplodeSpawnedLogo(logo);
                }
                else
                {
                    ImplodeSpawnedLogo(logo);
                }
                break;
            case 3:
                logo.velocityX *= RandomRange(1.20f, 1.25f);
                logo.velocityY *= RandomRange(1.20f, 1.25f);
                break;
            case 4:
                logo.gravityPolarity *= -1.0f;
                logo.gravityStrength = std::clamp(logo.gravityStrength * RandomRange(0.82f, 1.18f), 6000.0f, 28000.0f);
                break;
            case 5:
            default:
                logo.spinRate += RandomRange(-260.0f, 260.0f);
                logo.velocityX += RandomRange(-80.0f, 80.0f);
                logo.velocityY += RandomRange(-80.0f, 80.0f);
                break;
            }
        }

        if (cartman_.active)
        {
            cartman_.targetScale = std::clamp(cartman_.targetScale * RandomRange(0.85f, 1.25f), 0.34f, 4.4f);
            cartman_.velocityX *= RandomRange(1.20f, 1.25f);
            cartman_.velocityY *= RandomRange(1.20f, 1.25f);
            cartman_.spin += RandomRange(-160.0f, 160.0f);
            if (RandomChance(0.5f))
            {
                cartmanJiggleStrength_ = std::min(1.0f, cartmanJiggleStrength_ + 0.4f);
            }
        }

        if (RandomChance(0.35f))
        {
            cartmanPulseTimer_ = RandomRange(0.8f, 2.2f);
        }

        activeJoke_ = "Background click scrambled the scene.";
        jokeTimer_ = 1.8f;
        MaybeTriggerProceduralBackground();
        PlaySystemSound(RandomSystemSoundAlias());
    }

    void ImplodeSpawnedLogo(LogoOrb& logo)
    {
        if (logo.imploding)
        {
            return;
        }

        logo.imploding = true;
        logo.exploding = false;
        logo.implodeAge = 0.0f;
        logo.explosionAge = 0.0f;
        logo.pixelFragments.clear();
        logo.pixelFragments.reserve(36);

        constexpr int pixelRows = 6;
        constexpr int pixelColumns = 6;
        for (int row = 0; row < pixelRows; ++row)
        {
            for (int column = 0; column < pixelColumns; ++column)
            {
                const float normalizedX = ((column + 0.5f) / static_cast<float>(pixelColumns)) - 0.5f;
                const float normalizedY = ((row + 0.5f) / static_cast<float>(pixelRows)) - 0.5f;

                Fragment fragment;
                fragment.u0 = column / static_cast<float>(pixelColumns);
                fragment.v0 = row / static_cast<float>(pixelRows);
                fragment.u1 = (column + 1) / static_cast<float>(pixelColumns);
                fragment.v1 = (row + 1) / static_cast<float>(pixelRows);
                fragment.offsetX = normalizedX * 12.0f;
                fragment.offsetY = normalizedY * 12.0f;
                fragment.offsetZ = RandomRange(-10.0f, 10.0f);
                fragment.velocityX = -normalizedX * RandomRange(100.0f, 220.0f);
                fragment.velocityY = -normalizedY * RandomRange(100.0f, 220.0f);
                fragment.velocityZ = RandomRange(20.0f, 160.0f);
                fragment.rotationSpeed = RandomRange(-220.0f, 220.0f);
                logo.pixelFragments.push_back(fragment);
            }
        }

        logo.velocityX *= 0.25f;
        logo.velocityY *= 0.25f;
        logo.targetScale = std::max(0.0f, logo.scale * 0.4f);
    }

    void UpdateLaser(float deltaTime)
    {
        if (!spaceHeld_)
        {
            laserSoundTimer_ = 0.0f;
            return;
        }

        laserSoundTimer_ -= deltaTime;
        if (laserSoundTimer_ <= 0.0f)
        {
            PlaySystemSound(laserStereoFlip_ ? L"SystemAsterisk" : L"SystemExclamation");
            laserStereoFlip_ = !laserStereoFlip_;
            laserSoundTimer_ = 0.14f;
        }

        const LogoRect logoRect = GetLogoRect();
        const float angle = (spinAngle_ + rotation_ + laserDirectionOffset_) * 0.0174532925f;
        const float directionX = std::cos(angle);
        const float directionY = std::sin(angle);
        const float length = std::sqrt((clientWidth_ * clientWidth_) + (clientHeight_ * clientHeight_));
        const float startX = logoRect.centerX + directionX * (logoRect.halfWidth + 8.0f);
        const float startY = logoRect.centerY + directionY * (logoRect.halfHeight + 8.0f);
        const float endX = startX + directionX * length * 3.0f;
        const float endY = startY + directionY * length * 3.0f;
        const float beamRadius = 20.0f * 3.0f;

        for (auto& logo : spawnedLogos_)
        {
            if (logo.exploding || logo.imploding)
            {
                continue;
            }

            const float hitRadius = 24.0f + (logo.scale * 18.0f);
            if (DistancePointToSegment(logo.x, logo.y, startX, startY, endX, endY) <= beamRadius + hitRadius)
            {
                ImplodeSpawnedLogo(logo);
            }
        }

        if (cartman_.active)
        {
            const float cartmanHitRadius = 26.0f + (cartman_.scale * 22.0f);
            if (DistancePointToSegment(cartman_.x, cartman_.y, startX, startY, endX, endY) <= beamRadius + cartmanHitRadius)
            {
                cartman_.targetScale = std::min(cartman_.targetScale + (deltaTime * 0.85f), 4.2f);
                cartmanJiggleStrength_ = std::min(1.0f, cartmanJiggleStrength_ + (deltaTime * 0.8f));
            }
        }
    }

    static float DistancePointToSegment(float pointX, float pointY, float startX, float startY, float endX, float endY)
    {
        const float segmentX = endX - startX;
        const float segmentY = endY - startY;
        const float segmentLengthSquared = (segmentX * segmentX) + (segmentY * segmentY);
        if (segmentLengthSquared <= 0.0001f)
        {
            return std::sqrt(((pointX - startX) * (pointX - startX)) + ((pointY - startY) * (pointY - startY)));
        }

        const float projection = std::clamp((((pointX - startX) * segmentX) + ((pointY - startY) * segmentY)) / segmentLengthSquared, 0.0f, 1.0f);
        const float closestX = startX + (segmentX * projection);
        const float closestY = startY + (segmentY * projection);
        const float deltaX = pointX - closestX;
        const float deltaY = pointY - closestY;
        return std::sqrt((deltaX * deltaX) + (deltaY * deltaY));
    }

    bool HandleSpawnedLogoClick(int mouseX, int mouseY)
    {
        for (auto& logo : spawnedLogos_)
        {
            if (logo.exploding || !IsPointInsideSpawnedLogo(logo, static_cast<float>(mouseX), static_cast<float>(mouseY)))
            {
                continue;
            }

            ++logo.hitCount;
            ++playerLogoEatCount_;
            if (playerLogoEatCount_ >= targetLogoEatCount_ && playerLogoEatCount_ > cartman_.logosEaten)
            {
                playerWon_ = true;
            }

            ExplodeSpawnedLogo(logo);
            activeJoke_ = "A bonus logo pixelated and exploded.";
            jokeTimer_ = 1.8f;
            PlaySystemSound(RandomSystemSoundAlias());
            MaybeTriggerProceduralBackground();
            return true;
        }

        return false;
    }

    bool IsPointInsideSpawnedLogo(const LogoOrb& logo, float x, float y) const
    {
        const float textureAspect = static_cast<float>(logoTexture_.height) / static_cast<float>(logoTexture_.width);
        float fitWidth = clientWidth_ * 0.92f;
        float fitHeight = fitWidth * textureAspect;
        if (fitHeight > clientHeight_ * 0.92f)
        {
            fitHeight = clientHeight_ * 0.92f;
            fitWidth = fitHeight / textureAspect;
        }

        const float halfWidth = fitWidth * 0.11f * logo.scale;
        const float halfHeight = fitHeight * 0.11f * logo.scale;
        return x >= (logo.x - halfWidth) && x <= (logo.x + halfWidth) &&
            y >= (logo.y - halfHeight) && y <= (logo.y + halfHeight);
    }

    void ExplodeSpawnedLogo(LogoOrb& logo)
    {
        logo.exploding = true;
        logo.imploding = false;
        logo.explosionAge = 0.0f;
        logo.implodeAge = 0.0f;
        logo.pixelFragments.clear();
        logo.pixelFragments.reserve(36);

        constexpr int pixelRows = 6;
        constexpr int pixelColumns = 6;
        for (int row = 0; row < pixelRows; ++row)
        {
            for (int column = 0; column < pixelColumns; ++column)
            {
                const float normalizedX = ((column + 0.5f) / static_cast<float>(pixelColumns)) - 0.5f;
                const float normalizedY = ((row + 0.5f) / static_cast<float>(pixelRows)) - 0.5f;

                Fragment fragment;
                fragment.u0 = column / static_cast<float>(pixelColumns);
                fragment.v0 = row / static_cast<float>(pixelRows);
                fragment.u1 = (column + 1) / static_cast<float>(pixelColumns);
                fragment.v1 = (row + 1) / static_cast<float>(pixelRows);
                fragment.offsetX = normalizedX * 10.0f;
                fragment.offsetY = normalizedY * 10.0f;
                fragment.velocityX = normalizedX * RandomRange(120.0f, 260.0f) + RandomRange(-30.0f, 30.0f);
                fragment.velocityY = normalizedY * RandomRange(120.0f, 260.0f) - RandomRange(20.0f, 120.0f);
                fragment.rotationSpeed = RandomRange(-220.0f, 220.0f);
                logo.pixelFragments.push_back(fragment);
            }
        }

        logo.velocityX = 0.0f;
        logo.velocityY = 0.0f;
    }

    void ApplyArrowKeyChaos()
    {
        targetScaleX_ = std::min(targetScaleX_ + 0.18f, 3.8f);
        targetScaleY_ = std::min(targetScaleY_ + 0.18f, 3.8f);
        targetRotation_ += RandomRange(-55.0f, 55.0f);
        jiggleStrength_ = std::min(1.0f, jiggleStrength_ + 0.22f);

        for (auto& logo : spawnedLogos_)
        {
            if (logo.exploding)
            {
                continue;
            }

            logo.targetScale = std::min(logo.targetScale + RandomRange(0.08f, 0.24f), 3.4f);
            logo.spinRate += RandomRange(-120.0f, 120.0f);
            if (RandomChance(0.35f))
            {
                logo.pulseTimer = RandomRange(1.2f, 2.4f);
            }
        }

        if (cartman_.active)
        {
            cartman_.targetScale = std::min(cartman_.targetScale + 0.22f, 4.4f);
            cartman_.spin += RandomRange(-32.0f, 32.0f);
            if (RandomChance(0.45f))
            {
                cartmanJiggleStrength_ = std::min(1.0f, cartmanJiggleStrength_ + 0.35f);
                cartmanPulseTimer_ = RandomRange(1.2f, 2.3f);
            }
        }

        MaybeTriggerProceduralBackground();
        activeJoke_ = "Arrow chaos boosted the whole scene.";
        jokeTimer_ = 1.8f;
        PlaySystemSound(RandomSystemSoundAlias());
    }

    bool HandleExitButtonClick(int mouseX, int mouseY)
    {
        if (IsPointInsideExitButton(static_cast<float>(mouseX), static_cast<float>(mouseY)))
        {
            activeJoke_ = "Exit button pressed.";
            jokeTimer_ = 1.0f;
            running_ = false;
            DestroyWindow(window_);
            return true;
        }

        return false;
    }

    bool IsPointInsideExitButton(float x, float y) const
    {
        const ExitButtonRect rect = GetExitButtonRect();
        return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
    }

    struct ExitButtonRect
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
    };

    ExitButtonRect GetExitButtonRect() const
    {
        constexpr float baseWidth = 104.0f;
        constexpr float baseHeight = 38.0f;
        const float pulse = 1.0f + std::sin(elapsedSeconds_ * 6.5f) * 0.08f;
        const float width = baseWidth * pulse;
        const float height = baseHeight * pulse;
        return ExitButtonRect{ static_cast<float>(clientWidth_) - 12.0f - width, 12.0f, static_cast<float>(clientWidth_) - 12.0f, 12.0f + height };
    }

    void DrawExitButton() const
    {
        const ExitButtonRect rect = GetExitButtonRect();
        const float pulse = 1.0f + std::sin(elapsedSeconds_ * 6.5f) * 0.08f;
        const float red = 0.55f + std::sin(elapsedSeconds_ * 7.0f) * 0.15f;
        const float green = 0.12f + std::sin(elapsedSeconds_ * 5.0f) * 0.05f;
        const float blue = 0.18f + std::cos(elapsedSeconds_ * 4.0f) * 0.04f;

        glBegin(GL_QUADS);
        glColor4f(red, green, blue, 0.95f);
        glVertex2f(rect.left, rect.top);
        glVertex2f(rect.right, rect.top);
        glColor4f(0.85f, 0.20f, 0.18f, 0.98f);
        glVertex2f(rect.right, rect.bottom);
        glVertex2f(rect.left, rect.bottom);
        glEnd();

        DrawText(rect.left + 18.0f, rect.top + 25.0f, std::string("EXIT"), 1.0f, 0.98f, 0.95f, pulse * 0.95f);
    }

    bool HandleOptionsButtonClick(int mouseX, int mouseY)
    {
        if (!IsPointInsideOptionsButton(static_cast<float>(mouseX), static_cast<float>(mouseY)))
        {
            return false;
        }

        OpenOptionsWindow();
        return true;
    }

    bool IsPointInsideOptionsButton(float x, float y) const
    {
        const OptionsButtonRect rect = GetOptionsButtonRect();
        return x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
    }

    struct OptionsButtonRect
    {
        float left = 0.0f;
        float top = 0.0f;
        float right = 0.0f;
        float bottom = 0.0f;
    };

    OptionsButtonRect GetOptionsButtonRect() const
    {
        const ExitButtonRect exitRect = GetExitButtonRect();
        constexpr float gap = 10.0f;
        const float width = 122.0f;
        const float height = exitRect.bottom - exitRect.top;
        return OptionsButtonRect{ static_cast<float>(clientWidth_) - 12.0f - width, exitRect.bottom + gap, static_cast<float>(clientWidth_) - 12.0f, exitRect.bottom + gap + height };
    }

    void DrawOptionsButton() const
    {
        const OptionsButtonRect rect = GetOptionsButtonRect();
        const float glow = 0.55f + std::sin(elapsedSeconds_ * 7.5f) * 0.45f;

        glBegin(GL_QUADS);
        glColor4f(0.02f, 0.18f, 0.04f, 0.80f);
        glVertex2f(rect.left - 5.0f, rect.top - 5.0f);
        glVertex2f(rect.right + 5.0f, rect.top - 5.0f);
        glVertex2f(rect.right + 5.0f, rect.bottom + 5.0f);
        glVertex2f(rect.left - 5.0f, rect.bottom + 5.0f);
        glEnd();

        glBegin(GL_QUADS);
        glColor4f(0.10f + glow * 0.15f, 0.55f + glow * 0.35f, 0.12f + glow * 0.20f, 0.96f);
        glVertex2f(rect.left, rect.top);
        glColor4f(0.12f, 0.80f, 0.18f, 0.98f);
        glVertex2f(rect.right, rect.top);
        glColor4f(0.05f, 0.45f, 0.06f, 0.98f);
        glVertex2f(rect.right, rect.bottom);
        glColor4f(0.12f, 0.90f, 0.20f, 0.98f);
        glVertex2f(rect.left, rect.bottom);
        glEnd();

        DrawText(rect.left + 18.0f, rect.top + 25.0f, std::string("options"), 0.82f, 1.0f, 0.85f, 0.98f + glow * 0.12f);
    }

    void ApplyPresentationSettings()
    {
        if (swapIntervalProc_ == nullptr)
        {
            return;
        }

        swapIntervalProc_((vsyncEnabled_ || tripleBufferingEnabled_) ? 1 : 0);
    }

    void InitializeCommonControls()
    {
        INITCOMMONCONTROLSEX controls{};
        controls.dwSize = sizeof(controls);
        controls.dwICC = ICC_BAR_CLASSES;
        InitCommonControlsEx(&controls);
    }

    void RefreshResolutionOptions()
    {
        resolutionOptions_.clear();

        DEVMODEW mode{};
        mode.dmSize = sizeof(mode);
        for (DWORD index = 0; EnumDisplaySettingsW(nullptr, index, &mode) != 0; ++index)
        {
            if (mode.dmPelsWidth < 800 || mode.dmPelsHeight < 600)
            {
                continue;
            }

            const ResolutionOption option{ static_cast<int>(mode.dmPelsWidth), static_cast<int>(mode.dmPelsHeight) };
            if (std::find_if(resolutionOptions_.begin(), resolutionOptions_.end(), [option](const ResolutionOption& existing)
                {
                    return existing.width == option.width && existing.height == option.height;
                }) == resolutionOptions_.end())
            {
                resolutionOptions_.push_back(option);
            }
        }

        std::sort(resolutionOptions_.begin(), resolutionOptions_.end(), [](const ResolutionOption& a, const ResolutionOption& b)
            {
                if (a.width == b.width)
                {
                    return a.height < b.height;
                }

                return a.width < b.width;
            });

        const auto current = std::find_if(resolutionOptions_.begin(), resolutionOptions_.end(), [this](const ResolutionOption& option)
            {
                return option.width == clientWidth_ && option.height == clientHeight_;
            });

        if (current == resolutionOptions_.end())
        {
            resolutionOptions_.insert(resolutionOptions_.begin(), ResolutionOption{ clientWidth_, clientHeight_ });
        }
    }

    void OpenOptionsWindow()
    {
        if (optionsWindow_ != nullptr)
        {
            ShowWindow(optionsWindow_, SW_SHOW);
            SetForegroundWindow(optionsWindow_);
            return;
        }

        WNDCLASSEXW optionsClass{};
        optionsClass.cbSize = sizeof(optionsClass);
        optionsClass.style = CS_DBLCLKS;
        optionsClass.lpfnWndProc = OptionsWindowProc;
        optionsClass.hInstance = instance_;
        optionsClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
        optionsClass.hIcon = LoadIconW(nullptr, IDI_APPLICATION);
        optionsClass.hbrBackground = GetSysColorBrush(COLOR_3DFACE);
        optionsClass.lpszClassName = L"CCClickGameOptionsWindow";

        if (RegisterClassExW(&optionsClass) == 0)
        {
            const DWORD lastError = GetLastError();
            if (lastError != ERROR_CLASS_ALREADY_EXISTS)
            {
                return;
            }
        }

        RECT rect{ 0, 0, 500, 380 };
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        RECT ownerRect{};
        GetWindowRect(window_, &ownerRect);
        const int x = ownerRect.left + ((ownerRect.right - ownerRect.left) - (rect.right - rect.left)) / 2;
        const int y = ownerRect.top + ((ownerRect.bottom - ownerRect.top) - (rect.bottom - rect.top)) / 2;

        optionsWindow_ = CreateWindowExW(
            WS_EX_APPWINDOW,
            optionsClass.lpszClassName,
            L"Options",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            x,
            y,
            rect.right - rect.left,
            rect.bottom - rect.top,
            window_,
            nullptr,
            instance_,
            this);
    }

    void CloseOptionsWindow()
    {
        if (optionsWindow_ != nullptr)
        {
            DestroyWindow(optionsWindow_);
            optionsWindow_ = nullptr;
        }
    }

    static LRESULT CALLBACK OptionsWindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
    {
        auto* game = reinterpret_cast<Game*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        if (message == WM_NCCREATE)
        {
            auto* createStruct = reinterpret_cast<CREATESTRUCTW*>(lParam);
            game = reinterpret_cast<Game*>(createStruct->lpCreateParams);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(game));
        }

        if (game != nullptr)
        {
            switch (message)
            {
            case WM_CREATE:
                game->optionsWindow_ = hwnd;
                game->InitializeOptionsControls(hwnd);
                return 0;
            case WM_COMMAND:
                game->HandleOptionsCommand(hwnd, LOWORD(wParam), HIWORD(wParam));
                return 0;
            case WM_HSCROLL:
                game->HandleOptionsScroll(hwnd, reinterpret_cast<HWND>(lParam));
                return 0;
            case WM_CLOSE:
                DestroyWindow(hwnd);
                return 0;
            case WM_DESTROY:
                if (game->optionsWindow_ == hwnd)
                {
                    game->optionsWindow_ = nullptr;
                }
                return 0;
            }
        }

        return DefWindowProcW(hwnd, message, wParam, lParam);
    }

    void InitializeOptionsControls(HWND hwnd)
    {
        constexpr int left = 18;
        constexpr int top = 18;
        constexpr int comboWidth = 220;
        constexpr int buttonWidth = 100;
        constexpr int trackLeft = 150;
        constexpr int trackWidth = 250;
        constexpr int valueLeft = 410;

        CreateWindowExW(0, L"STATIC", L"Resolution", WS_CHILD | WS_VISIBLE,
            left, top, 110, 20, hwnd, nullptr, instance_, nullptr);
        CreateWindowExW(0, L"COMBOBOX", nullptr, WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_VSCROLL,
            left, top + 20, comboWidth, 200, hwnd, reinterpret_cast<HMENU>(static_cast<int>(OptionsControlId::ResolutionCombo)), instance_, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Apply Resolution", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            left + comboWidth + 14, top + 18, buttonWidth, 24, hwnd, reinterpret_cast<HMENU>(static_cast<int>(OptionsControlId::ApplyResolutionButton)), instance_, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Sound On", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            left + comboWidth + 14, top + 48, 90, 24, hwnd, reinterpret_cast<HMENU>(static_cast<int>(OptionsControlId::SoundCheck)), instance_, nullptr);
        CreateWindowExW(0, L"BUTTON", L"VSync On", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            left + comboWidth + 14, top + 76, 90, 24, hwnd, reinterpret_cast<HMENU>(static_cast<int>(OptionsControlId::VSyncCheck)), instance_, nullptr);
        CreateWindowExW(0, L"BUTTON", L"Triple Buffering", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
            left + comboWidth + 14, top + 104, 126, 24, hwnd, reinterpret_cast<HMENU>(static_cast<int>(OptionsControlId::TripleBufferCheck)), instance_, nullptr);

        const struct SliderRow
        {
            const wchar_t* label;
            OptionsControlId sliderId;
            OptionsControlId valueId;
            int y;
        } rows[] =
        {
            { L"Logo spawn rate", OptionsControlId::SpawnRateSlider, OptionsControlId::SpawnValue, 144 },
            { L"Logo gravity", OptionsControlId::GravitySlider, OptionsControlId::GravityValue, 194 },
            { L"Laser speed", OptionsControlId::LaserSlider, OptionsControlId::LaserValue, 244 },
            { L"Scene jiggle", OptionsControlId::ChaosSlider, OptionsControlId::ChaosValue, 294 },
        };

        for (const auto& row : rows)
        {
            CreateWindowExW(0, L"STATIC", row.label, WS_CHILD | WS_VISIBLE,
                left, row.y, 120, 20, hwnd, nullptr, instance_, nullptr);

            HWND track = CreateWindowExW(0, TRACKBAR_CLASSW, nullptr, WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
                trackLeft, row.y - 2, trackWidth, 28, hwnd, reinterpret_cast<HMENU>(static_cast<int>(row.sliderId)), instance_, nullptr);
            SendMessageW(track, TBM_SETRANGE, TRUE, MAKELPARAM(0, 100));

            CreateWindowExW(0, L"STATIC", L"", WS_CHILD | WS_VISIBLE,
                valueLeft, row.y, 60, 20, hwnd, reinterpret_cast<HMENU>(static_cast<int>(row.valueId)), instance_, nullptr);
        }

        CreateWindowExW(0, L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            382, 344, 86, 26, hwnd, reinterpret_cast<HMENU>(static_cast<int>(OptionsControlId::CloseButton)), instance_, nullptr);

        PopulateResolutionCombo(hwnd);
        SyncOptionsControls(hwnd);
    }

    void PopulateResolutionCombo(HWND hwnd)
    {
        const HWND combo = GetDlgItem(hwnd, static_cast<int>(OptionsControlId::ResolutionCombo));
        SendMessageW(combo, CB_RESETCONTENT, 0, 0);

        selectedResolutionIndex_ = -1;
        for (std::size_t i = 0; i < resolutionOptions_.size(); ++i)
        {
            const auto& option = resolutionOptions_[i];
            std::wostringstream stream;
            stream << option.width << L" x " << option.height;
            const LRESULT index = SendMessageW(combo, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(stream.str().c_str()));
            if (option.width == clientWidth_ && option.height == clientHeight_)
            {
                selectedResolutionIndex_ = static_cast<int>(index);
            }
        }

        if (selectedResolutionIndex_ < 0 && !resolutionOptions_.empty())
        {
            selectedResolutionIndex_ = 0;
        }

        if (selectedResolutionIndex_ >= 0)
        {
            SendMessageW(combo, CB_SETCURSEL, selectedResolutionIndex_, 0);
        }
    }

    void SyncOptionsControls(HWND hwnd)
    {
        const HWND soundCheck = GetDlgItem(hwnd, static_cast<int>(OptionsControlId::SoundCheck));
        SendMessageW(soundCheck, BM_SETCHECK, soundEnabled_ ? BST_CHECKED : BST_UNCHECKED, 0);
        SetWindowTextW(soundCheck, soundEnabled_ ? L"Sound On" : L"Sound Off");

        const HWND vsyncCheck = GetDlgItem(hwnd, static_cast<int>(OptionsControlId::VSyncCheck));
        SendMessageW(vsyncCheck, BM_SETCHECK, vsyncEnabled_ ? BST_CHECKED : BST_UNCHECKED, 0);
        SetWindowTextW(vsyncCheck, vsyncEnabled_ ? L"VSync On" : L"VSync Off");

        const HWND tripleBufferCheck = GetDlgItem(hwnd, static_cast<int>(OptionsControlId::TripleBufferCheck));
        SendMessageW(tripleBufferCheck, BM_SETCHECK, tripleBufferingEnabled_ ? BST_CHECKED : BST_UNCHECKED, 0);
        SetWindowTextW(tripleBufferCheck, tripleBufferingEnabled_ ? L"Triple Buffering On" : L"Triple Buffering Off");

        SetTrackbarPosition(hwnd, OptionsControlId::SpawnRateSlider, logoSpawnRateScale_, 0.35f, 2.5f);
        SetTrackbarPosition(hwnd, OptionsControlId::GravitySlider, logoGravityScale_, 0.50f, 2.50f);
        SetTrackbarPosition(hwnd, OptionsControlId::LaserSlider, laserVelocity_, 0.20f, 4.00f);
        SetTrackbarPosition(hwnd, OptionsControlId::ChaosSlider, sceneJiggleScale_, 0.50f, 2.00f);
        UpdateOptionsValueLabels(hwnd);
    }

    void HandleOptionsCommand(HWND hwnd, int controlId, int notificationCode)
    {
        if (controlId == static_cast<int>(OptionsControlId::ResolutionCombo) && notificationCode == CBN_SELCHANGE)
        {
            selectedResolutionIndex_ = static_cast<int>(SendMessageW(GetDlgItem(hwnd, controlId), CB_GETCURSEL, 0, 0));
            return;
        }

        if (controlId == static_cast<int>(OptionsControlId::SoundCheck) && notificationCode == BN_CLICKED)
        {
            soundEnabled_ = SendMessageW(GetDlgItem(hwnd, controlId), BM_GETCHECK, 0, 0) == BST_CHECKED;
            SetWindowTextW(GetDlgItem(hwnd, controlId), soundEnabled_ ? L"Sound On" : L"Sound Off");
            if (!soundEnabled_)
            {
                PlaySoundW(nullptr, nullptr, 0);
            }
            return;
        }

        if (controlId == static_cast<int>(OptionsControlId::VSyncCheck) && notificationCode == BN_CLICKED)
        {
            vsyncEnabled_ = SendMessageW(GetDlgItem(hwnd, controlId), BM_GETCHECK, 0, 0) == BST_CHECKED;
            SetWindowTextW(GetDlgItem(hwnd, controlId), vsyncEnabled_ ? L"VSync On" : L"VSync Off");
            ApplyPresentationSettings();
            return;
        }

        if (controlId == static_cast<int>(OptionsControlId::TripleBufferCheck) && notificationCode == BN_CLICKED)
        {
            tripleBufferingEnabled_ = SendMessageW(GetDlgItem(hwnd, controlId), BM_GETCHECK, 0, 0) == BST_CHECKED;
            SetWindowTextW(GetDlgItem(hwnd, controlId), tripleBufferingEnabled_ ? L"Triple Buffering On" : L"Triple Buffering Off");
            ApplyPresentationSettings();
            return;
        }

        if (controlId == static_cast<int>(OptionsControlId::ApplyResolutionButton) && notificationCode == BN_CLICKED)
        {
            ApplyResolutionSelection(hwnd);
            return;
        }

        if (controlId == static_cast<int>(OptionsControlId::CloseButton) && notificationCode == BN_CLICKED)
        {
            DestroyWindow(hwnd);
        }
    }

    void HandleOptionsScroll(HWND hwnd, HWND control)
    {
        if (control == nullptr)
        {
            return;
        }

        const int controlId = GetDlgCtrlID(control);
        if (controlId == static_cast<int>(OptionsControlId::SpawnRateSlider))
        {
            logoSpawnRateScale_ = TrackbarPositionToValue(control, 0.35f, 2.5f);
            nextLogoSpawnTimer_ = std::min(nextLogoSpawnTimer_, RandomRange(0.67f, 1.6f) * logoSpawnRateScale_);
        }
        else if (controlId == static_cast<int>(OptionsControlId::GravitySlider))
        {
            logoGravityScale_ = TrackbarPositionToValue(control, 0.50f, 2.5f);
        }
        else if (controlId == static_cast<int>(OptionsControlId::LaserSlider))
        {
            laserVelocity_ = TrackbarPositionToValue(control, 0.20f, 4.0f);
        }
        else if (controlId == static_cast<int>(OptionsControlId::ChaosSlider))
        {
            sceneJiggleScale_ = TrackbarPositionToValue(control, 0.50f, 2.0f);
        }

        UpdateOptionsValueLabels(hwnd);
    }

    void UpdateOptionsValueLabels(HWND hwnd)
    {
        SetWindowTextW(GetDlgItem(hwnd, static_cast<int>(OptionsControlId::SpawnValue)), FormatOptionFloat(logoSpawnRateScale_, 2).c_str());
        SetWindowTextW(GetDlgItem(hwnd, static_cast<int>(OptionsControlId::GravityValue)), FormatOptionFloat(logoGravityScale_, 2).c_str());
        SetWindowTextW(GetDlgItem(hwnd, static_cast<int>(OptionsControlId::LaserValue)), FormatOptionFloat(laserVelocity_, 2).c_str());
        SetWindowTextW(GetDlgItem(hwnd, static_cast<int>(OptionsControlId::ChaosValue)), FormatOptionFloat(sceneJiggleScale_, 2).c_str());
    }

    void ApplyResolutionSelection(HWND hwnd)
    {
        const HWND combo = GetDlgItem(hwnd, static_cast<int>(OptionsControlId::ResolutionCombo));
        const int index = static_cast<int>(SendMessageW(combo, CB_GETCURSEL, 0, 0));
        if (index < 0 || index >= static_cast<int>(resolutionOptions_.size()))
        {
            return;
        }

        const ResolutionOption option = resolutionOptions_[static_cast<std::size_t>(index)];
        if (fullscreen_)
        {
            ToggleFullscreen();
        }

        clientWidth_ = option.width;
        clientHeight_ = option.height;
        RECT desiredRect{ 0, 0, clientWidth_, clientHeight_ };
        AdjustWindowRect(&desiredRect, WS_OVERLAPPEDWINDOW, FALSE);
        RECT currentRect{};
        GetWindowRect(window_, &currentRect);
        SetWindowPos(
            window_,
            nullptr,
            currentRect.left,
            currentRect.top,
            desiredRect.right - desiredRect.left,
            desiredRect.bottom - desiredRect.top,
            SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOZORDER);
        RefreshResolutionOptions();
        PopulateResolutionCombo(hwnd);
        SyncOptionsControls(hwnd);
        ApplyPresentationSettings();
        UpdateWindow(window_);
    }

    static float TrackbarPositionToValue(HWND trackbar, float minimum, float maximum)
    {
        const int position = static_cast<int>(SendMessageW(trackbar, TBM_GETPOS, 0, 0));
        return minimum + (static_cast<float>(position) / 100.0f) * (maximum - minimum);
    }

    void SetTrackbarPosition(HWND hwnd, OptionsControlId controlId, float value, float minimum, float maximum)
    {
        const HWND trackbar = GetDlgItem(hwnd, static_cast<int>(controlId));
        const float normalized = std::clamp((value - minimum) / (maximum - minimum), 0.0f, 1.0f);
        const int position = static_cast<int>(std::round(normalized * 100.0f));
        SendMessageW(trackbar, TBM_SETPOS, TRUE, position);
    }

    static std::wstring FormatOptionFloat(float value, int decimals)
    {
        std::wostringstream stream;
        stream.precision(decimals);
        stream << std::fixed << value;
        return stream.str();
    }

    void MaybeTriggerProceduralBackground()
    {
        if (RandomChance(0.05f))
        {
            proceduralBackgroundTimer_ = 5.0f;
        }
    }

    bool RandomChance(float probability)
    {
        std::uniform_real_distribution<float> distribution(0.0f, 1.0f);
        return distribution(rng_) <= probability;
    }

    bool LoadShaderFunctions()
    {
        glCreateShader_ = reinterpret_cast<PFNGLCREATESHADERPROC>(wglGetProcAddress("glCreateShader"));
        glShaderSource_ = reinterpret_cast<PFNGLSHADERSOURCEPROC>(wglGetProcAddress("glShaderSource"));
        glCompileShader_ = reinterpret_cast<PFNGLCOMPILESHADERPROC>(wglGetProcAddress("glCompileShader"));
        glGetShaderiv_ = reinterpret_cast<PFNGLGETSHADERIVPROC>(wglGetProcAddress("glGetShaderiv"));
        glGetShaderInfoLog_ = reinterpret_cast<PFNGLGETSHADERINFOLOGPROC>(wglGetProcAddress("glGetShaderInfoLog"));
        glCreateProgram_ = reinterpret_cast<PFNGLCREATEPROGRAMPROC>(wglGetProcAddress("glCreateProgram"));
        glAttachShader_ = reinterpret_cast<PFNGLATTACHSHADERPROC>(wglGetProcAddress("glAttachShader"));
        glLinkProgram_ = reinterpret_cast<PFNGLLINKPROGRAMPROC>(wglGetProcAddress("glLinkProgram"));
        glGetProgramiv_ = reinterpret_cast<PFNGLGETPROGRAMIVPROC>(wglGetProcAddress("glGetProgramiv"));
        glGetProgramInfoLog_ = reinterpret_cast<PFNGLGETPROGRAMINFOLOGPROC>(wglGetProcAddress("glGetProgramInfoLog"));
        glUseProgram_ = reinterpret_cast<PFNGLUSEPROGRAMPROC>(wglGetProcAddress("glUseProgram"));
        glGetUniformLocation_ = reinterpret_cast<PFNGLGETUNIFORMLOCATIONPROC>(wglGetProcAddress("glGetUniformLocation"));
        glUniform1f_ = reinterpret_cast<PFNGLUNIFORM1FPROC>(wglGetProcAddress("glUniform1f"));
        glUniform2f_ = reinterpret_cast<PFNGLUNIFORM2FPROC>(wglGetProcAddress("glUniform2f"));
        glUniform3f_ = reinterpret_cast<PFNGLUNIFORM3FPROC>(wglGetProcAddress("glUniform3f"));
        glDeleteProgram_ = reinterpret_cast<PFNGLDELETEPROGRAMPROC>(wglGetProcAddress("glDeleteProgram"));
        glDeleteShader_ = reinterpret_cast<PFNGLDELETESHADERPROC>(wglGetProcAddress("glDeleteShader"));

        return glCreateShader_ != nullptr &&
            glShaderSource_ != nullptr &&
            glCompileShader_ != nullptr &&
            glGetShaderiv_ != nullptr &&
            glCreateProgram_ != nullptr &&
            glAttachShader_ != nullptr &&
            glLinkProgram_ != nullptr &&
            glGetProgramiv_ != nullptr &&
            glUseProgram_ != nullptr &&
            glGetUniformLocation_ != nullptr &&
            glUniform1f_ != nullptr &&
            glUniform2f_ != nullptr &&
            glUniform3f_ != nullptr;
    }

    bool CreateLaserShaderProgram()
    {
        if (glCreateShader_ == nullptr)
        {
            return false;
        }

        static const char* vertexShaderSource =
            "varying vec2 vTexCoord;"
            "void main() {"
            "  gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
            "  vTexCoord = gl_MultiTexCoord0.xy;"
            "}";

        static const char* fragmentShaderSource =
            "uniform float uTime;"
            "uniform float uStereo;"
            "uniform float uIntensity;"
            "varying vec2 vTexCoord;"
            "void main() {"
            "  float beam = max(0.0, 1.0 - abs(vTexCoord.y - 0.5) * 2.0);"
            "  beam = pow(beam, 2.7);"
            "  float flicker = 0.7 + 0.3 * sin((vTexCoord.x * 40.0) - (uTime * 22.0) + (uStereo * 1.7));"
            "  vec3 leftColor = vec3(0.10, 0.90, 1.00);"
            "  vec3 rightColor = vec3(1.00, 0.20, 0.85);"
            "  vec3 color = mix(leftColor, rightColor, (uStereo + 1.0) * 0.5);"
            "  float alpha = beam * flicker * uIntensity;"
            "  gl_FragColor = vec4(color * (0.8 + flicker * 0.4), alpha);"
            "}";

        const GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
        const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
        if (vertexShader == 0 || fragmentShader == 0)
        {
            if (vertexShader != 0 && glDeleteShader_ != nullptr)
            {
                glDeleteShader_(vertexShader);
            }

            if (fragmentShader != 0 && glDeleteShader_ != nullptr)
            {
                glDeleteShader_(fragmentShader);
            }

            return false;
        }

        laserProgram_ = glCreateProgram_();
        glAttachShader_(laserProgram_, vertexShader);
        glAttachShader_(laserProgram_, fragmentShader);
        glLinkProgram_(laserProgram_);

        GLint linkStatus = 0;
        glGetProgramiv_(laserProgram_, GL_LINK_STATUS, &linkStatus);

        if (glDeleteShader_ != nullptr)
        {
            glDeleteShader_(vertexShader);
            glDeleteShader_(fragmentShader);
        }

        if (linkStatus == 0)
        {
            if (glDeleteProgram_ != nullptr)
            {
                glDeleteProgram_(laserProgram_);
            }

            laserProgram_ = 0;
            return false;
        }

        laserTimeLocation_ = glGetUniformLocation_(laserProgram_, "uTime");
        laserStereoLocation_ = glGetUniformLocation_(laserProgram_, "uStereo");
        laserIntensityLocation_ = glGetUniformLocation_(laserProgram_, "uIntensity");
        return true;
    }

    bool CreateDeformShaderProgram()
    {
        if (glCreateShader_ == nullptr)
        {
            return false;
        }

        static const char* vertexShaderSource =
            "uniform float uTime;"
            "uniform float uAmount;"
            "uniform vec2 uFreq;"
            "uniform float uTwist;"
            "uniform float uBulge;"
            "uniform float uShear;"
            "uniform vec2 uHalfSize;"
            "varying vec2 vTexCoord;"
            "void main() {"
            "  vec2 safeSize = max(uHalfSize, vec2(1.0, 1.0));"
            "  vec2 local = gl_Vertex.xy / safeSize;"
            "  float radius = length(local);"
            "  float angle = atan(local.y, local.x);"
            "  float twistedAngle = angle + uTwist * uAmount * (1.0 - clamp(radius, 0.0, 1.0));"
            "  vec2 twisted = vec2(cos(twistedAngle), sin(twistedAngle)) * radius;"
            "  vec2 warped = mix(local, twisted, min(abs(uTwist) * 0.35, 1.0));"
            "  warped.x += uShear * local.y * uAmount;"
            "  float bulge = 1.0 + uBulge * uAmount * (1.0 - clamp(radius, 0.0, 1.0));"
            "  warped *= bulge;"
            "  vec2 wave;"
            "  wave.x = sin((local.y * uFreq.y) + (uTime * 4.0)) * uAmount * safeSize.x * 0.18;"
            "  wave.y = cos((local.x * uFreq.x) + (uTime * 3.3)) * uAmount * safeSize.y * 0.18;"
            "  vec2 finalPos = vec2(warped.x * safeSize.x, warped.y * safeSize.y) + wave;"
            "  gl_Position = gl_ModelViewProjectionMatrix * vec4(finalPos, gl_Vertex.z, 1.0);"
            "  vTexCoord = gl_MultiTexCoord0.xy;"
            "}";

        static const char* fragmentShaderSource =
            "uniform float uAmount;"
            "uniform float uTime;"
            "varying vec2 vTexCoord;"
            "void main() {"
            "  vec4 color = texture2D(gl_Texture[0], vTexCoord);"
            "  float flash = 1.0 + sin((vTexCoord.x + vTexCoord.y + uTime) * 8.0) * uAmount * 0.08;"
            "  gl_FragColor = vec4(color.rgb * flash, color.a);"
            "}";

        const GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vertexShaderSource);
        const GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);
        if (vertexShader == 0 || fragmentShader == 0)
        {
            if (vertexShader != 0 && glDeleteShader_ != nullptr)
            {
                glDeleteShader_(vertexShader);
            }

            if (fragmentShader != 0 && glDeleteShader_ != nullptr)
            {
                glDeleteShader_(fragmentShader);
            }

            return false;
        }

        deformProgram_ = glCreateProgram_();
        glAttachShader_(deformProgram_, vertexShader);
        glAttachShader_(deformProgram_, fragmentShader);
        glLinkProgram_(deformProgram_);

        GLint linkStatus = 0;
        glGetProgramiv_(deformProgram_, GL_LINK_STATUS, &linkStatus);

        if (glDeleteShader_ != nullptr)
        {
            glDeleteShader_(vertexShader);
            glDeleteShader_(fragmentShader);
        }

        if (linkStatus == 0)
        {
            if (glDeleteProgram_ != nullptr)
            {
                glDeleteProgram_(deformProgram_);
            }

            deformProgram_ = 0;
            return false;
        }

        deformTimeLocation_ = glGetUniformLocation_(deformProgram_, "uTime");
        deformAmountLocation_ = glGetUniformLocation_(deformProgram_, "uAmount");
        deformFreqLocation_ = glGetUniformLocation_(deformProgram_, "uFreq");
        deformTwistLocation_ = glGetUniformLocation_(deformProgram_, "uTwist");
        deformBulgeLocation_ = glGetUniformLocation_(deformProgram_, "uBulge");
        deformShearLocation_ = glGetUniformLocation_(deformProgram_, "uShear");
        deformHalfSizeLocation_ = glGetUniformLocation_(deformProgram_, "uHalfSize");
        return true;
    }

    GLuint CompileShader(GLenum shaderType, const char* source)
    {
        const GLuint shader = glCreateShader_(shaderType);
        if (shader == 0)
        {
            return 0;
        }

        glShaderSource_(shader, 1, &source, nullptr);
        glCompileShader_(shader);

        GLint compileStatus = 0;
        glGetShaderiv_(shader, GL_COMPILE_STATUS, &compileStatus);
        if (compileStatus == 0)
        {
            if (glDeleteShader_ != nullptr)
            {
                glDeleteShader_(shader);
            }

            return 0;
        }

        return shader;
    }

    void BuildFragments()
    {
        fragments_.clear();
        fragments_.reserve(fragmentRows_ * fragmentColumns_);

        std::uniform_real_distribution<float> velocityOffset(-45.0f, 45.0f);
        std::uniform_real_distribution<float> angularVelocity(-260.0f, 260.0f);

        for (int row = 0; row < fragmentRows_; ++row)
        {
            for (int column = 0; column < fragmentColumns_; ++column)
            {
                const float normalizedX = ((column + 0.5f) / static_cast<float>(fragmentColumns_)) - 0.5f;
                const float normalizedY = ((row + 0.5f) / static_cast<float>(fragmentRows_)) - 0.5f;
                const float magnitude = 210.0f + (std::abs(normalizedX) + std::abs(normalizedY)) * 220.0f;

                Fragment fragment;
                fragment.u0 = column / static_cast<float>(fragmentColumns_);
                fragment.v0 = row / static_cast<float>(fragmentRows_);
                fragment.u1 = (column + 1) / static_cast<float>(fragmentColumns_);
                fragment.v1 = (row + 1) / static_cast<float>(fragmentRows_);
                fragment.velocityX = normalizedX * magnitude * 3.0f + velocityOffset(rng_);
                fragment.velocityY = normalizedY * magnitude * 2.2f + velocityOffset(rng_) - 180.0f;
                fragment.velocityZ = angularVelocity(rng_) * 0.05f;
                fragment.rotationSpeed = angularVelocity(rng_);
                fragments_.push_back(fragment);
            }
        }
    }

    void ToggleFullscreen()
    {
        fullscreen_ = !fullscreen_;

        if (fullscreen_)
        {
            windowedStyle_ = GetWindowLongW(window_, GWL_STYLE);
            GetWindowRect(window_, &windowedRect_);

            MONITORINFO monitorInfo{ sizeof(monitorInfo) };
            GetMonitorInfoW(MonitorFromWindow(window_, MONITOR_DEFAULTTONEAREST), &monitorInfo);

            SetWindowLongW(window_, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowPos(
                window_,
                HWND_TOP,
                monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.top,
                monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
                monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
                SWP_FRAMECHANGED);
        }
        else
        {
            SetWindowLongW(window_, GWL_STYLE, windowedStyle_ | WS_VISIBLE);
            SetWindowPos(
                window_,
                HWND_NOTOPMOST,
                windowedRect_.left,
                windowedRect_.top,
                windowedRect_.right - windowedRect_.left,
                windowedRect_.bottom - windowedRect_.top,
                SWP_FRAMECHANGED);
        }

        PlaySystemSound(L"SystemDefault");
    }

    void UpdateWindowTitle() const
    {
        std::wostringstream title;
        title.precision(1);
        title << std::fixed << L"CC Click Game | FPS: " << fps_;
        if (gameOver_)
        {
            title << L" | GAME OVER";
        }
        else if (ouchTimer_ > 0.0f)
        {
            title << L" | OUCH";
        }
        else if (fullscreen_)
        {
            title << L" | Fullscreen";
        }

        SetWindowTextW(window_, title.str().c_str());
    }

    static std::string FormatFloat(float value)
    {
        std::ostringstream stream;
        stream.precision(1);
        stream << std::fixed << value;
        return stream.str();
    }

    void PlaySystemSound(const wchar_t* alias) const
    {
        if (!soundEnabled_)
        {
            return;
        }

        PlaySoundW(alias, nullptr, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
    }

    float RandomRange(float minimum, float maximum)
    {
        std::uniform_real_distribution<float> distribution(minimum, maximum);
        return distribution(rng_);
    }

    Color3 RandomGradientColor()
    {
        return Color3{
            RandomRange(0.04f, 0.55f),
            RandomRange(0.04f, 0.55f),
            RandomRange(0.08f, 0.65f)
        };
    }

    std::string RandomJoke()
    {
        static const std::array<const char*, 8> jokes{
            "The logo went airborne without a pilot license.",
            "Gravity called. It wants its globe back.",
            "Too much flying. Not enough landing.",
            "The globe achieved orbit, then forgot the return trip.",
            "Breaking news: local logo defeats basic aerodynamics.",
            "The spinning was fine. The flying paperwork was not.",
            "Mission failed successfully. Very successfully.",
            "Next time, maybe do fewer barrel rolls."
        };

        std::uniform_int_distribution<size_t> distribution(0, jokes.size() - 1);
        return jokes[distribution(rng_)];
    }

    const wchar_t* RandomSystemSoundAlias()
    {
        static const std::array<const wchar_t*, 7> sounds{
            L"SystemAsterisk",
            L"SystemExclamation",
            L"SystemQuestion",
            L"SystemDefault",
            L"SystemStart",
            L"SystemExit",
            L"SystemHand"
        };

        std::uniform_int_distribution<size_t> distribution(0, sounds.size() - 1);
        return sounds[distribution(rng_)];
    }

    void Cleanup()
    {
        CloseOptionsWindow();

        if (deformProgram_ != 0 && glDeleteProgram_ != nullptr)
        {
            if (glUseProgram_ != nullptr)
            {
                glUseProgram_(0);
            }

            glDeleteProgram_(deformProgram_);
            deformProgram_ = 0;
        }

        if (laserProgram_ != 0 && glDeleteProgram_ != nullptr)
        {
            if (glUseProgram_ != nullptr)
            {
                glUseProgram_(0);
            }

            glDeleteProgram_(laserProgram_);
            laserProgram_ = 0;
        }

        if (fontBase_ != 0 && renderingContext_ != nullptr)
        {
            glDeleteLists(fontBase_, 256);
            fontBase_ = 0;
        }

        if (logoTexture_.id != 0 && renderingContext_ != nullptr)
        {
            glDeleteTextures(1, &logoTexture_.id);
            logoTexture_.id = 0;
        }

        if (cartmanTexture_.id != 0 && renderingContext_ != nullptr)
        {
            glDeleteTextures(1, &cartmanTexture_.id);
            cartmanTexture_.id = 0;
        }

        if (drawnTexture_.id != 0 && renderingContext_ != nullptr)
        {
            glDeleteTextures(1, &drawnTexture_.id);
            drawnTexture_.id = 0;
        }

        if (proceduralBackgroundTexture_.id != 0 && renderingContext_ != nullptr)
        {
            glDeleteTextures(1, &proceduralBackgroundTexture_.id);
            proceduralBackgroundTexture_.id = 0;
        }

        if (renderingContext_ != nullptr)
        {
            wglMakeCurrent(nullptr, nullptr);
            wglDeleteContext(renderingContext_);
            renderingContext_ = nullptr;
        }

        if (deviceContext_ != nullptr && window_ != nullptr)
        {
            ReleaseDC(window_, deviceContext_);
            deviceContext_ = nullptr;
        }

        if (gdiPlusToken_ != 0)
        {
            Gdiplus::GdiplusShutdown(gdiPlusToken_);
            gdiPlusToken_ = 0;
        }
    }

    HINSTANCE instance_ = nullptr;
    HWND window_ = nullptr;
    HDC deviceContext_ = nullptr;
    HGLRC renderingContext_ = nullptr;
    ULONG_PTR gdiPlusToken_ = 0;

    Texture logoTexture_;
    Texture cartmanTexture_;
    Texture drawnTexture_;
    Texture proceduralBackgroundTexture_;
    std::wstring assetStatus_;

    bool running_ = false;
    bool fullscreen_ = false;
    bool exploding_ = false;
    bool gameOver_ = false;
    bool soundEnabled_ = true;
    bool vsyncEnabled_ = true;
    bool tripleBufferingEnabled_ = false;
    bool spinLeftHeld_ = false;
    bool spinRightHeld_ = false;
    bool moveUpHeld_ = false;
    bool moveDownHeld_ = false;
    bool spaceHeld_ = false;
    HWND optionsWindow_ = nullptr;

    int clientWidth_ = 1280;
    int clientHeight_ = 720;
    int clickCount_ = 0;
    int frameCounter_ = 0;

    DWORD windowedStyle_ = WS_OVERLAPPEDWINDOW;
    RECT windowedRect_{ 0, 0, 1280, 720 };

    GLuint fontBase_ = 0;
    float fps_ = 0.0f;
    float elapsedSeconds_ = 0.0f;
    float gameOverCloseTime_ = 0.0f;
    float scaleX_ = 1.0f;
    float scaleY_ = 1.0f;
    float targetScaleX_ = 1.0f;
    float targetScaleY_ = 1.0f;
    float rotation_ = 0.0f;
    float targetRotation_ = 0.0f;
    float spinAngle_ = 0.0f;
    float spinRate_ = 0.0f;
    float targetSpinRate_ = 0.0f;
    float jiggleStrength_ = 0.0f;
    float bounceOffsetX_ = 0.0f;
    float bounceOffsetY_ = 0.0f;
    float bounceVelocityX_ = 0.0f;
    float bounceVelocityY_ = 0.0f;
    float randomBounceTimer_ = 1.1f;
    float backgroundGradientTimer_ = 2.0f;
    float flyDistance_ = 0.0f;
    float maxFlyDistance_ = 2300.0f;
    float jokeTimer_ = 0.0f;
    float ouchTimer_ = 0.0f;
    float nextLogoSpawnTimer_ = 3.0f;
    float cartmanJiggleStrength_ = 0.0f;
    float cartmanDragOffsetX_ = 0.0f;
    float cartmanDragOffsetY_ = 0.0f;
    float nextCartmanDragSoundTime_ = 0.0f;
    float cartmanDisappearTimer_ = 7.0f;
    float cartmanReturnTimer_ = 0.0f;
    float nextGradientCircleTimer_ = 3.8f;
    float laserSoundTimer_ = 0.0f;
    float proceduralBackgroundTimer_ = 0.0f;
    float cartmanPulseTimer_ = 0.0f;
    float laserDirectionOffset_ = 0.0f;
    float laserVelocity_ = 1.0f;
    float logoSpawnRateScale_ = 1.0f;
    float logoGravityScale_ = 1.0f;
    float sceneJiggleScale_ = 1.0f;
    int playerLogoEatCount_ = 0;
    int targetLogoEatCount_ = 10;

    Color3 currentBackgroundTop_{ 0.18f, 0.12f, 0.30f };
    Color3 currentBackgroundBottom_{ 0.06f, 0.12f, 0.20f };
    Color3 targetBackgroundTop_{ 0.32f, 0.18f, 0.28f };
    Color3 targetBackgroundBottom_{ 0.08f, 0.18f, 0.24f };
    DeformState deformState_;
    CartmanEntity cartman_;
    std::string gameOverJoke_;
    std::string activeJoke_;
    std::vector<GradientCircle> gradientCircles_;
    bool spawnReverseGravity_ = false;
    bool draggingCartman_ = false;
    bool cartmanHasEntered_ = false;
    bool playerWon_ = false;
    bool laserStereoFlip_ = false;

    bool shaderEnabled_ = false;
    bool laserShaderEnabled_ = false;
    GLuint deformProgram_ = 0;
    GLuint laserProgram_ = 0;
    GLint deformTimeLocation_ = -1;
    GLint deformAmountLocation_ = -1;
    GLint deformFreqLocation_ = -1;
    GLint deformTwistLocation_ = -1;
    GLint deformBulgeLocation_ = -1;
    GLint deformShearLocation_ = -1;
    GLint deformHalfSizeLocation_ = -1;
    GLint laserTimeLocation_ = -1;
    GLint laserStereoLocation_ = -1;
    GLint laserIntensityLocation_ = -1;

    PFNGLCREATESHADERPROC glCreateShader_ = nullptr;
    PFNGLSHADERSOURCEPROC glShaderSource_ = nullptr;
    PFNGLCOMPILESHADERPROC glCompileShader_ = nullptr;
    PFNGLGETSHADERIVPROC glGetShaderiv_ = nullptr;
    PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog_ = nullptr;
    PFNGLCREATEPROGRAMPROC glCreateProgram_ = nullptr;
    PFNGLATTACHSHADERPROC glAttachShader_ = nullptr;
    PFNGLLINKPROGRAMPROC glLinkProgram_ = nullptr;
    PFNGLGETPROGRAMIVPROC glGetProgramiv_ = nullptr;
    PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog_ = nullptr;
    PFNGLUSEPROGRAMPROC glUseProgram_ = nullptr;
    PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation_ = nullptr;
    PFNGLUNIFORM1FPROC glUniform1f_ = nullptr;
    PFNGLUNIFORM2FPROC glUniform2f_ = nullptr;
    PFNGLUNIFORM3FPROC glUniform3f_ = nullptr;
    PFNGLDELETEPROGRAMPROC glDeleteProgram_ = nullptr;
    PFNGLDELETESHADERPROC glDeleteShader_ = nullptr;
    PFNWGLSWAPINTERVALEXTPROC swapIntervalProc_ = nullptr;

    static constexpr int fragmentRows_ = 4;
    static constexpr int fragmentColumns_ = 4;
    std::vector<Fragment> fragments_;
    std::vector<LogoOrb> spawnedLogos_;
    LogoNodeTree logoNodeTree_;
    std::vector<ResolutionOption> resolutionOptions_;
    int selectedResolutionIndex_ = -1;
    std::mt19937 rng_;
};

int main()
{
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    const HINSTANCE instance = GetModuleHandleW(nullptr);
    constexpr int nCmdShow = SW_SHOWDEFAULT;
    Game game(instance);
    if (!game.Initialize(nCmdShow))
    {
        MessageBoxW(nullptr, L"Failed to start the OpenGL game.", L"CC Click Game", MB_ICONERROR | MB_OK);
        return -1;
    }

    return game.Run();
}

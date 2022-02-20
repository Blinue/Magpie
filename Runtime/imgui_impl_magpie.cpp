#include "pch.h"
#include "App.h"
#include <imgui.h>
#include "imgui_impl_magpie.h"
#include "Renderer.h"
#include "FrameSourceBase.h"


struct ImGui_ImplMagpie_Data {
    INT64                       Time;
    INT64                       TicksPerSecond;

    ImGui_ImplMagpie_Data() { memset(this, 0, sizeof(*this)); }
};

// Backend data stored in io.BackendPlatformUserData to allow support for multiple Dear ImGui contexts
// It is STRONGLY preferred that you use docking branch with multi-viewports (== single Dear ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
// FIXME: multi-context support is not well tested and probably dysfunctional in this backend.
// FIXME: some shared resources (mouse cursor shape, gamepad) are mishandled when using multi-context.
static ImGui_ImplMagpie_Data* ImGui_ImplMagpie_GetBackendData() {
    return ImGui::GetCurrentContext() ? (ImGui_ImplMagpie_Data*)ImGui::GetIO().BackendPlatformUserData : NULL;
}

// Functions
bool ImGui_ImplMagpie_Init() {
    ImGuiIO& io = ImGui::GetIO();
    IM_ASSERT(io.BackendPlatformUserData == NULL && "Already initialized a platform backend!");

    INT64 perf_frequency, perf_counter;
    if (!::QueryPerformanceFrequency((LARGE_INTEGER*)&perf_frequency))
        return false;
    if (!::QueryPerformanceCounter((LARGE_INTEGER*)&perf_counter))
        return false;

    // Setup backend capabilities flags
    ImGui_ImplMagpie_Data* bd = IM_NEW(ImGui_ImplMagpie_Data)();
    io.BackendPlatformUserData = (void*)bd;
    io.BackendPlatformName = "imgui_impl_magpie";

    bd->TicksPerSecond = perf_frequency;
    bd->Time = perf_counter;

    io.ImeWindowHandle = App::Get().GetHwndHost();

    return true;
}

void ImGui_ImplMagpie_Shutdown() {
    ImGui_ImplMagpie_Data* bd = ImGui_ImplMagpie_GetBackendData();
    IM_ASSERT(bd != NULL && "No platform backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    io.BackendPlatformName = NULL;
    io.BackendPlatformUserData = NULL;
    IM_DELETE(bd);
}

static void ImGui_ImplMagpie_UpdateMousePos() {
    ImGui_ImplMagpie_Data* bd = ImGui_ImplMagpie_GetBackendData();
    ImGuiIO& io = ImGui::GetIO();

    const RECT& srcFrameRect = App::Get().GetFrameSource().GetSrcFrameRect();
    const RECT& outputRect = App::Get().GetRenderer().GetOutputRect();

    SIZE srcFrameSize = { srcFrameRect.right - srcFrameRect.left, srcFrameRect.bottom - srcFrameRect.top };
    SIZE outputSize = { outputRect.right - outputRect.left, outputRect.bottom - outputRect.top };

    POINT pos{};
    GetCursorPos(&pos);
    
    io.MousePos = ImVec2(
        std::roundf((pos.x - srcFrameRect.left) * outputSize.cx / (float)srcFrameSize.cx),
        std::roundf((pos.y - srcFrameRect.top) * outputSize.cy / (float)srcFrameSize.cy)
    );
}

void ImGui_ImplMagpie_NewFrame() {
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplMagpie_Data* bd = ImGui_ImplMagpie_GetBackendData();
    IM_ASSERT(bd != NULL && "Did you call ImGui_ImplMagpie_Init()?");

    // Setup display size (every frame to accommodate for window resizing)
    const RECT& hostRect = App::Get().GetHostWndRect();
    const RECT& outputRect = App::Get().GetRenderer().GetOutputRect();
    io.DisplaySize = ImVec2((float)(hostRect.right - outputRect.left), (float)(hostRect.bottom - outputRect.top));

    // Setup time step
    INT64 current_time = 0;
    ::QueryPerformanceCounter((LARGE_INTEGER*)&current_time);
    io.DeltaTime = (float)(current_time - bd->Time) / bd->TicksPerSecond;
    bd->Time = current_time;

    // Update OS mouse position
    ImGui_ImplMagpie_UpdateMousePos();
}

// Win32 message handler (process Win32 mouse/keyboard inputs, etc.)
// Call from your application's message handler.
// When implementing your own backend, you can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if Dear ImGui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application.
// Generally you may always pass all inputs to Dear ImGui, and hide them from your application based on those two flags.
// PS: In this Win32 handler, we use the capture API (GetCapture/SetCapture/ReleaseCapture) to be able to read mouse coordinates when dragging mouse outside of our window bounds.
// PS: We treat DBLCLK messages as regular mouse down messages, so this code will work on windows classes that have the CS_DBLCLKS flag set. Our own example app code doesn't set this flag.

IMGUI_IMPL_API LRESULT ImGui_ImplMagpie_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui::GetCurrentContext() == NULL)
        return 0;

    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplMagpie_Data* bd = ImGui_ImplMagpie_GetBackendData();

    switch (msg) {
    case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
    case WM_XBUTTONDOWN: case WM_XBUTTONDBLCLK:
    {
        int button = 0;
        if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONDBLCLK) { button = 0; }
        if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONDBLCLK) { button = 1; }
        if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONDBLCLK) { button = 2; }
        if (msg == WM_XBUTTONDOWN || msg == WM_XBUTTONDBLCLK) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == NULL)
            ::SetCapture(hwnd);
        io.MouseDown[button] = true;
        return 0;
    }
    case WM_LBUTTONUP:
    case WM_RBUTTONUP:
    case WM_MBUTTONUP:
    case WM_XBUTTONUP:
    {
        int button = 0;
        if (msg == WM_LBUTTONUP) { button = 0; }
        if (msg == WM_RBUTTONUP) { button = 1; }
        if (msg == WM_MBUTTONUP) { button = 2; }
        if (msg == WM_XBUTTONUP) { button = (GET_XBUTTON_WPARAM(wParam) == XBUTTON1) ? 3 : 4; }
        io.MouseDown[button] = false;
        if (!ImGui::IsAnyMouseDown() && ::GetCapture() == hwnd)
            ::ReleaseCapture();
        return 0;
    }
    case WM_MOUSEWHEEL:
        io.MouseWheel += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
        return 0;
    case WM_MOUSEHWHEEL:
        io.MouseWheelH += (float)GET_WHEEL_DELTA_WPARAM(wParam) / (float)WHEEL_DELTA;
        return 0;
    }
    return 0;
}

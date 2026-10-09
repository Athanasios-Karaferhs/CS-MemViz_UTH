#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <tchar.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>

// Data structure to hold allocation details
struct Allocation {
    size_t size = 0;
    std::string file;
    int line = 0;
};

using AllocMap = std::unordered_map<std::string, Allocation>;

// Each window owns its own data so refreshing one doesn't wipe the other
struct PtrSnapshot {
    std::string name, var_addr, target_addr, value;
    size_t size = 0;
    std::string file;
    int line = 0;
};

std::vector<PtrSnapshot> ptr_snapshots;

void ParsePtrLogFile(const std::string& filename, std::vector<PtrSnapshot>& out) {
    out.clear();
    std::ifstream file(filename);
    if (!file.is_open()) return;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string tag, size_str, line_str;
        PtrSnapshot s;

        std::getline(ss, tag, ',');
        if (tag != "PTR") continue;
        std::getline(ss, s.name, ',');
        std::getline(ss, s.var_addr, ',');
        std::getline(ss, s.target_addr, ',');
        std::getline(ss, s.value, ',');
        std::getline(ss, size_str, ',');
        std::getline(ss, s.file, ',');
        std::getline(ss, line_str, ',');

        try { s.size = std::stoull(size_str); s.line = std::stoi(line_str); }
        catch (...) { continue; }

        out.push_back(s);
    }
}
//holds one line of the file: label, address, size, file
struct NodeEntry {
    std::string label;
    std::string address;
    size_t size = 0;
    std::string file;
};

std::vector<NodeEntry> node_entries;

// DirectX 11 globals
static ID3D11Device*           g_pd3dDevice = nullptr;
static ID3D11DeviceContext*    g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*         g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Parse CSV log file into the given map
// Format: EVENT,ADDRESS,SIZE,FILE,LINE   (EVENT = ALLOC or FREE) (year 1)
void ParseLogFile(const std::string& filename, AllocMap& out) {
    out.clear();
    std::ifstream file(filename);
    if (!file.is_open())
        return;

    std::string line;
    while (std::getline(file, line)) {
        if (line.empty())
            continue;

        std::stringstream ss(line);
        std::string event_type, address, size_str, source_file, line_str;

        std::getline(ss, event_type, ',');
        std::getline(ss, address, ',');
        std::getline(ss, size_str, ',');
        std::getline(ss, source_file, ',');
        std::getline(ss, line_str, ',');

        if (event_type == "ALLOC") {
            try {
                Allocation alloc;
                alloc.size = std::stoull(size_str);
                alloc.file = source_file;
                alloc.line = std::stoi(line_str);
                out[address] = alloc;
            }
            catch (...) {
            }
        }
        else if (event_type == "FREE") {
            out.erase(address);
        }
    }
}
//(year 2)
void ParseNodeLogFile(const std::string& filename, std::vector<NodeEntry>& out) {
    out.clear();
    std::ifstream file(filename);
    if (!file.is_open())
        return;

    std::string line;
    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back(); // \r for windows carry
        if (line.empty()) continue;

        std::stringstream ss(line);
        NodeEntry e;
        std::string size_str;

        std::getline(ss, e.label, ',');
        std::getline(ss, e.address, ',');
        std::getline(ss, size_str, ',');
        std::getline(ss, e.file, ',');

        try { e.size = std::stoull(size_str); } //turns the size text into a number. It is wrapped in 
       // try/catch so a bad or header line is skipped (continue) instead of crashing the program
        catch (...) { continue; }

        out.push_back(e); // stores the finished entry
    }
}

int main(int, char**) {
    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"ImGui Example", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"CS MemViz Memory Leak Debugger  ", WS_OVERLAPPEDWINDOW, 100, 100, 1280, 800, nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr; // don't restore stale/off-screen window positions
    ImGui::StyleColorsDark();

    // Setup platform/renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    bool running = true;
    while (running) {
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE)) {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT) running = false;
        }
        if (!running) break;

        ImGuiIO& io = ImGui::GetIO();

        // Start one ImGui frame for the entire application
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();
        
        //WINDOW 1:1st year mode, heap status
         ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y));
        ImGui::Begin("Pointers | Memory allocation");
        ImGui::Text("Pointer snapshots");
        if (ImGui::Button("Refresh Memory Log (1st Year)")) {
            ParsePtrLogFile("memory_log_1st_year.csv", ptr_snapshots);
        }
        ImGui::Separator();

        if (ptr_snapshots.empty()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "No pointer snapshots yet.");
        }
        else {  
            for (int i = 0; i < (int)ptr_snapshots.size(); i++) {   //castin at ptr>snaphots.size() cause of warning with signed/unsigned comparison
                const PtrSnapshot& s = ptr_snapshots[i];
                ImGui::PushID(i);   //give each row a unique ID(not strictly necessary but good for buttons)
                ImGui::Text("%s  (at %s)  ->  %s  | value: %s | %zu bytes | %s : line %d",
                    s.name.c_str(), s.var_addr.c_str(), s.target_addr.c_str(),
                    s.value.c_str(), s.size, s.file.c_str(), s.line);
                ImGui::PopID();     //give each row a unique ID(same here)
            }
        }
        ImGui::End();


        //WINDOW 2: 2nd year mode, data structures
        ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x*0.5f, 0));
        ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y));
        ImGui::Begin("Data Structure | Live Connection Status");
        if (ImGui::Button("Refresh Memory Log (2nd Year)")) {
            ParseNodeLogFile("memory_log_2st_year.csv", node_entries);
        }

        ImGui::Separator();

        if (node_entries.empty()) {
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "No Nodes Tracked!");
        }
        else {
            ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Active Nodes: %d", (int)node_entries.size());
            ImGui::Separator();

            for (int i = 0; i < (int)node_entries.size(); i++) {
                const NodeEntry& n = node_entries[i];
                ImGui::PushID(i);
                ImGui::Text("%s | Address: %s | Size: %zu bytes | File: %s",
                    n.label.c_str(), n.address.c_str(), n.size, n.file.c_str());
                ImGui::PopID();
            }
        }

        ImGui::End();


        // Rendering
        ImGui::Render();
        const float clear_color_with_alpha[4] = { 0.1f, 0.1f, 0.1f, 1.0f };
        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        g_pSwapChain->Present(1, 0); // vsync
    }

    // Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

//directX 11 helpers
bool CreateDeviceD3D(HWND hWnd) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // try high-performance WARP software driver if hardware is not available
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK) return false;

    CreateRenderTarget();
    return true;
}

void CleanupDeviceD3D() {
    CleanupRenderTarget();
    if (g_pSwapChain)        { g_pSwapChain->Release();        g_pSwapChain = nullptr; }
    if (g_pd3dDeviceContext) { g_pd3dDeviceContext->Release(); g_pd3dDeviceContext = nullptr; }
    if (g_pd3dDevice)        { g_pd3dDevice->Release();        g_pd3dDevice = nullptr; }
}

void CreateRenderTarget() {
    ID3D11Texture2D* pBackBuffer = nullptr;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();
}

void CleanupRenderTarget() {
    if (g_mainRenderTargetView) { g_mainRenderTargetView->Release(); g_mainRenderTargetView = nullptr; }
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, (UINT)LOWORD(lParam), (UINT)HIWORD(lParam), DXGI_FORMAT_UNKNOWN, 0);
            CreateRenderTarget();
        }
        return 0;
    case WM_SYSCOMMAND:
        if ((wParam & 0xFFF0) == SC_KEYMENU) // disable ALT application menu
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}

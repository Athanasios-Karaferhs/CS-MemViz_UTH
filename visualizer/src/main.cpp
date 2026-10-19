#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include <d3d11.h>
#include <dxgi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <utility>

#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

//data structure to hold allocation information
struct Allocation {
    size_t size = 0;
    std::string file;
    int line = 0;
};

using AllocMap = std::unordered_map<std::string, Allocation>;

// Independent data for each monitor.
AllocMap node_allocs;

// Log loading state.
bool heapLogLoaded = false;
bool nodeLogLoaded = false;

bool heapLogLoadFailed = false;
bool nodeLogLoadFailed = false;

std::string heapLogStatus = "Waiting for first refresh...";
std::string nodeLogStatus = "Waiting for first refresh...";
// DIRECTX 11 GLOBALS
static ID3D11Device* g_pd3dDevice = nullptr;
static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
static IDXGISwapChain* g_pSwapChain = nullptr;
static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;

bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
void SetupCustomStyle();
bool ParseLogFile(const std::string& filename, AllocMap& out);
bool ParsePtrLogFile(const std::string& filename, std::vector<struct PtrSnapshot>& out);
size_t TotalAllocatedBytes(const AllocMap& allocations);

LRESULT WINAPI WndProc(
    HWND hWnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
);

//for year 1 - pointers
struct PtrSnapshot {
    std::string name, var_addr, target_addr, value;
    size_t size = 0;
    std::string file;
    int line = 0;
};

std::vector<PtrSnapshot> ptr_snapshots;

bool ParsePtrLogFile(const std::string& filename, std::vector<PtrSnapshot>& out)
{
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::vector<PtrSnapshot> parsed;
    std::string line;

    while (std::getline(file, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string tag, size_str, line_str;
        PtrSnapshot s;

        std::getline(ss, tag, ',');
        if (tag != "PTR")
            continue;

        std::getline(ss, s.name, ',');
        std::getline(ss, s.var_addr, ',');
        std::getline(ss, s.target_addr, ',');
        std::getline(ss, s.value, ',');
        std::getline(ss, size_str, ',');
        std::getline(ss, s.file, ',');
        std::getline(ss, line_str);

        try {
            s.size = std::stoull(size_str);
            s.line = std::stoi(line_str);
        }
        catch (...) { continue; }

        parsed.push_back(s);
    }

    out.swap(parsed);
    return true;
}

//for year 2
bool ParseLogFile(const std::string& filename, AllocMap& out)
{
    std::ifstream file(filename);

    if (!file.is_open()) {
        return false;
    }

    AllocMap parsed;

    std::string line;

    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        std::stringstream ss(line);
        std::string event_type;
        std::string address;
        std::string size_str;
        std::string source_file;
        std::string line_str;
        std::getline(ss, event_type, ',');
        std::getline(ss, address, ',');
        std::getline(ss, size_str, ',');
        std::getline(ss, source_file, ',');
        std::getline(ss, line_str);

        if (event_type == "ALLOC") {
            try {
                Allocation alloc;

                alloc.size = std::stoull(size_str);  //string to unsigned long long
                alloc.file = source_file;
                alloc.line = std::stoi(line_str);   //same but for ints
                parsed[address] = alloc;//track the current allocation by address.
            }
            catch (...) {
                //ignore malformed rows and CSV headers.
            }
        }
        else if (event_type == "FREE") {
            //remove the allocation when it is freed.
            // I have to add a way to reprisent how its freed
            parsed.erase(address);
        }
    }

    //replace the old data only after success
    out.swap(parsed);

    return true;
}

size_t TotalAllocatedBytes(const AllocMap& allocations)
{
    size_t total = 0;
    for (const auto& pair : allocations) {
        total += pair.second.size;
    }
    return total;
}

void SetupCustomStyle()
{
    ImGuiStyle& style = ImGui::GetStyle();

    style.WindowRounding = 10.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 6.0f;
    style.PopupRounding = 6.0f;
    style.ScrollbarRounding = 6.0f;
    style.GrabRounding = 6.0f;
    style.TabRounding = 5.0f;
    style.WindowPadding = ImVec2(15.0f, 15.0f);
    style.FramePadding = ImVec2(10.0f, 7.0f);
    style.ItemSpacing = ImVec2(10.0f, 9.0f);
    style.ItemInnerSpacing = ImVec2(8.0f, 6.0f);
    style.ScrollbarSize = 12.0f;
    style.WindowBorderSize = 1.0f;
    style.FrameBorderSize = 0.0f;
    ImVec4* colors = style.Colors;
    //backgrounds
    colors[ImGuiCol_WindowBg] =
        ImVec4(0.05f, 0.065f, 0.090f, 1.00f);

    colors[ImGuiCol_ChildBg] =
        ImVec4(0.040f, 0.050f, 0.070f, 1.00f);

    colors[ImGuiCol_PopupBg] =
        ImVec4(0.070f, 0.080f, 0.110f, 1.00f);


    colors[ImGuiCol_Border] =
        ImVec4(0.19f, 0.23f, 0.32f, 1.00f);

    colors[ImGuiCol_FrameBg] =
        ImVec4(0.10f, 0.12f, 0.17f, 1.00f);

    colors[ImGuiCol_FrameBgHovered] =
        ImVec4(0.15f, 0.19f, 0.27f, 1.00f);

    colors[ImGuiCol_FrameBgActive] =
        ImVec4(0.19f, 0.25f, 0.35f, 1.00f);


    colors[ImGuiCol_Button] =
        ImVec4(0.12f, 0.30f, 0.49f, 1.00f);

    colors[ImGuiCol_ButtonHovered] =
        ImVec4(0.17f, 0.40f, 0.63f, 1.00f);

    colors[ImGuiCol_ButtonActive] =
        ImVec4(0.10f, 0.24f, 0.40f, 1.00f);

    colors[ImGuiCol_Header] =
        ImVec4(0.12f, 0.28f, 0.45f, 1.00f);

    colors[ImGuiCol_HeaderHovered] =
        ImVec4(0.16f, 0.38f, 0.60f, 1.00f);

    colors[ImGuiCol_HeaderActive] =
        ImVec4(0.10f, 0.24f, 0.40f, 1.00f);

    colors[ImGuiCol_TitleBg] =
        ImVec4(0.035f, 0.055f, 0.095f, 1.00f);

    colors[ImGuiCol_TitleBgActive] =
        ImVec4(0.055f, 0.105f, 0.18f, 1.00f);

    colors[ImGuiCol_TitleBgCollapsed] =
        ImVec4(0.035f, 0.055f, 0.095f, 0.80f);

    colors[ImGuiCol_Text] =
        ImVec4(0.91f, 0.93f, 0.97f, 1.00f);

    colors[ImGuiCol_TextDisabled] =
        ImVec4(0.48f, 0.52f, 0.60f, 1.00f);

    colors[ImGuiCol_ScrollbarBg] =
        ImVec4(0.035f, 0.045f, 0.065f, 1.00f);

    colors[ImGuiCol_ScrollbarGrab] =
        ImVec4(0.20f, 0.25f, 0.34f, 1.00f);

    colors[ImGuiCol_ScrollbarGrabHovered] =
        ImVec4(0.30f, 0.38f, 0.50f, 1.00f);

    colors[ImGuiCol_ScrollbarGrabActive] =
        ImVec4(0.38f, 0.48f, 0.65f, 1.00f);

    colors[ImGuiCol_ResizeGrip] =
        ImVec4(0.12f, 0.30f, 0.49f, 0.50f);

    colors[ImGuiCol_ResizeGripHovered] =
        ImVec4(0.17f, 0.40f, 0.63f, 0.75f);

    colors[ImGuiCol_ResizeGripActive] =
        ImVec4(0.20f, 0.48f, 0.75f, 1.00f);
}

int main()
{
    /*describe a window class: structure size, style
    (CS_CLASSDC is a leftover from the ImGui example), the WndProc function that receives this window’s messages, two extra-bytes fields (0),
    the instance handle of your exe (GetModuleHandleW(nullptr)), icon, cursor, background brush, menu (all nullptr), the class name L"CS MemViz",
    and a small icon. The W suffix and the L prefix mean wide (UTF-16) strings.*/
    WNDCLASSEXW wc = {
        sizeof(WNDCLASSEXW),
        CS_CLASSDC,
        WndProc,
        0L,
        0L,
        GetModuleHandleW(nullptr),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        L"CS MemViz",

    };

    if (!RegisterClassExW(&wc)) {   //registers the class. It returns 0 on failure, so the code returns 1
        return 1;
    }

    HWND hwnd = CreateWindowW(
        wc.lpszClassName,
        L"CS MemViz | Memory Leak Debugger",
        WS_OVERLAPPEDWINDOW,
        100,
        100,
        1280,
        800,
        nullptr,
        nullptr,
        wc.hInstance,
        nullptr
    );

    if (hwnd == nullptr) {
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    //we initialize Direct3D11
    if (!CreateDeviceD3D(hwnd)) {
        CleanupDeviceD3D();
        DestroyWindow(hwnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);

        return 1;
    }

    ShowWindow(hwnd, SW_SHOWDEFAULT);
    UpdateWindow(hwnd);

    //initialize dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    //do not restore previous window positions.
    io.IniFilename = nullptr;

    ImGui::StyleColorsDark();
    SetupCustomStyle();

    if (!ImGui_ImplWin32_Init(hwnd)) {
        ImGui::DestroyContext();
        CleanupDeviceD3D();
        DestroyWindow(hwnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);

        return 1;
    }

    if (!ImGui_ImplDX11_Init(
        g_pd3dDevice,
        g_pd3dDeviceContext)) {

        ImGui_ImplWin32_Shutdown();
        ImGui::DestroyContext();
        CleanupDeviceD3D();
        DestroyWindow(hwnd);
        UnregisterClassW(wc.lpszClassName, wc.hInstance);

        return 1;
    }

    bool running = true;
    while (running) {
        // prosses a windows message
        MSG msg;

        while (PeekMessageW(
            &msg,
            nullptr,
            0U,
            0U,
            PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
            if (msg.message == WM_QUIT) {
                running = false;
            }
        }

        if (!running) {
            break;
        }

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        //start of window 1 
        ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(600.0f, 400.0f), ImGuiCond_FirstUseEver);

        ImGui::Begin("CS MemViz | Heap Monitor");

        ImGui::TextColored(ImVec4(0.30f, 0.75f, 1.00f, 1.00f), "1ST YEAR  /  POINTERS");
        ImGui::TextDisabled("Watch what each pointer holds, one snapshot at a time.");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Refresh Memory Log", ImVec2(190.0f, 35.0f))) {
            if (ParsePtrLogFile("memory_log_1st_year.csv", ptr_snapshots)) {
                heapLogLoaded = true;
                heapLogLoadFailed = false;
                heapLogStatus = "Log refreshed successfully.";
            }
            else {
                heapLogLoadFailed = true;
                heapLogStatus = "Cannot open memory_log_1st_year.csv";
            }
        }
      

        ImGui::SameLine();
        ImGui::TextDisabled("%d snapshot(s)", static_cast<int>(ptr_snapshots.size()));

        ImGui::TextColored(heapLogLoadFailed ? ImVec4(1.00f, 0.35f, 0.35f, 1.00f) : ImVec4(0.48f, 0.60f, 0.70f, 1.00f), "%s", heapLogStatus.c_str());

        ImGui::Spacing();

        ImGui::SameLine();
        ImGui::SetCursorPos(ImVec2(520.0f,150.0f));
        if (ImGui::Button("Clear"))
            ptr_snapshots.clear();

        ImGui::Spacing();

        ImGui::BeginChild("HeapAllocationList", ImVec2(0.0f, 0.0f), true);

        if (!heapLogLoaded) {
            ImGui::TextColored(ImVec4(1.00f, 0.75f, 0.25f, 1.00f), "WAITING FOR DATA");
            ImGui::TextWrapped("Click Refresh Memory Log to load the first-year memory log.");
        }
        else if (ptr_snapshots.empty()) {
            ImGui::TextColored(ImVec4(0.20f, 1.00f, 0.45f, 1.00f), "NO POINTER SNAPSHOTS");
            ImGui::TextWrapped("The log is loaded but contains no pointer snapshots yet.");
        }
        else {
            for (size_t i = 0; i < ptr_snapshots.size(); ++i) {         //size_t cause .size() returns a type of size_t, preventing overflow/underflow as we maximize the pos
                const PtrSnapshot& s = ptr_snapshots[i];
                ImGui::PushID(static_cast<int>(i));

                ImGui::TextColored(ImVec4(0.30f, 0.75f, 1.00f, 1.00f), "%s", s.name.c_str());
                ImGui::SameLine();
                ImGui::TextDisabled("at %s", s.var_addr.c_str());

                if (s.target_addr == "NULL") {
                    ImGui::TextColored(ImVec4(1.00f, 0.35f, 0.35f, 1.00f), "-> NULL");
                }
                else {
                    ImGui::TextColored(ImVec4(1.00f, 0.60f, 0.20f, 1.00f), "-> %s", s.target_addr.c_str());
                    ImGui::SameLine();
                    ImGui::TextColored(ImVec4(0.20f, 1.00f, 0.45f, 1.00f), "value: %s", s.value.c_str());
                }

                ImGui::TextWrapped("Source: %s : Line %d", s.file.c_str(), s.line);
                ImGui::Separator();
                ImGui::PopID();
            }
       
        
        }

        ImGui::EndChild();
        ImGui::End();



        //start of window 2 data structurs
        ImGui::SetNextWindowPos(ImVec2(640.0f, 20.0f), ImGuiCond_FirstUseEver);

        ImGui::SetNextWindowSize(ImVec2(600.0f, 400.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("CS MemViz | Data Structures");

        ImGui::TextColored(ImVec4(1.00f, 0.76f, 0.25f, 1.00f), "2ND YEAR  /  DATA STRUCTURES");

        ImGui::TextDisabled("Monitor nodes and inspect their allocation details.");
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Refresh Node Log", ImVec2(190.0f, 35.0f))) {

            if (ParseLogFile("memory_log_2nd_year.csv", node_allocs)) {
                nodeLogLoaded = true;
                nodeLogLoadFailed = false;
                nodeLogStatus = "Log refreshed successfully.";
            }
            else {
                nodeLogLoadFailed = true;
                nodeLogStatus = "Cannot open memory_log_2nd_year.csv";
            }
        }

        ImGui::SameLine();
        ImGui::TextDisabled(
            "%d active node(s)",
            static_cast<int>(node_allocs.size())
        );

        ImGui::TextColored(
            nodeLogLoadFailed
            ? ImVec4(1.00f, 0.35f, 0.35f, 1.00f)
            : ImVec4(0.48f, 0.60f, 0.70f, 1.00f),
            "%s",
            nodeLogStatus.c_str()
        );

        ImGui::Spacing();

        ImGui::SameLine();
        ImGui::SetCursorPos(ImVec2(520.0f, 150.0f));
        if (ImGui::Button("Clear"))
            node_allocs.clear();

        ImGui::BeginChild("NodeAllocationList", ImVec2(0.0f, 0.0f), true);

        if (!nodeLogLoaded) {
            ImGui::TextColored(ImVec4(1.00f, 0.75f, 0.25f, 1.00f), "WAITING FOR DATA"
            );
            ImGui::TextWrapped("Click Refresh Node Log to load the ""second-year memory log."
            );
        }
        else if (node_allocs.empty()) {

            ImGui::TextColored(ImVec4(0.20f, 1.00f, 0.45f, 1.00f), "NO ACTIVE NODES");

            ImGui::TextWrapped("The loaded log contains no outstanding nodes.");
        }
        else {

            ImGui::TextColored(ImVec4(1.00f, 0.76f, 0.25f, 1.00f), "ACTIVE NODES");

            ImGui::Text("Tracked bytes: %zu", TotalAllocatedBytes(node_allocs));

            ImGui::Separator();

            int nodeNumber = 1;

            for (const auto& pair : node_allocs) {

                ImGui::PushID((pair.first + "_node").c_str());
                ImGui::TextColored(ImVec4(0.89f, 1.69f, 0.59f, 1.00f), "NODE %d", nodeNumber);
                ImGui::SameLine();
                ImGui::TextWrapped("%s", pair.first.c_str());
                ImGui::Text("Size: %zu bytes", pair.second.size);
                ImGui::TextWrapped("Source: %s : Line %d", pair.second.file.c_str(), pair.second.line);
                ImGui::Separator();
                ImGui::PopID();

                ++nodeNumber;
            }
        }
        ImGui::EndChild();
        ImGui::End();


        //data struct visualizer
        ImGui::SetNextWindowPos(ImVec2(20.0f, 440.0f), ImGuiCond_FirstUseEver);
        ImGui::SetNextWindowSize(ImVec2(1220.0f, 300.0f), ImGuiCond_FirstUseEver);
        ImGui::Begin("CS MemViz | Data Structure Visualizer");
        ImGui::TextColored(ImVec4(0.30f, 0.85f, 1.00f, 1.00f), "NODE VISUALIZATION");
        ImGui::SameLine();
        ImGui::TextDisabled("| %d tracked node(s)", static_cast<int>(node_allocs.size()));
        ImGui::Separator();
        ImGui::TextDisabled("Connections show visual order only; actual pointers ""are not available in the current CSV format.");
        ImGui::Spacing();
        ImGui::BeginChild("VisualizerCanvas", ImVec2(0.0f, 0.0f), true, ImGuiWindowFlags_HorizontalScrollbar);
        if (!nodeLogLoaded)
            ImGui::TextColored(ImVec4(1.00f, 0.75f, 0.25f, 1.00f), "Load the node log to display the visualization.");
        else if (node_allocs.empty()) {
            ImGui::TextColored(ImVec4(0.45f, 0.80f, 0.60f, 1.00f), "NULL");
            ImGui::TextDisabled("There are currently no outstanding nodes.");
        }
        else {

            //sort addresses for a stable
            //this order does not represent pointer relationships.
            //visualNodes is a vector of pointers to entries
            std::vector<const AllocMap::value_type*> visualNodes;

            visualNodes.reserve(node_allocs.size());

            for (const auto& pair : node_allocs) {
                visualNodes.push_back(&pair);
            }

            std::sort(  //sort the vector of pointers based on the address (key) of each entry
                visualNodes.begin(),
                visualNodes.end(),
                [](const AllocMap::value_type* a,
                    const AllocMap::value_type* b) {
                        return a->first < b->first; //returns true if the address of a is less than the address of b
                }
            );

            ImDrawList* drawList = ImGui::GetWindowDrawList();

            const float nodeWidth = 180.0f;
            const float nodeHeight = 100.0f;
            const float spacing = 65.0f;
            const float topPadding = 15.0f;
            const float leftPadding = 15.0f;

            ImVec2 canvasStart = ImGui::GetCursorScreenPos();

            float startX = canvasStart.x + leftPadding;
            float startY = canvasStart.y + topPadding;

            for (size_t i = 0; i < visualNodes.size(); ++i) {

                const auto& pair = *visualNodes[i];

                float x = startX +static_cast<float>(i) * (nodeWidth + spacing);

                float y = startY;

                ImVec2 nodeMin(x, y);

                ImVec2 nodeMax(x + nodeWidth,y + nodeHeight
                );

                // Node background.
                drawList->AddRectFilled(nodeMin, nodeMax, IM_COL32(30, 40, 62, 255), 9.0f);

                // Header separator.
                drawList->AddLine(ImVec2(x + 10.0f, y + 32.0f), ImVec2(x + nodeWidth - 10.0f, y + 32.0f), IM_COL32(75, 90, 125, 255), 1.0f);

                // Node title.
                std::string nodeTitle = "NODE " + std::to_string(i + 1);

                drawList->AddText(ImVec2(x + 12.0f, y + 10.0f), IM_COL32(150, 210, 255, 255), nodeTitle.c_str());

                // Address.
                drawList->AddText(ImVec2(x + 12.0f, y + 43.0f), IM_COL32(225, 230, 240, 255), pair.first.c_str());

                // Allocation size.
                std::string sizeText = std::to_string(pair.second.size) + " bytes";

                drawList->AddText(ImVec2(x + 12.0f, y + 68.0f), IM_COL32(120, 225, 175, 255), sizeText.c_str());

                // Draw an illustrative connection to the next node.
                if (i + 1 < visualNodes.size()) {   //draws for each i+1 node
                    float lineY = y + nodeHeight / 2.0f;ImVec2 lineStart(nodeMax.x, lineY);

                    ImVec2 lineEnd(nodeMax.x + spacing, lineY);

                    drawList->AddLine(lineStart, lineEnd, IM_COL32(70, 180, 255, 255), 3.0f);

                    // Arrow head.
                    drawList->AddTriangleFilled(ImVec2(lineEnd.x, lineEnd.y), ImVec2(lineEnd.x - 10.0f, lineEnd.y - 6.0f), ImVec2(lineEnd.x - 10.0f, lineEnd.y + 6.0f), IM_COL32(70, 180, 255, 255));
                }
            }

            // Reserve content area and enable horizontal scrolling.
            float contentWidth = leftPadding * 2.0f;

            if (!visualNodes.empty()) {
                contentWidth += static_cast<float>(visualNodes.size()) * (nodeWidth + spacing) - spacing;
            }

            ImGui::Dummy(ImVec2(contentWidth, nodeHeight + topPadding * 2.0f)
            );
        }

        ImGui::EndChild();

        ImGui::End();

        ImGui::Render();
        const float clearColor[4] = {
            0.055f,
            0.065f,
            0.090f,
            1.00f
        };

        g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, nullptr);

        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clearColor);

        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

        // Present with vertical synchronization.
        g_pSwapChain->Present(1, 0);
    }

    //cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();

    ImGui::DestroyContext();

    CleanupDeviceD3D();

    DestroyWindow(hwnd);

    UnregisterClassW(
        wc.lpszClassName,
        wc.hInstance
    );

    return 0;
}

//device and swap change
bool CreateDeviceD3D(HWND hWnd)
{
    DXGI_SWAP_CHAIN_DESC sd = {};

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
    const D3D_FEATURE_LEVEL featureLevelArray[2] = {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_0
    };
    HRESULT res = D3D11CreateDeviceAndSwapChain(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        createDeviceFlags,
        featureLevelArray,
        2,
        D3D11_SDK_VERSION,
        &sd,
        &g_pSwapChain,
        &g_pd3dDevice,
        &featureLevel,
        &g_pd3dDeviceContext
    );

    // Retry with the software renderer if hardware is unsupported.
    if (res == DXGI_ERROR_UNSUPPORTED) {

        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    }

    if (FAILED(res)) {
        return false;
    }

    CreateRenderTarget();

    if (g_mainRenderTargetView == nullptr) {
        CleanupDeviceD3D();
        return false;
    }

    return true;
}

//dir  cleanup dev
void CleanupDeviceD3D()
{
    CleanupRenderTarget();

    if (g_pSwapChain) {
        g_pSwapChain->Release();
        g_pSwapChain = nullptr;
    }

    if (g_pd3dDeviceContext) {
        g_pd3dDeviceContext->Release();
        g_pd3dDeviceContext = nullptr;
    }

    if (g_pd3dDevice) {
        g_pd3dDevice->Release();
        g_pd3dDevice = nullptr;
    }
}

//dir  create render target
void CreateRenderTarget()
{
    if (g_pSwapChain == nullptr || g_pd3dDevice == nullptr) {
        return;
    }

    ID3D11Texture2D* pBackBuffer = nullptr;

    HRESULT hr = g_pSwapChain->GetBuffer(
        0,
        IID_PPV_ARGS(&pBackBuffer)
    );

    if (FAILED(hr) || pBackBuffer == nullptr) {
        return;
    }

    hr = g_pd3dDevice->CreateRenderTargetView(
        pBackBuffer,
        nullptr,
        &g_mainRenderTargetView
    );

    pBackBuffer->Release();

    if (FAILED(hr)) {
        g_mainRenderTargetView = nullptr;
    }
}

// same here but with the func above but i clean up
void CleanupRenderTarget()
{
    if (g_mainRenderTargetView) {
        g_mainRenderTargetView->Release();
        g_mainRenderTargetView = nullptr;
    }
}

//msg handler
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg) {
    case WM_SIZE:
        if (g_pd3dDevice != nullptr && wParam != SIZE_MINIMIZED) {
            UINT width = static_cast<UINT>(LOWORD(lParam));
            UINT height = static_cast<UINT>(HIWORD(lParam));

            if (width > 0 && height > 0) {
                CleanupRenderTarget();
                HRESULT hr = g_pSwapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0);
                if (SUCCEEDED(hr))
                    CreateRenderTarget();
            }
        }

        return 0;

    case WM_SYSCOMMAND:
        // disable the alt application menu.
        if ((wParam & 0xFFF0) == SC_KEYMENU)
            return 0;
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(hWnd, msg, wParam, lParam);
}

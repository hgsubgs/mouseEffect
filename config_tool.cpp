#include <windows.h>
#include <commctrl.h>
#include <fstream>
#include <string>

// 配置参数
int maxTrailLength = 20;
int blurRadius = 15;
int maxAlpha = 200;
int minimizeToTray = 1;
int timerInterval = 50;
int autoCleanTime = 500;

HWND hwndTrackbar1, hwndTrackbar2, hwndTrackbar3, hwndTrackbar4, hwndTrackbar5;
HWND hwndLabel1, hwndLabel2, hwndLabel3, hwndLabel4, hwndLabel5;
HWND hwndApplyButton, hwndSaveButton;

// 函数声明
void LoadConfig();
void SaveConfig();
void UpdateLabels();

// 加载配置文件
void LoadConfig() {
    std::ifstream file("config.h");
    std::string line;
    
    if (!file.is_open()) return;
    
    while (std::getline(file, line)) {
        if (line.find("#define MAX_TRAIL_LENGTH") != std::string::npos) {
            sscanf_s(line.c_str(), "#define MAX_TRAIL_LENGTH %d", &maxTrailLength);
        } else if (line.find("#define BLUR_RADIUS") != std::string::npos) {
            sscanf_s(line.c_str(), "#define BLUR_RADIUS %d", &blurRadius);
        } else if (line.find("#define MAX_ALPHA") != std::string::npos) {
            sscanf_s(line.c_str(), "#define MAX_ALPHA %d", &maxAlpha);
        } else if (line.find("#define MINIMIZE_TO_TRAY") != std::string::npos) {
            sscanf_s(line.c_str(), "#define MINIMIZE_TO_TRAY %d", &minimizeToTray);
        } else if (line.find("#define TIMER_INTERVAL") != std::string::npos) {
            sscanf_s(line.c_str(), "#define TIMER_INTERVAL %d", &timerInterval);
        } else if (line.find("#define AUTO_CLEAN_TIME") != std::string::npos) {
            sscanf_s(line.c_str(), "#define AUTO_CLEAN_TIME %d", &autoCleanTime);
        }
    }
    
    file.close();
}

// 保存配置文件
void SaveConfig() {
    std::ofstream file("config.h");
    
    if (!file.is_open()) return;
    
    file << "// 鼠标模糊效果配置文件\n";
    file << "// 请根据需要调整以下参数\n\n";
    file << "// 轨迹点数量 (越大轨迹越长)\n";
    file << "#define MAX_TRAIL_LENGTH " << maxTrailLength << "\n\n";
    file << "// 模糊半径 (越大单个点越模糊)\n";
    file << "#define BLUR_RADIUS " << blurRadius << "\n\n";
    file << "// 最大透明度 (0-255, 越大越不透明)\n";
    file << "#define MAX_ALPHA " << maxAlpha << "\n\n";
    file << "// 最小化到系统托盘 (1=启用, 0=禁用)\n";
    file << "#define MINIMIZE_TO_TRAY " << minimizeToTray << "\n\n";
    file << "// 定时器间隔 (毫秒, 越小越流畅但消耗更多CPU)\n";
    file << "#define TIMER_INTERVAL " << timerInterval << "\n\n";
    file << "// 自动清理过期点的时间 (毫秒, 越小轨迹消失越快)\n";
    file << "#define AUTO_CLEAN_TIME " << autoCleanTime << "\n";
    
    file.close();
}

// 更新标签显示
void UpdateLabels() {
    wchar_t buffer[256];
    
    // 更新轨迹长度标签
    swprintf_s(buffer, L"轨迹点数量: %d", maxTrailLength);
    SetWindowTextW(hwndLabel1, buffer);
    
    // 更新模糊半径标签
    swprintf_s(buffer, L"模糊半径: %d", blurRadius);
    SetWindowTextW(hwndLabel2, buffer);
    
    // 更新透明度标签
    swprintf_s(buffer, L"最大透明度: %d", maxAlpha);
    SetWindowTextW(hwndLabel3, buffer);
    
    // 更新定时器间隔标签
    swprintf_s(buffer, L"定时器间隔: %d ms", timerInterval);
    SetWindowTextW(hwndLabel4, buffer);
    
    // 更新自动清理时间标签
    swprintf_s(buffer, L"自动清理时间: %d ms", autoCleanTime);
    SetWindowTextW(hwndLabel5, buffer);
    
    // 更新滑块位置
    SendMessage(hwndTrackbar1, TBM_SETPOS, TRUE, maxTrailLength);
    SendMessage(hwndTrackbar2, TBM_SETPOS, TRUE, blurRadius);
    SendMessage(hwndTrackbar3, TBM_SETPOS, TRUE, maxAlpha);
    SendMessage(hwndTrackbar4, TBM_SETPOS, TRUE, timerInterval);
    SendMessage(hwndTrackbar5, TBM_SETPOS, TRUE, autoCleanTime);
}

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // 加载当前配置
            LoadConfig();
            
            // 创建轨迹长度滑块
            hwndTrackbar1 = CreateWindowExW(
                0, TRACKBAR_CLASSW, NULL,
                WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
                20, 20, 300, 40,
                hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            SendMessage(hwndTrackbar1, TBM_SETRANGE, TRUE, MAKELONG(1, 50));
            
            // 创建轨迹长度标签
            hwndLabel1 = CreateWindowExW(
                0, L"STATIC", L"轨迹点数量: 20",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 60, 300, 20,
                hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            
            // 创建模糊半径滑块
            hwndTrackbar2 = CreateWindowExW(
                0, TRACKBAR_CLASSW, NULL,
                WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
                20, 90, 300, 40,
                hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            SendMessage(hwndTrackbar2, TBM_SETRANGE, TRUE, MAKELONG(1, 50));
            
            // 创建模糊半径标签
            hwndLabel2 = CreateWindowExW(
                0, L"STATIC", L"模糊半径: 15",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 130, 300, 20,
                hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            
            // 创建透明度滑块
            hwndTrackbar3 = CreateWindowExW(
                0, TRACKBAR_CLASSW, NULL,
                WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
                20, 160, 300, 40,
                hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            SendMessage(hwndTrackbar3, TBM_SETRANGE, TRUE, MAKELONG(10, 255));
            
            // 创建透明度标签
            hwndLabel3 = CreateWindowExW(
                0, L"STATIC", L"最大透明度: 200",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 200, 300, 20,
                hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            
            // 创建定时器间隔滑块
            hwndTrackbar4 = CreateWindowExW(
                0, TRACKBAR_CLASSW, NULL,
                WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
                20, 230, 300, 40,
                hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            SendMessage(hwndTrackbar4, TBM_SETRANGE, TRUE, MAKELONG(10, 200));
            
            // 创建定时器间隔标签
            hwndLabel4 = CreateWindowExW(
                0, L"STATIC", L"定时器间隔: 50 ms",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 270, 300, 20,
                hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            
            // 创建自动清理时间滑块
            hwndTrackbar5 = CreateWindowExW(
                0, TRACKBAR_CLASSW, NULL,
                WS_CHILD | WS_VISIBLE | TBS_AUTOTICKS | TBS_HORZ,
                20, 300, 300, 40,
                hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            SendMessage(hwndTrackbar5, TBM_SETRANGE, TRUE, MAKELONG(100, 1000));
            
            // 创建自动清理时间标签
            hwndLabel5 = CreateWindowExW(
                0, L"STATIC", L"自动清理时间: 500 ms",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                20, 340, 300, 20,
                hwnd, NULL, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            
            // 创建应用按钮
            hwndApplyButton = CreateWindowExW(
                0, L"BUTTON", L"应用设置",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                50, 380, 100, 30,
                hwnd, (HMENU)101, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            
            // 创建保存按钮
            hwndSaveButton = CreateWindowExW(
                0, L"BUTTON", L"保存到配置",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                180, 380, 120, 30,
                hwnd, (HMENU)102, ((LPCREATESTRUCT)lParam)->hInstance, NULL
            );
            
            // 初始化滑块位置
            UpdateLabels();
            break;
        }
        case WM_HSCROLL: {
            // 处理滑块滚动
            if ((HWND)lParam == hwndTrackbar1) {
                maxTrailLength = SendMessage(hwndTrackbar1, TBM_GETPOS, 0, 0);
            } else if ((HWND)lParam == hwndTrackbar2) {
                blurRadius = SendMessage(hwndTrackbar2, TBM_GETPOS, 0, 0);
            } else if ((HWND)lParam == hwndTrackbar3) {
                maxAlpha = SendMessage(hwndTrackbar3, TBM_GETPOS, 0, 0);
            } else if ((HWND)lParam == hwndTrackbar4) {
                timerInterval = SendMessage(hwndTrackbar4, TBM_GETPOS, 0, 0);
            } else if ((HWND)lParam == hwndTrackbar5) {
                autoCleanTime = SendMessage(hwndTrackbar5, TBM_GETPOS, 0, 0);
            }
            
            UpdateLabels();
            break;
        }
        case WM_COMMAND: {
            int controlId = LOWORD(wParam);
            
            if (controlId == 101) { // 应用按钮
                // 应用设置 - 保存当前值到配置文件
                SaveConfig();
                MessageBoxW(hwnd, L"设置已应用！请重新启动鼠标模糊效果程序以生效。", L"提示", MB_OK | MB_ICONINFORMATION);
            } else if (controlId == 102) { // 保存按钮
                // 保存设置到配置文件
                SaveConfig();
                MessageBoxW(hwnd, L"配置已保存到 config.h 文件！", L"提示", MB_OK | MB_ICONINFORMATION);
            }
            break;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// WinMain函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 初始化通用控件
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC = ICC_BAR_CLASSES; // 包括滑块控件
    InitCommonControlsEx(&icc);
    
    // 注册窗口类
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"ConfigWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    
    RegisterClassExW(&wc);
    
    // 创建窗口
    HWND hwnd = CreateWindowExW(
        0, L"ConfigWindowClass", L"鼠标模糊效果配置",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 480,
        NULL, NULL, hInstance, NULL
    );
    
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    return (int)msg.wParam;
}
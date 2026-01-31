#include <windows.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include "config.h"

// 定义鼠标轨迹点结构
struct MousePoint {
    int x, y;
    DWORD timestamp;
    
    MousePoint(int x, int y) : x(x), y(y), timestamp(GetTickCount()) {}
};

// 全局变量
std::vector<MousePoint> mouseTrail;
HHOOK hMouseHook = NULL;
HWND g_hwnd = NULL;
bool g_bShowWindow = true;

// 为GDI+添加必要的引用
#include <gdiplus.h>
using namespace Gdiplus;

// 函数声明
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam);
void DrawMouseBlur(HDC hdc);
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
void CreateTrayIcon(HWND hwnd);
void DeleteTrayIcon(HWND hwnd);

// 全局鼠标钩子回调函数 - 只记录鼠标位置，不阻止事件
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0 && wParam == WM_MOUSEMOVE) {
        MOUSEHOOKSTRUCT *pMouseStruct = (MOUSEHOOKSTRUCT*)lParam;
        if (pMouseStruct != NULL) {
            // 添加鼠标位置到轨迹列表
            mouseTrail.push_back(MousePoint(pMouseStruct->pt.x, pMouseStruct->pt.y));
            
            // 限制轨迹点数量
            if (mouseTrail.size() > MAX_TRAIL_LENGTH) {
                mouseTrail.erase(mouseTrail.begin());
            }
            
            // 触发窗口重绘
            if (g_hwnd) {
                // 使用UpdateLayeredWindow更新全屏窗口
                InvalidateRect(g_hwnd, NULL, FALSE);
            }
        }
    }
    
    // 重要：调用下一个钩子，确保鼠标事件正常传递
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

// 定时器回调函数，用于清理过期的轨迹点
VOID CALLBACK TimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    DWORD currentTime = GetTickCount();
    // 移除超过AUTO_CLEAN_TIME的轨迹点
    mouseTrail.erase(
        std::remove_if(mouseTrail.begin(), mouseTrail.end(),
            [currentTime](const MousePoint& pt) {
                return (currentTime - pt.timestamp) > AUTO_CLEAN_TIME;
            }),
        mouseTrail.end()
    );
    
    if (g_hwnd) {
        InvalidateRect(g_hwnd, NULL, FALSE);
    }
}

// 使用GDI+绘制更平滑的模糊效果
void DrawMouseBlur(HDC hdc) {
    if (mouseTrail.empty()) return;
    
    // 获取屏幕尺寸
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // 创建一个内存DC用于双缓冲绘图
    HDC memDC = CreateCompatibleDC(hdc);
    HBITMAP hBitmap = CreateCompatibleBitmap(hdc, screenWidth, screenHeight);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(memDC, hBitmap);
    
    // 填充透明背景
    COLORREF transparentColor = RGB(0, 0, 0);  // 黑色作为透明色
    PatBlt(memDC, 0, 0, screenWidth, screenHeight, BLACKNESS);
    
    // 使用GDI+进行高质量绘图
    Gdiplus::Graphics graphics(memDC);
    graphics.SetCompositingMode(CompositingModeSourceOver);
    graphics.SetCompositingQuality(Gdiplus::CompositingQualityHighQuality);
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    
    // 绘制轨迹点，实现模糊效果
    for (size_t i = 0; i < mouseTrail.size(); ++i) {
        float alpha = static_cast<float>(i) / mouseTrail.size();
        int radius = static_cast<int>(BLUR_RADIUS * alpha);
        
        // 根据时间衰减计算透明度
        DWORD currentTime = GetTickCount();
        float timeFactor = 1.0f - ((float)(currentTime - mouseTrail[i].timestamp) / AUTO_CLEAN_TIME);
        timeFactor = timeFactor < 0 ? 0 : timeFactor;
        alpha *= timeFactor;
        
        int nAlpha = static_cast<int>(MAX_ALPHA * alpha);
        
        // 创建半透明画刷
        Gdiplus::SolidBrush brush(Gdiplus::Color(nAlpha, 100, 150, 255));
        
        // 绘制圆形轨迹
        graphics.FillEllipse(&brush, 
                            mouseTrail[i].x - radius, 
                            mouseTrail[i].y - radius, 
                            radius * 2, 
                            radius * 2);
    }
    
    // 绘制当前鼠标指针
    if (!mouseTrail.empty()) {
        MousePoint& currentPos = mouseTrail.back();
        
        // 外圈 (白色)
        Gdiplus::Pen whitePen(Gdiplus::Color(200, 255, 255, 255), 2.0f);
        Gdiplus::SolidBrush whiteBrush(Gdiplus::Color(100, 255, 255, 255));
        graphics.FillEllipse(&whiteBrush, 
                            currentPos.x - 8, 
                            currentPos.y - 8, 
                            16, 16);
        graphics.DrawEllipse(&whitePen, 
                            currentPos.x - 8, 
                            currentPos.y - 8, 
                            16, 16);
        
        // 内圈 (蓝色)
        Gdiplus::SolidBrush blueBrush(Gdiplus::Color(255, 0, 100, 255));
        graphics.FillEllipse(&blueBrush, 
                            currentPos.x - 4, 
                            currentPos.y - 4, 
                            8, 8);
    }
    
    // 将内存DC的内容复制到目标DC
    BitBlt(hdc, 0, 0, screenWidth, screenHeight, memDC, 0, 0, SRCCOPY);
    
    // 清理
    SelectObject(memDC, hOldBitmap);
    DeleteDC(memDC);
    DeleteObject(hBitmap);
}

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // 绘制鼠标模糊效果
            DrawMouseBlur(hdc);
            
            EndPaint(hwnd, &ps);
            break;
        }
        case WM_TIMER: {
            // 定时器消息，用于定期更新显示
            if (wParam == 1) {
                InvalidateRect(hwnd, NULL, FALSE);
            }
            break;
        }
        case WM_DESTROY: {
            PostQuitMessage(0);
            break;
        }
        case WM_SIZE: {
            if (wParam == SIZE_MINIMIZED && MINIMIZE_TO_TRAY) {
                // 最小化时隐藏窗口
                ShowWindow(hwnd, SW_HIDE);
            }
            break;
        }
        case WM_USER + 1: {
            // 系统托盘图标消息处理
            if (lParam == WM_RBUTTONDOWN) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hwnd); // 重要：设置前景窗口以显示上下文菜单
                
                HMENU hMenu = CreatePopupMenu();
                AppendMenu(hMenu, MF_STRING, 1001, TEXT("显示"));
                AppendMenu(hMenu, MF_STRING, 1002, TEXT("退出"));
                
                int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY, 
                                         pt.x, pt.y, 0, hwnd, NULL);
                DestroyMenu(hMenu);
                
                if (cmd == 1001) {
                    ShowWindow(hwnd, SW_RESTORE);
                    SetForegroundWindow(hwnd);
                } else if (cmd == 1002) {
                    PostQuitMessage(0);
                }
            } else if (lParam == WM_LBUTTONDBLCLK) {
                // 双击显示窗口
                ShowWindow(hwnd, SW_RESTORE);
                SetForegroundWindow(hwnd);
            }
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// 创建系统托盘图标
void CreateTrayIcon(HWND hwnd) {
    if (MINIMIZE_TO_TRAY) {
        NOTIFYICONDATA nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd;
        nid.uID = 1;
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage = WM_USER + 1;
        lstrcpy(nid.szTip, TEXT("鼠标模糊效果"));
        
        // 创建一个简单的彩色图标
        HICON hIcon = LoadIcon(NULL, IDI_INFORMATION);
        nid.hIcon = hIcon;
        
        Shell_NotifyIcon(NIM_ADD, &nid);
    }
}

// 删除系统托盘图标
void DeleteTrayIcon(HWND hwnd) {
    if (MINIMIZE_TO_TRAY) {
        NOTIFYICONDATA nid = {};
        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hwnd;
        nid.uID = 1;
        Shell_NotifyIcon(NIM_DELETE, &nid);
    }
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 初始化GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    ULONG_PTR gdiplusToken;
    Gdiplus::GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
    
    // 注册窗口类
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TEXT("MouseBlurClass");
    wc.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    
    RegisterClassEx(&wc);
    
    // 获取屏幕尺寸
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // 创建全屏、透明、顶层窗口，用于绘制鼠标轨迹
    HWND hwnd = CreateWindowEx(
        WS_EX_LAYERED | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_TRANSPARENT, // 使用分层窗口
        TEXT("MouseBlurClass"),
        TEXT("Mouse Blur Effect"),
        WS_POPUP,
        0, 0, screenWidth, screenHeight,
        NULL, NULL, hInstance, NULL
    );
    
    g_hwnd = hwnd;
    
    // 设置窗口为透明（使用黑色作为透明色）
    SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
    
    // 安装鼠标钩子 - 这是关键，只记录鼠标位置不阻止事件
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, hInstance, 0);
    if (!hMouseHook) {
        MessageBox(NULL, TEXT("无法安装鼠标钩子!"), TEXT("错误"), MB_OK | MB_ICONERROR);
        Gdiplus::GdiplusShutdown(gdiplusToken);
        return 1;
    }
    
    // 创建系统托盘图标
    CreateTrayIcon(hwnd);
    
    // 设置定时器清理过期轨迹点
    SetTimer(hwnd, 1, TIMER_INTERVAL, TimerProc);
    
    // 显示窗口
    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    // 清理
    if (hMouseHook) {
        UnhookWindowsHookEx(hMouseHook);
    }
    
    // 移除系统托盘图标
    DeleteTrayIcon(hwnd);
    
    // 关闭GDI+
    Gdiplus::GdiplusShutdown(gdiplusToken);
    
    return (int)msg.wParam;
}
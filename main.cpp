#include <windows.h>
#include <shellapi.h>
#include "resource.h"

// ==========================================
// 1. 配置模块：定义扫描码常量
// ==========================================
const WORD SCANCODE_NUMPAD4 = 75;  // 小键盘4 (左)
const WORD SCANCODE_NUMPAD5 = 76;  // 小键盘5 (下)
const WORD SCANCODE_NUMPAD6 = 77;  // 小键盘6 (右)
const WORD SCANCODE_NUMPAD8 = 72;  // 小键盘8 (上)

// ==========================================
// 2. 全局状态变量
// ==========================================
bool g_bMappingEnabled = false; //

// ==========================================
// 3. 键盘模拟模块
// ==========================================
namespace KeySimulator {
    // 统一封装按下和释放的逻辑
    void SendScanCode(WORD scanCode, bool isDown, bool isExtended = false) {
        INPUT input = { 0 };
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = scanCode;
        // 使用 KEYEVENTF_SCANCODE （扫描码）进行模拟
        input.ki.dwFlags = KEYEVENTF_SCANCODE | (isExtended ? KEYEVENTF_EXTENDEDKEY : 0);
        
        if (!isDown) {
            input.ki.dwFlags |= KEYEVENTF_KEYUP; // 如果是释放动作，追加 KEYUP 标志
        }
        
        SendInput(1, &input, sizeof(INPUT));
    }
}

// ==========================================
// 4. 输入映射与状态管理模块
// ==========================================
namespace InputMapper {
    HHOOK hMouseHook = NULL;

    // 状态管理结构体：自动处理防抖与按键发送
    struct KeyState {
        WORD scanCode;
        bool isPressed;

        void SetState(bool down) {
            if (isPressed != down) { // 只有状态发生变化时才发送输入
                isPressed = down;
                KeySimulator::SendScanCode(scanCode, down);
            }
        }
    };

    // 绑定映射关系
    KeyState stateLButton  = { SCANCODE_NUMPAD4, false };
    KeyState stateRButton  = { SCANCODE_NUMPAD6, false };
    KeyState stateX1Button = { SCANCODE_NUMPAD5, false };
    KeyState stateX2Button = { SCANCODE_NUMPAD8, false };

    // 安全释放所有按键
    void ReleaseAll() {
        stateLButton.SetState(false);
        stateRButton.SetState(false);
        stateX1Button.SetState(false);
        stateX2Button.SetState(false);
    }

    // 鼠标钩子回调
    LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
        if (nCode >= 0 && g_bMappingEnabled) {
            MSLLHOOKSTRUCT* pMouse = (MSLLHOOKSTRUCT*)lParam;
            DWORD xButton = GET_XBUTTON_WPARAM(pMouse->mouseData);

            switch (wParam) {
                // 左键
                case WM_LBUTTONDOWN: stateLButton.SetState(true); break;
                case WM_LBUTTONUP:   stateLButton.SetState(false); break;
                // 右键
                case WM_RBUTTONDOWN: stateRButton.SetState(true); break;
                case WM_RBUTTONUP:   stateRButton.SetState(false); break;
                // 侧键按下
                case WM_XBUTTONDOWN:
                    if (xButton == XBUTTON1) stateX1Button.SetState(true);
                    else if (xButton == XBUTTON2) stateX2Button.SetState(true);
                    break;
                // 侧键释放
                case WM_XBUTTONUP:
                    if (xButton == XBUTTON1) stateX1Button.SetState(false);
                    else if (xButton == XBUTTON2) stateX2Button.SetState(false);
                    break;
            }
        }
        // 始终保留原鼠标功能
        return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
    }

    bool InstallHook() {
        hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
        return hMouseHook != NULL;
    }

    void UninstallHook() {
        if (hMouseHook) {
            UnhookWindowsHookEx(hMouseHook);
            hMouseHook = NULL;
        }
        ReleaseAll(); // 卸载时强行释放所有模拟按键
    }
}

// ==========================================
// 5. 托盘图标与窗口消息模块
// ==========================================
namespace TrayApp {
    const UINT WM_TRAYICON = WM_USER + 1;
    const UINT ID_EXIT = 1001;
    const UINT ID_TOGGLE = 1002;
    NOTIFYICONDATA nid = { 0 };
    HMENU hTrayMenu = NULL;

    //向前声明
    void UpdateTrayIconAndMenu();

    void AddIcon(HWND hWnd) {
        // 1. 强制清零结构体，防止内存中的垃圾数据导致图标加载失败
        ZeroMemory(&nid, sizeof(NOTIFYICONDATA));

        nid.cbSize = sizeof(NOTIFYICONDATA);
        nid.hWnd = hWnd;
        nid.uID = 1;
        
        // 2. 设置标志位：必须有 NIF_ICON
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        
        nid.uCallbackMessage = WM_TRAYICON;
        
        // 3. 加载图标
        HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MAIN_ICON));
        
        // 容错处理：如果自定义图标加载失败，使用系统默认图标
        if (!hIcon) {
            hIcon = LoadIcon(NULL, IDI_APPLICATION);
        }
        
        nid.hIcon = hIcon;
        
        if(g_bMappingEnabled) {
            lstrcpyA(nid.szTip, "BFJetKeyMapping - Running");
        }else{
            lstrcpyA(nid.szTip, "BFJetKeyMapping - Paused");
        }

        // 4. 先尝试修改已存在的图标记录
        // 如果程序上次非正常退出，托盘里可能还有残留，NIM_MODIFY 会尝试更新它
        Shell_NotifyIconA(NIM_MODIFY, &nid);
        
        // 5. 正式添加图标
        Shell_NotifyIconA(NIM_ADD, &nid);

//        hTrayMenu = CreatePopupMenu();
//        AppendMenuA(hTrayMenu, MF_STRING, ID_EXIT, "Exit");
    }

    void UpdateTrayIconAndMenu() {
        if(hTrayMenu){
            DestroyMenu(hTrayMenu);
        }
        hTrayMenu = CreatePopupMenu();

        if(g_bMappingEnabled) {
            AppendMenuA(hTrayMenu, MF_STRING, ID_TOGGLE, "Turn OFF");
        }else{
            AppendMenuA(hTrayMenu, MF_STRING, ID_TOGGLE, "Turn ON");
        }

        AppendMenuA(hTrayMenu, MF_SEPARATOR, 0, NULL);
        AppendMenuA(hTrayMenu, MF_STRING, ID_EXIT, "Exit");

        if(g_bMappingEnabled) {
            lstrcpyA(nid.szTip, "BFJetKeyMapping - Running");
        }else{
            lstrcpyA(nid.szTip, "BFJetKeyMapping - Paused");
        }

        Shell_NotifyIconA(NIM_MODIFY, &nid);
    }

    void RemoveIcon() {
        Shell_NotifyIconA(NIM_DELETE, &nid);
        if (hTrayMenu) DestroyMenu(hTrayMenu);
    }

    LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
        switch (msg) {
            case WM_TRAYICON:
                if (lParam == WM_RBUTTONUP) {
                    UpdateTrayIconAndMenu();
                    POINT pt;
                    GetCursorPos(&pt);
                    SetForegroundWindow(hWnd);
                    TrackPopupMenu(hTrayMenu, TPM_BOTTOMALIGN | TPM_LEFTBUTTON, pt.x, pt.y, 0, hWnd, NULL);
                    PostMessage(hWnd, WM_NULL, 0, 0);
                }
                break;
            case WM_COMMAND:
                if(LOWORD(wParam) == ID_TOGGLE){
                    //切换前先释放所有模拟按键
                    if(g_bMappingEnabled){
                        InputMapper::ReleaseAll();
                    }
                    g_bMappingEnabled = !g_bMappingEnabled;
                    UpdateTrayIconAndMenu();
                }else if (LOWORD(wParam) == ID_EXIT) {
                    PostQuitMessage(0);
                }
                break;
            case WM_DESTROY:
                RemoveIcon();
                PostQuitMessage(0);
                break;
            default:
                return DefWindowProc(hWnd, msg, wParam, lParam);
        }
        return 0;
    }
}

// ==========================================
// 6. 应用程序入口
// ==========================================
int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 注册 Message-Only 窗口类
    WNDCLASSEXA wc = { sizeof(WNDCLASSEXA), 0, TrayApp::WndProc, 0, 0, hInstance, NULL, NULL, NULL, NULL, "BFJetKeyMappingClass", NULL };
    if (!RegisterClassExA(&wc)) return 1;

    // 创建隐藏窗口处理消息
    HWND hWnd = CreateWindowExA(0, "BFJetKeyMappingClass", "Low Level Input Mapper", 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);
    if (!hWnd) return 1;

    TrayApp::AddIcon(hWnd);

    if (!InputMapper::InstallHook()) {
        MessageBoxA(NULL, "安装鼠标钩子失败，请尝试以管理员身份运行。", "Error", MB_ICONERROR);
        return 1;
    }

    // 标准消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    InputMapper::UninstallHook();
    return (int)msg.wParam;
}
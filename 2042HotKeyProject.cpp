#include <windows.h>
#include <stdio.h>

// 全局变量
HHOOK hMouseHook;
NOTIFYICONDATA nid;
HMENU hTrayMenu;

// 跟踪各鼠标按键的按下状态
BOOL isLButtonDown = FALSE;
BOOL isRButtonDown = FALSE;
BOOL isX1ButtonDown = FALSE;
BOOL isX2ButtonDown = FALSE;

// 消息ID
#define WM_TRAYICON (WM_USER + 1)
#define ID_EXIT 1001

// 小键盘按键的扫描码 (十进制)
#define SCANCODE_NUMPAD4  75   // 小键盘4 (左)
#define SCANCODE_NUMPAD5  76   // 小键盘5 (下)
#define SCANCODE_NUMPAD6  77   // 小键盘6 (右)
#define SCANCODE_NUMPAD8  72   // 小键盘8 (上)

// 使用扫描码模拟键盘按键按下
void PressKeyWithScanCode(WORD scanCode, BOOL isExtended) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;  // 不使用虚拟键码
    input.ki.wScan = scanCode;  // 使用扫描码
    input.ki.dwFlags = KEYEVENTF_SCANCODE;  // 标志为扫描码输入
    
    // 对于需要扩展标志的键设置扩展标志
    if (isExtended) {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    
    SendInput(1, &input, sizeof(INPUT));
}

// 使用扫描码模拟键盘按键释放
void ReleaseKeyWithScanCode(WORD scanCode, BOOL isExtended) {
    INPUT input = {0};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;  // 不使用虚拟键码
    input.ki.wScan = scanCode;  // 使用扫描码
    input.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;  // 标志为扫描码释放
    
    // 对于需要扩展标志的键设置扩展标志
    if (isExtended) {
        input.ki.dwFlags |= KEYEVENTF_EXTENDEDKEY;
    }
    
    SendInput(1, &input, sizeof(INPUT));
}

// 创建系统托盘图标
void CreateTrayIcon(HWND hWnd) {
    ZeroMemory(&nid, sizeof(NOTIFYICONDATA));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = hWnd;
    nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAYICON;
    nid.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    lstrcpy(nid.szTip, "BFJetKeyMapping");
    Shell_NotifyIcon(NIM_ADD, &nid);
    
    hTrayMenu = CreatePopupMenu();
    AppendMenu(hTrayMenu, MF_STRING, ID_EXIT, "Exit");
}

// 移除系统托盘图标
void RemoveTrayIcon() {
    Shell_NotifyIcon(NIM_DELETE, &nid);
}

// 鼠标钩子回调函数
LRESULT CALLBACK MouseProc(int nCode, WPARAM wParam, LPARAM lParam) {
    if (nCode >= 0) {
        MSLLHOOKSTRUCT *pMouseStruct = (MSLLHOOKSTRUCT *)lParam;
        DWORD xButton = GET_XBUTTON_WPARAM(pMouseStruct->mouseData);
        
        // 鼠标左键 -> 小键盘4
        if (wParam == WM_LBUTTONDOWN && !isLButtonDown) {
            isLButtonDown = TRUE;
            PressKeyWithScanCode(SCANCODE_NUMPAD4, FALSE);
        }
        else if (wParam == WM_LBUTTONUP && isLButtonDown) {
            isLButtonDown = FALSE;
            ReleaseKeyWithScanCode(SCANCODE_NUMPAD4, FALSE);
        }
        
        // 鼠标右键 -> 小键盘6
        if (wParam == WM_RBUTTONDOWN && !isRButtonDown) {
            isRButtonDown = TRUE;
            PressKeyWithScanCode(SCANCODE_NUMPAD6, FALSE);
        }
        else if (wParam == WM_RBUTTONUP && isRButtonDown) {
            isRButtonDown = FALSE;
            ReleaseKeyWithScanCode(SCANCODE_NUMPAD6, FALSE);
        }
        
        // 鼠标侧键1 (X1) -> 小键盘5
        if (wParam == WM_XBUTTONDOWN) {
            if (xButton == XBUTTON1 && !isX1ButtonDown) {
                isX1ButtonDown = TRUE;
                PressKeyWithScanCode(SCANCODE_NUMPAD5, FALSE);
            }
        }
        else if (wParam == WM_XBUTTONUP) {
            if (xButton == XBUTTON1 && isX1ButtonDown) {
                isX1ButtonDown = FALSE;
                ReleaseKeyWithScanCode(SCANCODE_NUMPAD5, FALSE);
            }
        }
        
        // 鼠标侧键2 (X2) -> 小键盘8
        if (wParam == WM_XBUTTONDOWN) {
            if (xButton == XBUTTON2 && !isX2ButtonDown) {
                isX2ButtonDown = TRUE;
                PressKeyWithScanCode(SCANCODE_NUMPAD8, FALSE);
            }
        }
        else if (wParam == WM_XBUTTONUP) {
            if (xButton == XBUTTON2 && isX2ButtonDown) {
                isX2ButtonDown = FALSE;
                ReleaseKeyWithScanCode(SCANCODE_NUMPAD8, FALSE);
            }
        }
    }
    
    // 保留原鼠标功能
    return CallNextHookEx(hMouseHook, nCode, wParam, lParam);
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TRAYICON:
            if (lParam == WM_RBUTTONUP) {
                POINT pt;
                GetCursorPos(&pt);
                SetForegroundWindow(hWnd);
                TrackPopupMenu(hTrayMenu, TPM_BOTTOMALIGN | TPM_LEFTBUTTON, 
                              pt.x, pt.y, 0, hWnd, NULL);
                PostMessage(hWnd, WM_NULL, 0, 0);
            }
            break;
            
        case WM_COMMAND:
            if (LOWORD(wParam) == ID_EXIT) {
                PostQuitMessage(0);
            }
            break;
            
        case WM_DESTROY:
            RemoveTrayIcon();
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// 注册窗口类
ATOM RegisterWindowClass(HINSTANCE hInstance) {
    WNDCLASSEX wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = "LowLevelInputClass";
    
    return RegisterClassEx(&wc);
}

// 安装鼠标钩子
void SetHook() {
    hMouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseProc, GetModuleHandle(NULL), 0);
    if (hMouseHook == NULL) {
        printf("安装鼠标钩子失败！错误代码: %d\n", GetLastError());
        exit(1);
    }
    printf("鼠标钩子安装成功，正在运行...\n");
    printf("功能映射:\n");
    printf(" - 鼠标左键 -> 小键盘4 \n");
    printf(" - 鼠标右键 -> 小键盘6 \n");
    printf(" - 鼠标侧键1 (X1) -> 小键盘5 \n");
    printf(" - 鼠标侧键2 (X2) -> 小键盘8 \n");
    printf("右键点击系统托盘图标可退出程序\n");
}

// 卸载钩子并释放所有按键
void Unhook() {
    if (isLButtonDown) ReleaseKeyWithScanCode(SCANCODE_NUMPAD4, FALSE);
    if (isRButtonDown) ReleaseKeyWithScanCode(SCANCODE_NUMPAD6, FALSE);
    if (isX1ButtonDown) ReleaseKeyWithScanCode(SCANCODE_NUMPAD5, FALSE);
    if (isX2ButtonDown) ReleaseKeyWithScanCode(SCANCODE_NUMPAD8, FALSE);
    
    UnhookWindowsHookEx(hMouseHook);
    printf("鼠标钩子已卸载\n");
}

int main() {
    HINSTANCE hInstance = GetModuleHandle(NULL);
    
    if (!RegisterWindowClass(hInstance)) {
        printf("窗口类注册失败！错误代码: %d\n", GetLastError());
        return 1;
    }
    
    HWND hWnd = CreateWindowEx(0, "LowLevelInputClass", "Low Level Input Mapper", 0,
                              CW_USEDEFAULT, CW_USEDEFAULT, 0, 0,
                              HWND_MESSAGE, NULL, hInstance, NULL);
                              
    if (!hWnd) {
        printf("窗口创建失败！错误代码: %d\n", GetLastError());
        return 1;
    }
    
    CreateTrayIcon(hWnd);
    SetHook();
    
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    Unhook();
    return 0;
}

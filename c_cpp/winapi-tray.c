// SPDX-License-Identifier: MIT

// http://ivoronline.com/Coding/Languages/C/Tutorials/C%20-%20WinAPI%20-%20Window%20-%20With%20Tray%20Icon%20-%20With%20Popup%20Menu.php

#include <Windows.h>

#define TRAY_NORETURN __declspec(noreturn)

#define TRAY_CLASSNAME	"TrayDemo"
#define TRAY_TITLE		"Tray Demo"
#define TRAY_STYLE      0

enum {
    TRAY_CMD_TEST = 1,
    TRAY_CMD_QUIT,
};

TRAY_NORETURN static void Die(const char* message) {
    MessageBox(NULL, message, TRAY_TITLE " - Error", MB_OK | MB_ICONERROR);
    exit(0);
}

static LRESULT CALLBACK TrayProc(HWND wnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        // Create tray icon
        NOTIFYICONDATA nid      = { 0 }; nid.cbSize = sizeof(nid);
        nid.hWnd                = wnd;
        nid.uID                 = 1;
        nid.uFlags              = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.uCallbackMessage    = WM_APP;
        nid.hIcon               = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(1));
        if (!Shell_NotifyIcon(NIM_ADD, &nid)) {
            Die("Failed to register tray icon");
        }
    } break;
    case WM_CLOSE: {
        // Remove tray icon
        NOTIFYICONDATA nid      = { 0 }; nid.cbSize = sizeof(nid);
        nid.hWnd                = wnd;
        nid.uID                 = 1;
        if (!Shell_NotifyIcon(NIM_DELETE, &nid)) {
            Die("Failed to remove tray icon");
        }
        // Destroy window
        PostQuitMessage(0);
        return DefWindowProc(wnd, msg, wp, lp);
    } break;
    case WM_APP: {
        switch (lp) {
        case WM_RBUTTONUP: {
            SetForegroundWindow(wnd);
            // Create popup
            HMENU popup = CreatePopupMenu();
            if (!popup) {
                Die("Failed to create popup");
            }
            InsertMenu(popup, 0, MF_BYPOSITION | MF_STRING, TRAY_CMD_TEST, "Test");
            InsertMenu(popup, 1, MF_BYPOSITION | MF_SEPARATOR, 0, 0);
            InsertMenu(popup, 2, MF_BYPOSITION | MF_STRING, TRAY_CMD_QUIT, "Quit");

            // Show at cursor
            POINT cur = { 0 }; GetCursorPos(&cur);
            WORD cmd = TrackPopupMenu(popup, TPM_LEFTALIGN | TPM_RIGHTALIGN | TPM_RETURNCMD | TPM_NONOTIFY, cur.x, cur.y, 0, wnd, NULL);
            SendMessage(wnd, WM_COMMAND, cmd, 0);

            DestroyMenu(popup);
        } break;
        }
    } break;
    case WM_COMMAND: {
        switch (wp) {
        case TRAY_CMD_TEST: {
            MessageBox(wnd, "Test button pressed", TRAY_TITLE, MB_OK);
        } break;
        case TRAY_CMD_QUIT: {
            PostQuitMessage(0);
        }
        }
    } break;
    }
    return DefWindowProc(wnd, msg, wp, lp);
}

int WinMain(_In_ HINSTANCE inst, _In_opt_ HINSTANCE prev, _In_ LPSTR cmdline, _In_ int show) {
    if (FindWindow(TRAY_CLASSNAME, TRAY_TITLE)) {
        Die("Tray demo is already running");
    }

    WNDCLASSEX wcl = { 0 }; wcl.cbSize = sizeof(wcl);
    wcl.hInstance       = inst;
    wcl.lpfnWndProc     = &TrayProc;
    wcl.lpszClassName   = TRAY_CLASSNAME;
    RegisterClassEx(&wcl);

    HWND wnd = CreateWindow(TRAY_CLASSNAME, TRAY_TITLE, TRAY_STYLE, 100, 100, 250, 100, NULL, NULL, inst, NULL);
    if (!wnd) {
        Die("Failed to create tray window");
    }

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}

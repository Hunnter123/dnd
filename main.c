#include <windows.h>
#include "server.h"
#include "client.h"

HWND username;
HWND ip;
HWND hwnd;

void run() {
    char usernamet[30];
    GetWindowText(username, usernamet, 30);
    DestroyWindow(hwnd);
    if (strcmp(usernamet, "server") == 0) {
        startserver();
    } else {
        startclient(usernamet);
    }
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CLOSE:
            DestroyWindow(hwnd);
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        case WM_COMMAND:
            if (LOWORD(wParam) == 1) {  // Check button ID
                if (HIWORD(wParam) == BN_CLICKED) {
                    run();
                }
            }
            break;
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {0};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "MyWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    RegisterClass(&wc);

    hwnd = CreateWindow(
        "MyWindowClass",
        "Login",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        600, 200,
        NULL, NULL, hInstance, NULL
    );

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    HWND hwndButton = CreateWindow(
        "BUTTON",       // Predefined class for buttons
        "Login",     // Button text
        WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
        235, 75, 100, 30, // x, y, width, height
        hwnd,           // Parent window
        (HMENU)1,       // Button ID
        hInstance,
        NULL
    );

    username = CreateWindowEx(
        WS_EX_CLIENTEDGE,   // Extended style (gives a sunken border)
        "EDIT",             // Class name for text boxes
        "",                 // Initial text
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, // Styles
        50, 50, 200, 25,  // x, y, width, height
        hwnd,               // Parent window
        (HMENU)2,           // Control ID (unique number)
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
        NULL
    );


    ip = CreateWindowEx(
        WS_EX_CLIENTEDGE,   // Extended style (gives a sunken border)
        "EDIT",             // Class name for text boxes
        "",                 // Initial text
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, // Styles
        325, 50, 200, 25,  // x, y, width, height
        hwnd,               // Parent window
        (HMENU)3,           // Control ID (unique number)
        (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE),
        NULL
    );
    // server testing
    //DestroyWindow(hwnd);
    //startserver();
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
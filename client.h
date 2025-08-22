#ifndef CLIENT_H
#define CLIENT_H

#include <winsock2.h>
#include <process.h>

#define BUF_SIZE 512
#define WM_UPDATE_HEALTH (WM_USER + 1)

SOCKET sock = INVALID_SOCKET;
HWND inputBox;
HWND logtext;
HWND healthh;
WNDPROC oldInputProc;
volatile int clientRunning = 1;

int Health;

// ------------------------
// Log to GUI
// ------------------------
void Logg(const char* text) {
    if (!logtext) return;

    int len = GetWindowTextLength(logtext);
    SendMessage(logtext, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessage(logtext, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessage(logtext, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
}

// ------------------------
// Input box procedure
// ------------------------
LRESULT CALLBACK InputProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYUP && wParam == VK_RETURN) {
        char buf[256];
        GetWindowText(hwnd, buf, sizeof(buf));
        if (*buf) {
            send(sock, buf, (int)strlen(buf), 0);
            SetWindowText(hwnd, "");
        }
        return 0;
    }
    return CallWindowProc(oldInputProc, hwnd, msg, wParam, lParam);
}

// ------------------------
// GUI Window procedure
// ------------------------
LRESULT CALLBACK ClientWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CLOSE: clientRunning = 0; DestroyWindow(hwnd); break;
        case WM_DESTROY: PostQuitMessage(0); break;
        case WM_UPDATE_HEALTH: {
            char *text = (char*)lParam;
            SetWindowText(healthh, text);
            free(text);  // free the duplicated string
            break;
        }
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ------------------------
// Thread to receive messages
// ------------------------
void recvThread(void* arg) {
    char buffer[BUF_SIZE];
    while (clientRunning) {
        int bytes = recv(sock, buffer, sizeof(buffer)-1, 0);
        if (bytes <= 0) break;
        buffer[bytes] = '\0';
        char *cmd = strtok(buffer, " ");
        if (cmd && strcmp(cmd, "d") == 0) {
            char *value = strtok(NULL, "");
            Health = atoi(value);
            SetWindowText(healthh, value);
        } else if (cmd && strcmp(cmd, "h") == 0) {
            char *value = strtok(NULL, "");
            Health = atoi(value);
            PostMessage(healthh, WM_UPDATE_HEALTH, 0, (LPARAM)strdup(value));
        } else {
            Logg(buffer);
        }
    }
    _endthread();
}

// ------------------------
// Client GUI Thread
// ------------------------
DWORD WINAPI ClientWindowThread(LPVOID lpParam) {
    HINSTANCE hInstance = (HINSTANCE)lpParam;

    WNDCLASS wc = {0};
    wc.lpfnWndProc = ClientWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ClientWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        "ClientWindowClass", "Client", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    logtext = CreateWindowEx(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY,
        450, 25, 300, 475, hwnd, (HMENU)1, hInstance, NULL
    );

    inputBox = CreateWindowEx(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        450, 515, 300, 25, hwnd, (HMENU)2, hInstance, NULL
    );

    HFONT hFont = CreateFont(
        24,                        // height of the font in logical units
        0,                         // average character width (0 = default)
        0,                         // angle of escapement
        0,                         // orientation angle
        FW_BOLD,                   // font weight (bold)
        FALSE,                     // italic
        FALSE,                     // underline
        FALSE,                     // strikeout
        ANSI_CHARSET,              // character set
        OUT_DEFAULT_PRECIS,        // output precision
        CLIP_DEFAULT_PRECIS,       // clipping precision
        DEFAULT_QUALITY,           // output quality
        DEFAULT_PITCH | FF_SWISS,  // pitch and family
        "Arial"                    // font name
    );

    healthh = CreateWindowEx(
        0,                      // extended style
        "STATIC",               // class name for a static control
        "ERROR",         // initial text
        WS_CHILD | WS_VISIBLE | SS_LEFT,  // styles: child, visible, left-aligned
        10, 10, 150, 25,       // position and size (x, y, width, height)
        hwnd,                   // parent window
        (HMENU)3,               // control ID
        hInstance,              // instance handle
        NULL                    // extra params
    );

    SendMessage(healthh, WM_SETFONT, (WPARAM)hFont, TRUE);

    oldInputProc = (WNDPROC)SetWindowLongPtr(inputBox, GWLP_WNDPROC, (LONG_PTR)InputProc);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    clientRunning = 0;
    return 0;
}

// ------------------------
// Client networking + GUI
// ------------------------
int startclient(const char* username) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) return 1;

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) { WSACleanup(); return 1; }

    struct sockaddr_in server = {0};
    server.sin_family = AF_INET;
    server.sin_port = htons(12345);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) != 0) {
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    send(sock, username, (int)strlen(username), 0);

    // Start GUI thread
    CreateThread(NULL, 0, ClientWindowThread, GetModuleHandle(NULL), 0, NULL);

    // Start recv thread
    _beginthread(recvThread, 0, NULL);

    while(clientRunning) Sleep(100);

    closesocket(sock);
    WSACleanup();
    return 0;
}
#endif
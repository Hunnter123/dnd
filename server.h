#ifndef SERVER_H
#define SERVER_H

#include <winsock2.h>
#include <stdio.h>
#include <stdint.h>

#define MAX_CLIENTS 4
#define BUF_SIZE 512
#define USERNAME_LEN 15

SOCKET listenSocket = INVALID_SOCKET;
SOCKET clientSockets[MAX_CLIENTS];
HWND console;
HWND logtext;

struct {
    unsigned int clientCount : 3;
    uint16_t health[MAX_CLIENTS];
    volatile int serverRunning: 1;
    char usernames[MAX_CLIENTS][USERNAME_LEN];
} client;
WNDPROC oldEditProc;

// ---------------------------
// Send message to specific client
// ---------------------------
void SendToClient(const char* username, const char* message) {
    if (!username || !message) return;
    uint8_t length = (int)strlen(message);
    for (int i = 0; i < client.clientCount; i++) {
        if (strcmp(client.usernames[i], username) == 0) {
            send(clientSockets[i], message, length, 0);
            break;
        }
    }
}

// ---------------------------
// Log to GUI
// ---------------------------
void Log(const char* text) {
    if (!logtext) return;

    int len = GetWindowTextLength(logtext);
    SendMessage(logtext, EM_SETSEL, (WPARAM)len, (LPARAM)len);
    SendMessage(logtext, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessage(logtext, EM_REPLACESEL, FALSE, (LPARAM)"\r\n");
}

// ---------------------------
// Broadcast to all clients
// ---------------------------
void send_all_clients(const char* text) {
    uint8_t length = (int)strlen(text);
    for (int i = 0; i < client.clientCount; i++) {
        send(clientSockets[i], text, length, 0);
    }
}

// ---------------------------
// Handle console input
// ---------------------------
LRESULT CALLBACK EditProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_KEYUP && wParam == VK_RETURN) {
        char buf[256];
        GetWindowText(hwnd, buf, sizeof(buf));
        send_all_clients(buf);
        SetWindowText(console, "");
        return 0;
    }
    return CallWindowProc(oldEditProc, hwnd, msg, wParam, lParam);
}

// ---------------------------
// GUI Window Procedure
// ---------------------------
LRESULT CALLBACK ServerWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CLOSE: DestroyWindow(hwnd); break;
        case WM_DESTROY:
            client.serverRunning = 0;
            if (listenSocket != INVALID_SOCKET) closesocket(listenSocket);
            for (int i = 0; i < client.clientCount; i++) closesocket(clientSockets[i]);
            PostQuitMessage(0);
            break;
        default: return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// ---------------------------
// GUI Thread
// ---------------------------
DWORD WINAPI ServerWindowThread(LPVOID lpParam) {
    HINSTANCE hInstance = (HINSTANCE)lpParam;

    WNDCLASS wc = {0};
    wc.lpfnWndProc = ServerWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "ServerWindowClass";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    RegisterClass(&wc);

    HWND hwnd = CreateWindow(
        "ServerWindowClass", "Server", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 800, 600,
        NULL, NULL, hInstance, NULL
    );

    logtext = CreateWindowEx(
        WS_EX_CLIENTEDGE, "EDIT", "", 
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOVSCROLL | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
        450, 25, 300, 475, hwnd, (HMENU)1, hInstance, NULL
    );

    console = CreateWindowEx(
        WS_EX_CLIENTEDGE, "EDIT", "",
        WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
        450, 515, 300, 25, hwnd, (HMENU)2, hInstance, NULL
    );

    oldEditProc = (WNDPROC)SetWindowLongPtr(console, GWLP_WNDPROC, (LONG_PTR)EditProc);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return 0;
}

// ---------------------------
// Thread to handle client
// ---------------------------
DWORD WINAPI ClientThread(LPVOID lpParam) {
    int clientId = (int)(intptr_t)lpParam;
    SOCKET clientSocket = clientSockets[clientId];

    // Receive username
    int bytes = recv(clientSocket, client.usernames[clientId], sizeof(client.usernames[clientId]) - 1, 0);
    if (bytes > 0) {
        client.usernames[clientId][bytes] = '\0';
        char msg[128];
        snprintf(msg, sizeof(msg), "[Server] %s connected", client.usernames[clientId]);
        Log(msg);
        char data[100];
        snprintf(data, 100, "d %d", client.health[clientId]);
        SendToClient(client.usernames[clientId], data);
    }

    char buffer[BUF_SIZE];
    while (client.serverRunning) {
        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived <= 0) {
            snprintf(buffer, sizeof(buffer), "[Server] %s disconnected", client.usernames[clientId]);
            Log(buffer);
            closesocket(clientSocket);
            return 0;
        }

        buffer[bytesReceived] = '\0';

        // Check command: "h <username> <message>"
        char *cmd = strtok(buffer, " ");
        if (cmd && strcmp(cmd, "client.health") == 0) {
            char *targetUser = strtok(NULL, " ");
            char *value = strtok(NULL, "");
            if (targetUser && value){
                client.health[clientId] -= atoi(value);
                char healthstr[4];
                snprintf(healthstr, sizeof(healthstr), "%d", client.health[clientId]);
                SendToClient(targetUser, healthstr);
            }
        } else {
            char temp[BUF_SIZE];
            snprintf(temp, sizeof(temp), "%s: %s", client.usernames[clientId], buffer);
            Log(temp);
            send_all_clients(temp);
        }
    }
    return 0;
}

// ---------------------------
// Server networking
// ---------------------------
int startserver() {
    client.clientCount = 0;
    client.serverRunning = 1;
    CreateThread(NULL, 0, ServerWindowThread, GetModuleHandle(NULL), 0, NULL);

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) return 1;

    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) { WSACleanup(); return 1; }

    struct sockaddr_in serverAddr = {0};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(12345);

    if (bind(listenSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    listen(listenSocket, SOMAXCONN);

    while (client.serverRunning) {
        struct sockaddr_in clientAddr;
        int clientSize = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientSize);
        if (clientSocket == INVALID_SOCKET) continue;

        if (client.clientCount >= MAX_CLIENTS) {
            Log("[Server] Max clients reached");
            closesocket(clientSocket);
            continue;
        }

        clientSockets[client.clientCount] = clientSocket;
        client.health[client.clientCount] = 100;
        CreateThread(NULL, 0, ClientThread, (LPVOID)(intptr_t)client.clientCount, 0, NULL);
        client.clientCount++;
    }

    WSACleanup();
    return 0;
}

#endif
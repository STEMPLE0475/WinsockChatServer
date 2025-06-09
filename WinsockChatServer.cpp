#include <iostream>
#include <string>
#include <format>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <thread>
#include <vector>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

atomic<bool> isRunning(true); // 전역 종료 플래그

//클라이언트 개별 처리 함수
void handleClient(SOCKET clientSocket) {
    const int bufferSize = 1024;
    char buffer[bufferSize];

    while (true) {
        int bytesReceived = recv(clientSocket, buffer, bufferSize, 0);
        if (bytesReceived <= 0) break;

        //수신된 데이터 처리
        buffer[bytesReceived] = '\0';
        cout << format("클라이언트로부터 수신: {}\n", buffer);

        //응답 전송
        string response = "메시지 수신 완료: ";
        response += buffer;

        send(clientSocket, response.c_str(), response.length(), 0);
    }

    closesocket(clientSocket);
}

void CloseServer(SOCKET& serverSocket, vector<SOCKET>& clientSockets) {
    if (!clientSockets.empty()) {
        for (auto clientSocket : clientSockets) closesocket(clientSocket);
    }
    closesocket(serverSocket);
    WSACleanup();
    cout << "서버 종료";
}

void handleInput(SOCKET& serverSocket, vector<SOCKET>& clientSockets) {
    string input;
    while (getline(cin, input)) {
        if (input == "exit") {
            isRunning = false;
            shutdown(serverSocket, SD_BOTH); // accept()를 깨움
            break;
        }
    }
}


int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cerr << "WSAStartup 실패" << endl;
        return 1;
    }

    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        cerr << "소켓 생성 실패: " << WSAGetLastError() << endl;
        WSACleanup();
        return 1;
    }
    vector<SOCKET> clientSockets;

    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(12345);

    if (bind(serverSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR) {
        cerr << "바인딩 실패: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    if (listen(serverSocket, SOMAXCONN) == SOCKET_ERROR) {
        cerr << "리스닝 실패: " << WSAGetLastError() << endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    cout << "서버가 시작되었습니다. 클라이언트를 기다리는 중..." << endl;

    //입력 감시 스레드
    thread inputThread(handleInput, ref(serverSocket), ref(clientSockets));
    inputThread.detach();

    //클라이언트 연결 수락 
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);


    while (isRunning) {
        SOCKET clientSocket = accept(serverSocket, reinterpret_cast<sockaddr*>(&clientAddr), &clientAddrSize);
        if (clientSocket == INVALID_SOCKET) {
            cerr << "클라이언트 연결 실패: " << WSAGetLastError() << endl;
            continue;
        }

        //clientSockets에 추가
        clientSockets.push_back(clientSocket);

        // 클라이언트 IP 출력
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        cout << format("클라이언트 연결됨: {}:{}\n", clientIP, ntohs(clientAddr.sin_port));

        // 독립 실행
        thread clientThread(handleClient, clientSocket);
        clientThread.detach();
    }

    return 0;
}


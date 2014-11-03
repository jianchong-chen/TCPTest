// OneServerMain.cpp  

#include <iostream>  
#include <cstdio>  
#include <string>  
#include <cstring>  
#include <vector>  
#include <iterator>  
#include <algorithm>  
#include <Winsock2.h>  
#include <Windows.h>  

#pragma comment(lib,"ws2_32.lib")

using namespace std;  
HANDLE bufferMutex;     // 令其能互斥成功正常通信的信号量句柄  
SOCKET sockConn;        // 客户端的套接字  
vector <SOCKET> clientSocketGroup;  

int main()  
{  
    // 加载socket动态链接库(dll)  
    WORD wVersionRequested;  
    WSADATA wsaData;    // 这结构是用于接收Wjndows Socket的结构信息的  
    wVersionRequested = MAKEWORD( 2, 2 );   // 请求2.2版本的WinSock库  
    int err = WSAStartup( wVersionRequested, &wsaData );  
    if ( err != 0 ) 
    {
        return -1;          // 返回值为零的时候是表示成功申请WSAStartup  
    }  
    if ( LOBYTE( wsaData.wVersion ) != 2 || HIBYTE( wsaData.wVersion ) != 2 ) { // 检测是否2.2版本的socket库  
        WSACleanup( );  
        return -1;   
    }  

    // 创建socket操作，建立流式套接字，返回套接字号sockSrv  
    SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0);     

    // 套接字sockSrv与本地地址相连  
    SOCKADDR_IN addrSrv;  
    addrSrv.sin_addr.S_un.S_addr = htonl(INADDR_ANY); // 将INADDR_ANY转换为网络字节序，调用 htonl(long型)或htons(整型)  
    addrSrv.sin_family = AF_INET;  
    addrSrv.sin_port = htons(6000);  

    if(SOCKET_ERROR == bind(sockSrv, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR))){ // 第二参数要强制类型转换  
        return -1;  
    }  

    // 将套接字设置为监听模式（连接请求）， listen()通知TCP服务器准备好接收连接  
    listen(sockSrv, 20);  

    cout << "服务器已成功就绪，若服务器想发送信息给客户端，可直接输入内容后按回车.\n";  
    // accept()，接收连接，等待客户端连接  

    bufferMutex = CreateSemaphore(NULL, 1, 1, NULL);   

    DWORD WINAPI SendMessageThread(LPVOID IpParameter);  
    DWORD WINAPI ReceiveMessageThread(LPVOID IpParameter);  

    HANDLE sendThread = CreateThread(NULL, 0, SendMessageThread, NULL, 0, NULL);    

    while(true){    // 不断等待客户端请求的到来  
        sockConn = accept(sockSrv, NULL, NULL);  
        if (SOCKET_ERROR != sockConn){  
            clientSocketGroup.push_back(sockConn);  
        }  
        HANDLE receiveThread = CreateThread(NULL, 0, ReceiveMessageThread, (LPVOID)sockConn, 0, NULL);    
        WaitForSingleObject(bufferMutex, INFINITE);     // P（资源未被占用）   
        if(NULL == receiveThread) {   
            printf("\nCreatThread AnswerThread() failed.\n");   
        }   
        else{   
            printf("\nCreate Receive Client Thread OK.\n");   
        }   
        ReleaseSemaphore(bufferMutex, 1, NULL);     // V（资源占用完毕）  
    }  

    WaitForSingleObject(sendThread, INFINITE);  // 等待线程结束  
    CloseHandle(sendThread);  
    CloseHandle(bufferMutex);  
    WSACleanup();   // 终止对套接字库的使用  
    printf("\n");  
    system("pause");  
    return 0;  
}  


DWORD WINAPI SendMessageThread(LPVOID IpParameter)  
{  
    while(1){  
        string talk;  
        getline(cin, talk);  
        WaitForSingleObject(bufferMutex, INFINITE);     // P（资源未被占用）    
        /*  if("quit" == talk){ 
        ReleaseSemaphore(bufferMutex, 1, NULL);     // V（资源占用完毕）  
        return 0; 
        } 
        else*/  
        {  
            talk.append("\n");  
        }  
        printf("I Say:(\"quit\"to exit):");  
        cout << talk;  
        for(int i = 0; i < clientSocketGroup.size(); ++i){  
            //      send(clientSocketGroup[i], talk.c_str(), talk.size(), 0);   // 发送信息  
            send(clientSocketGroup[i], talk.c_str(), 200, 0);   // 发送信息  
        }  
        ReleaseSemaphore(bufferMutex, 1, NULL);     // V（资源占用完毕）   
    }  
    return 0;  
}  


DWORD WINAPI ReceiveMessageThread(LPVOID IpParameter)  
{  
    SOCKET ClientSocket=(SOCKET)(LPVOID)IpParameter;   
    while(1){     
        char recvBuf[300];  
        recv(ClientSocket, recvBuf, 200, 0);  
        WaitForSingleObject(bufferMutex, INFINITE);     // P（资源未被占用）    

        if (recvBuf[0] == 'q' && recvBuf[1] == 'u' && recvBuf[2] == 'i' && recvBuf[3] == 't' && recvBuf[4] == '\0'){  
            vector<SOCKET>::iterator result = find(clientSocketGroup.begin(), clientSocketGroup.end(), ClientSocket);  
            clientSocketGroup.erase(result);  
            closesocket(ClientSocket);  
            ReleaseSemaphore(bufferMutex, 1, NULL);     // V（资源占用完毕）   
            printf("\nAttention: A Client has leave...\n", 200, 0);  
            break;  
        }  

        printf("%s Says: %s\n", "One Client", recvBuf);     // 接收信息  

        ReleaseSemaphore(bufferMutex, 1, NULL);     // V（资源占用完毕）   
    }  
    return 0;  
} 

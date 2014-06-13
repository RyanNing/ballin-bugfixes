#include <stdio.h>                   //用于printf等函数的调用
#include <winsock2.h>                //Socket的函数调用　
#include <stdlib.h>
#include <string.h>
#include <string>
#include <windows.h>
#include <iostream>
  
#pragma comment (lib, "ws2_32")
  
using namespace std;
  
#define BUFFER_SIZE 8192
#define FILE_NAME_MAX_SIZE 512

string GetLocalIpAddress()  
{  
    WORD wVersionRequested = MAKEWORD(2, 2);  
  
    WSADATA wsaData;  
    if (WSAStartup(wVersionRequested, &wsaData) != 0)  
        return "";  
  
    char local[255] = {0};  
    gethostname(local, sizeof(local));  
    hostent* ph = gethostbyname(local);  
    if (ph == NULL)  
        return "";  
  
    in_addr addr;  
    memcpy(&addr, ph->h_addr_list[0], sizeof(in_addr)); // 这里仅获取第一个ip  
  
    string localIP;  
    localIP.assign(inet_ntoa(addr));  
  
    WSACleanup();  
    return localIP;  
}


string myrecv(SOCKET clientSocket,long size) {
    size_t recived = 0;
    string sbuf; 
    char buffer[BUFFER_SIZE] = {0};
    while (recived < size) {
        memset(buffer,0,BUFFER_SIZE);
        recived += recv(clientSocket, buffer, BUFFER_SIZE, NULL);
		sbuf.append(buffer);
    }
	return sbuf;
}
  
void sendInfo(SOCKET clientSock,char * foldpath)
{
    char res[BUFFER_SIZE]="\0";
      
    HANDLE file;
    WIN32_FIND_DATA fileData;
    char line[255];                 //文件夹名
    TCHAR FOLD[127];
    TCHAR addFOLD[127];
  
    memset(FOLD,0,sizeof(FOLD));
    memset(addFOLD,0,sizeof(addFOLD));
    memset(res,0,sizeof(res));
  
    MultiByteToWideChar(CP_ACP, 0,foldpath, -1, FOLD, 100);
  
    lstrcpy(addFOLD,FOLD);
  
    lstrcat(addFOLD,TEXT("*"));
  
    file = FindFirstFile(addFOLD, &fileData);
  
    cout<<foldpath<<endl;
    if(file==INVALID_HANDLE_VALUE)
    {
        cout<<"error"<<endl;
        return ;
    }
  
  
    FindNextFile(file, &fileData);
  
    HANDLE hand;
    while(FindNextFile(file, &fileData)){
  
        {
            wcstombs(line,fileData.cFileName,255);
            strcat(res,line);
        }
        {
            SYSTEMTIME st;
  
            FileTimeToSystemTime(&fileData.ftCreationTime,&st);
  
            char createt[100];
            sprintf(createt,"\n%d/%d/%d %d:%d\n",st.wYear,st.wMonth,st.wDay,st.wHour,st.wMinute);
            strcat(res,createt);
  
        }
        {
            LPTSTR temp ;
            temp=(LPTSTR)malloc(100*sizeof(LPTSTR)); 
            lstrcpy(temp,FOLD);
            lstrcat(temp,TEXT("//"));
            lstrcat(temp,fileData.cFileName);  
  
  
            HANDLE hfile=CreateFile(
                temp,
                GENERIC_READ|GENERIC_WRITE,
                0,
                NULL,
                OPEN_ALWAYS,
                fileData.dwFileAttributes,
                NULL
                );
            DWORD filesize=GetFileSize(hfile,NULL);
            char judge[100];
            if( filesize == INVALID_FILE_SIZE ) 
            {
                sprintf(judge,"-1\n");
                strcat(res,judge);
            }
            else{
                char szText[100];
                sprintf(szText,"%ld\n",filesize);
                strcat(res,szText);
  
            }
  
            CloseHandle(hfile);
        }
    }
 
	cout<<res<<endl;
  
    int val=send(clientSock, res, BUFFER_SIZE, NULL);
  
    return ;
}
void addFolder(char * Path,char * name)
{
  
    TCHAR PATH[255];
    HANDLE file;
    WIN32_FIND_DATA fileData;
    char mypath[127]={0};
  
    strcpy(mypath,Path);
    strcat(mypath,name);
    MultiByteToWideChar(CP_ACP, 0,mypath, -1, PATH, 255);
  
  
  
    CreateDirectory(PATH,NULL);
  
}
  
DWORD WINAPI ProcessClientRequests(LPVOID lpParam)
{
      
    SOCKET* clientsocket=(SOCKET*)lpParam;  //这里需要强制转换，注意：指针类型的
  
    /*
    char* msg="Hello, my client.\r\n";
    send(*clientsocket, msg, strlen(msg)+sizeof(char), NULL);
    */
    printf("***SYS***    HELLO.\n");
  
  
    char * currentpath=new char[255];
	memset(currentpath,0,sizeof(currentpath));
	strcpy(currentpath,"root//");

    sendInfo(*clientsocket,currentpath);
    
  
    printf("***SYS***    HELLO.\n");
  
    while(TRUE)
    {
        char buffer[BUFFER_SIZE]={0};
  
        //recv(*clientsocket, buffer, BUFFER_SIZE, NULL);
		string sbuffer = myrecv(*clientsocket,BUFFER_SIZE);
		strcpy(buffer,sbuffer.c_str());
  
        if(buffer[0]=='0')//folder info
        {
            strcat(currentpath,buffer+1);
            strcat(currentpath,"//");
            sendInfo(*clientsocket,currentpath);
        }
        else if(strcmp(buffer,"back")==0)//back
        {
			if (strcmp(currentpath,"root//") != 0) {
			
				int q=strlen(currentpath);
  
				currentpath[q]=0;
				currentpath[q-1]=0;
				currentpath[q-2]=0;
  
  
				for(q=strlen(currentpath)-3;q>=0;q--)
				{
  
					if(currentpath[q]=='/') break;
					currentpath[q]=0;
				}
  
				currentpath[q+1]='\0';
				if(strcmp(currentpath,"d://")==0) {
					strcat(currentpath,"root//");
  
				}
			}
			sendInfo(*clientsocket,currentpath);

        }
        else if(buffer[0]=='1')//download
        {
  
            TCHAR szBuffer[513];
  
            char name[127]={0};
            char filebuffer[BUFFER_SIZE]={0};
  
            strcpy(name,buffer+1);
            char Mypath[127]={0};
            strcpy(Mypath,currentpath);
            strcat(Mypath,name);
  
            memset(filebuffer,0,sizeof(filebuffer));
  
            cout<<Mypath<<endl;
  
            MultiByteToWideChar(CP_ACP, 0,Mypath, -1, szBuffer, 100);
  
  
  
  
            HANDLE hFile = CreateFile(szBuffer, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  
  
  
            if(hFile==INVALID_HANDLE_VALUE)
            {  
                printf("File:\t%s Not Found!\n", name);
            }  
            else  
            {   
  
                DWORD file_block_length = 0;  
  
                while(1)  
                {  
  
                    memset(filebuffer,0,BUFFER_SIZE);
                    ReadFile(hFile, reinterpret_cast<LPVOID>(&filebuffer), BUFFER_SIZE, &file_block_length, NULL);
  
  
                    printf("file_block_length = %d\n", file_block_length);  
  
  
  
                    if (send(*clientsocket, filebuffer, BUFFER_SIZE, 0) < 0)  
                    {  
                        printf("Send File:\t%s Failed!\n", name);  
                        break;  
                    }  
  
                    if (file_block_length < BUFFER_SIZE) {
                        memset(filebuffer,0,BUFFER_SIZE);
                        Sleep(20);
                        break;
                    }
  
                }  
 
  
                CloseHandle(hFile);
                printf("File:\t%s Transfer Finished!\n", name);  
            }
        }
        else if(buffer[0]=='2')//create new folder
        {
            char foldername[127]={0};
            strcpy(foldername,buffer+1);
            addFolder(currentpath,foldername);
            sendInfo(*clientsocket,currentpath);
            //printf("Create folder success!");
        }
        else if (buffer[0]=='3')
        {
            char file_name[FILE_NAME_MAX_SIZE]={0};
            char filepath[FILE_NAME_MAX_SIZE]={0};
            char filebuffer[BUFFER_SIZE]={0};
  
  
            strcpy(filepath,currentpath);
  
            int i=0;
  
  
            for(i=1;i<strlen(buffer);i++)
            {
                if(buffer[i]=='\n')
                {
                    file_name[i]='\0';
                    break;
                }
                file_name[i-1]=buffer[i];
            }
  
  
            strcat(filepath,file_name);
  
            TCHAR FILEPATH[512];
  
            MultiByteToWideChar(CP_ACP, 0,filepath, -1,FILEPATH , 512);
  
  
            HANDLE dfile=CreateFile(  
                FILEPATH,
                GENERIC_READ|GENERIC_WRITE,  
                0,  
                NULL,  
                CREATE_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);  

            long total=atol(buffer+i+1); 
            memset(filebuffer,0,BUFFER_SIZE); 
            long written = 0;
  
            while(written < total)  
            {  
				long length = recv(*clientsocket, filebuffer, BUFFER_SIZE, 0); 
				long needWrite = 0;
				if (length <= total - written)
					needWrite = length;
				else 
					needWrite = total - written;

                DWORD fileblocklen=0;
  
                WriteFile(dfile,filebuffer,needWrite,&fileblocklen,NULL); 
				written += needWrite;
				printf("write:%ld\n",fileblocklen);

                memset(filebuffer,0,BUFFER_SIZE); 
            }  
  
            CloseHandle(dfile);
            Sleep(20);
            sendInfo(*clientsocket,currentpath);
            printf("Receive File:\t %s From Server Finished!\n", file_name);
  
              
        }
          
          
  
        if(strcmp(buffer,"offline")==0) break;
        printf("***Client***    %s\n", buffer);
    }
  
    delete[] currentpath;
   
    closesocket(*clientsocket);
      
  
    return 0;
}
  
  
#define MAXCLIENTS 5           //宏定义，最多3个客户端连接
  
  
int main()
{
	
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
  
    HANDLE threads[MAXCLIENTS];
  
	string ip = GetLocalIpAddress();

    SOCKET s=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  
    sockaddr_in sockaddr;
    sockaddr.sin_family=PF_INET;
	sockaddr.sin_addr.S_un.S_addr=inet_addr(ip.c_str());
    sockaddr.sin_port=htons(2333);
    bind(s, (SOCKADDR*)&sockaddr, sizeof(SOCKADDR));
  
    listen(s, 1);
  
    printf("listening on port [%d].\n", 2333);
  
    int existingClientCount=0;
      
  
    SOCKET clientsocket[MAXCLIENTS+1];
    SOCKADDR clientAddr[MAXCLIENTS+1];
    while(TRUE)
    {
          
        int size=sizeof(SOCKADDR);
  
        int num=existingClientCount;
        clientsocket[num]=accept(s, &clientAddr[num], &size);
        printf("***SYS***    New client touched.\n");
          
        if(existingClientCount<MAXCLIENTS)       //判断是否已经超出最大连接数了
        {
            threads[num]=CreateThread(NULL, 0, ProcessClientRequests, &clientsocket[num], 0, NULL);  //启动新线程，并且将socket传入
        }
        else
        {
            char* msg="Exceeded Max incoming requests, will refused this connect!\r\n";
              
            send(clientsocket[num], msg, BUFFER_SIZE, NULL);       //发送拒绝连接消息给客户端
            printf("***SYS***    REFUSED.\n");
            closesocket(clientsocket[num]);                                     //释放资源
            break;
        }
        existingClientCount++;
    }
  
    printf("Maximize clients occurred for d%.\r\n", MAXCLIENTS);
  
    WaitForMultipleObjects(MAXCLIENTS, threads, TRUE, INFINITE);           //等待所有子线程，直到完成为止
  
    closesocket(s);
      
    for(int i=0;i<MAXCLIENTS; i++)
    {
        CloseHandle(threads[i]);                                           //清理线程资源
    }
  
    WSACleanup();
  
    printf("Cleared all.\r\n");
  
    getchar();
  
    exit(0);
}
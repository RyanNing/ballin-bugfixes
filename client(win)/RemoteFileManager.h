#ifndef REMOTEFILEMANAGER_H_INCLUDED
#define REMOTEFILEMANAGER_H_INCLUDED

#include <windows.h>
#include <winsock2.h>
#include <gtk/gtk.h>
#include <ctype.h>

#define MAX_FILES 2000
#define MAX_TIME 50
#define SIZE_LENGTH 20
#define BUFFER_SIZE 8192
#define PORT 2333
#define MAX_ITER 1000

char buffer[BUFFER_SIZE];

// WIN32_FIND_DATA files[MAX_FILES];
typedef struct _FILES {
    char cFileName[MAX_PATH];
    char time[MAX_TIME];
    char file_size[SIZE_LENGTH];
    __int64 file_total;
} FILES;
FILES files[MAX_FILES];
int file_count;

__int64 totalsize;
typedef struct _ProgressData {
    GtkWidget * parent;
    GtkWidget * window;
    GtkWidget * pbar;
    int timer;
    char filename[MAX_PATH];
    __int64 filesize;
    HANDLE hFile;
} ProgressData;

GtkListStore * store;
GtkWidget * lists;
GtkMenuShell * right_click;

SOCKET s;

enum {
    ICON,
    FILENAME,
    FILESIZE,
    LASTUPDATE,
    TYPE,
    FILEBYTES,
    COLUMN_N
};

char * _(const char * str) { return g_locale_to_utf8(str, -1, 0, 0, 0); }
int chDIMOF(const char * Array) { return sizeof(Array)/sizeof(Array[0]); }
void reset_file_data() { file_count = 0; ZeroMemory(files, sizeof(files)); }

BOOL IsChildDir(WIN32_FIND_DATA * lpFindData)
{
    return (((lpFindData->dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY)!=0)&&
        (lstrcmp(lpFindData->cFileName, TEXT("."))!=0)&&
        (lstrcmp(lpFindData->cFileName, TEXT(".."))!=0));
}

BOOL GetFileSizeEx(HANDLE hFile, PLARGE_INTEGER fileSize)
{
    typedef void(WINAPI * GETFILESIZEEX)(HANDLE, PLARGE_INTEGER);
    GETFILESIZEEX MyGetFileSizeEx;
    MyGetFileSizeEx = (GETFILESIZEEX)GetProcAddress(
        GetModuleHandle("kernel32.dll"),
        "GetFileSizeEx");
    if(MyGetFileSizeEx==NULL)
    {
		return FALSE;
    }
    MyGetFileSizeEx(hFile, fileSize);
    return TRUE;
}

GdkPixbuf * icon_type(char name[])
{
    int i = 0;
    for(i=(int)strlen(name);i>=0;i--)
    {
        if(name[i]=='.')
        {
            break;
        }
    }
    if(i==-1)
    {
        return gdk_pixbuf_new_from_file("./resources/text-x-preview.png", NULL);
    }
    char suffix[MAX_PATH] = {0};
    int j = 0;
    for(j=0;i<(int)strlen(name);i++)
    {
        suffix[j++] = name[i];
    }
    for(j=0;j<(int)strlen(suffix);j++)
    {
        suffix[j] = tolower(suffix[j]);
    }
    if(strcmp(suffix, ".jpg")==0||strcmp(suffix, ".png")==0||strcmp(suffix, ".bmp")==0)
    {
        return gdk_pixbuf_new_from_file("./resources/image-x-generic.png", NULL);
    }
    else if(strcmp(suffix, ".mkv")==0||strcmp(suffix, ".avi")==0||strcmp(suffix, ".mp4")==0)
    {
        return gdk_pixbuf_new_from_file("./resources/video-x-generic.png", NULL);
    }
    else if(strcmp(suffix, ".mp3")==0||strcmp(suffix, ".wav")==0)
    {
        return gdk_pixbuf_new_from_file("./resources/audio-x-generic.png", NULL);
    }
    else if(strcmp(suffix, ".exe")==0)
    {
        return gdk_pixbuf_new_from_file("./resources/application-x-executable.png", NULL);
    }
    else if(strcmp(suffix, ".rar")==0||strcmp(suffix, ".zip")==0)
    {
        return gdk_pixbuf_new_from_file("./resources/package-x-generic.png", NULL);
    }
    else
    {
        return gdk_pixbuf_new_from_file("./resources/text-x-preview.png", NULL);
    }
}

void FileTimeToTime_t(FILETIME ft, time_t * t)
{
    ULARGE_INTEGER ui;
    ui.LowPart = ft.dwLowDateTime;
    ui.HighPart = ft.dwHighDateTime;
    *t = ((__int64)(ui.QuadPart-116444736000000000)/10000000);
}

char * UnicodeToANSI(const wchar_t * str)
{
     char * result;
     int textlen;
     textlen = WideCharToMultiByte(CP_ACP, 0, str, -1, NULL, 0, NULL, NULL);
     result = (char *)malloc((textlen+1)*sizeof(char));
     memset(result, 0, sizeof(char)*(textlen+1));
     WideCharToMultiByte(CP_ACP, 0, str, -1, result, textlen, NULL, NULL);
     return result;
}

wchar_t * UTF8ToUnicode(const char * str)
{
     int textlen;
     wchar_t * result;
     textlen = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
     result = (wchar_t *)malloc((textlen+1)*sizeof(wchar_t));
     memset(result, 0, (textlen+1)*sizeof(wchar_t));
     MultiByteToWideChar(CP_UTF8, 0, str, -1, (LPWSTR)result, textlen);
     return result;
}

char * _U(const char * str)
{
     return UnicodeToANSI(UTF8ToUnicode(str));
}

void show_main_window(GtkWidget * widget, gpointer data);
GtkWidget * show_start_window();
int set_up_socket(const char * IP);
void action();
// void recv_file_message();

int set_up_socket(const char * IP)
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in sockaddr;
    sockaddr.sin_family = PF_INET;
    sockaddr.sin_addr.S_un.S_addr = inet_addr(IP);
    sockaddr.sin_port = htons(PORT);

    if(connect(s, (SOCKADDR *)&sockaddr, sizeof(SOCKADDR))==0)
    {
        return 1;
    }
    return 0;
}

int flag = 1;
void recv_file_message()
{
    memset(buffer, 0, sizeof(buffer));
    reset_file_data();
    flag = 1;
    // int k = 0;
    // while(k<MAX_ITER)
    // while(1)
    {
        // k ++;
        int length = 0;
        while(length<BUFFER_SIZE)
        {
            // printf("hit");
            Sleep(10);
            char temp[BUFFER_SIZE] = {0};
            length += recv(s, temp, BUFFER_SIZE-length, 0);
            strcat(buffer, temp);
            if(length<0)
            {
                length = 0;
                continue;
            }
        }
        buffer[strlen(buffer)] = '\0';
        if(strlen(buffer)<BUFFER_SIZE)
            flag = 0;
        printf("receive length: %d\n%s\n", length, buffer);
        /*if(length<=0)
        {
            int err = WSAGetLastError();
            if(err==WSAEWOULDBLOCK)
                ;// continue;
            else if(err==WSAETIMEDOUT||err==WSAENETDOWN)
                ;
            else
                break;
        }
        if(strlen(buffer)==0)
        // if(length==0)
        {
            break;
        }*/
        // strcpy(buffer, "1.mp3\n2\n3\n999.kk\n23\noo\n");
        unsigned int i;
        for(i=0;i<strlen(buffer);i++)
        {
            FILES temp;
            int pfileName = 0, ptime = 0, psize = 0;
            memset(temp.cFileName, 0, sizeof(temp.cFileName));
            memset(temp.file_size, 0, sizeof(temp.file_size));
            memset(temp.time, 0, sizeof(temp.time));
            int record_mode = 0;
            while(true)
            {
                // printf("**");
                if(i<strlen(buffer))
                {
                    if(buffer[i]=='\n')
                    {
                        record_mode ++;
                        i ++;
                        continue;
                    }
                    if(record_mode==3)
                    {
                        break;
                    }
                    if(record_mode==0)
                    {
                        temp.cFileName[pfileName++] = buffer[i];
                    }
                    else if(record_mode==1)
                    {
                        temp.time[ptime++] = buffer[i];
                    }
                    else if(record_mode==2)
                    {
                        temp.file_size[psize++] = buffer[i];
                    }
                    i ++;
                }
                else
                {
                    break;
                }
            }
            strcpy(files[file_count].cFileName, temp.cFileName);
            strcpy(files[file_count].time, temp.time);
            strcpy(files[file_count].file_size, temp.file_size);
            files[file_count].file_total = atoll(temp.file_size);
            // printf("file name : %s\n", _U(files[file_count].cFileName));
            file_count ++;
            i --;
        }
    }
}

void main_quit()
{
    memset(buffer, 0, sizeof(buffer));
    strcpy(buffer, "offline");
    send(s, buffer, BUFFER_SIZE, 0);
    gtk_main_quit();
}

double Int64ToDouble(__int64 in64)
{
    int flag=0;
    double d;
    if(in64 < 0)
    {//负数处理
        flag = 1;
        in64 = -in64;
    }
    d = (unsigned long)(in64 >> 32);
    // 直接运算 1<<32 会有数值溢出
    d *= (1<<16);
    d *= (1<<16);
    d += (unsigned long)(in64 & 0xFFFFFFFF);
    if(flag)
        d = -d;
    return d;
}

#endif // REMOTEFILEMANAGER_H_INCLUDED

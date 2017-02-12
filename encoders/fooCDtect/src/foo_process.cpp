#include <windows.h>
#include "Shlwapi.h"
#include "foo_process.h"
#include "messages.h"
#include "onetime.h"

#include <strsafe.h>

#define MB10 1048576

bool MySendMessage(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	if( !isProcessExist() ) // проверяем мьютекс
		return false;
	if( SendMessage(hWnd, msg, wParam, lParam) )
		return true;
	return false; // ошибка, всегда нужно отвечать не нулём
}

bool SendFileName(HWND hWnd, LPCWSTR str, int pos)
{
	static wchar_t crpath[2000];
	GetCurrentDirectory(MAX_PATH, crpath);
	StringCchCat(crpath, 2000, L"\\");
	StringCchCat(crpath, 2000, str);

	COPYDATASTRUCT ds;
	ds.dwData = 1;
	ds.cbData = ( lstrlen(crpath) + 1 ) * sizeof(wchar_t);
	ds.lpData = (PVOID)crpath; 
	return MySendMessage(hWnd, WM_COPYDATA, pos, (LPARAM)&ds );
}

bool SendStdOut(HWND hWnd, const char* str, int pos)
{
	static wchar_t data[2000];
	int len = lstrlenA(str);
	MultiByteToWideChar(CP_ACP, 0, str, len, data, len);
	data[len] = L'\0';

	COPYDATASTRUCT ds;
	ds.dwData = 2;
	ds.cbData = ( len + 1 ) * sizeof(wchar_t);
	ds.lpData = (PVOID)data; 
	bool rez = MySendMessage(hWnd, WM_COPYDATA, pos, (LPARAM)&ds);
	return rez;
}


bool ReadAndCreateFile(LPCWSTR fname)
{
	HANDLE hFile = CreateFile(fname,FILE_ALL_ACCESS,NULL,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_TEMPORARY,NULL);
	if(hFile == INVALID_HANDLE_VALUE){
		MessageBox(0, L"Error creating file. Check foocdtect parameters", NULL, MB_OK);
		return false;
	}

	int stop = 1;
	DWORD D, D2;
	char *buf = new char[MB10];
	HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);

	while( stop )
	{
		int rez = ReadFile(hStdin,buf,MB10,&D,NULL);
		if(rez == 0) stop = 0;

		if( 0 == WriteFile(hFile, buf, D, &D2, NULL) )
		{
			delete [] buf;
			CloseHandle(hFile);
			DeleteFile(fname);
			MessageBox(0, L"WriteFile error", NULL, MB_OK);
			return false;
		}
	}
	delete [] buf;
	CloseHandle(hFile);
	return true;
}

DWORD CorrectHeader(LPCWSTR fname)
{
	HANDLE hFile = CreateFile(fname, FILE_ALL_ACCESS, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return 0;
	DWORD D, all_size = GetFileSize(hFile, NULL);
	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	TitleWave head;
	ReadFile(hFile, &head, sizeof(TitleWave), &D, NULL);

	if( head.type == 1 &&
		head.channels == 2 &&
		head.freq == 44100 &&
		head.bits == 16) // всё хорошо
	{
		head.len_riff = all_size - 8;
		head.len_data = all_size - 44;
		SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
		WriteFile(hFile, &head, sizeof(TitleWave), &D, NULL);
		CloseHandle(hFile);
		return head.len_data;
	}
	CloseHandle(hFile);
	return 0;
}
int ParsePercentString(const char* str)
{
	// Data analysis:          [46%]
	char dig[10]; int pos=0,i;
	if(lstrcmpA(str,"analysis")){
		for( i=0; i < (int)lstrlenA(str); i++ )
			if( str[i] >= 48 && str[i] <= 57 )
				dig[pos++] = str[i];
		dig[pos] = 0;
		pos = atoi(dig);
		return pos;
	}
	return 0;
}

bool RunProcessing(LPCWSTR fname, int mode, HWND hWnd, int curtrack)
{
	SECURITY_ATTRIBUTES sa;
	STARTUPINFO si;
	PROCESS_INFORMATION pi; 
	HANDLE hReadError, hReadOut, hWriteError, hWriteOut;
	char xxx[1000],tempbuf[1000]; DWORD dd; int i,pos=0;

	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	if (!CreatePipe(&hReadOut, &hWriteOut, &sa, 0)) return 1;
	if (!CreatePipe(&hReadError, &hWriteError, &sa, 0)) return 1;

	memset(&si, 0, sizeof(si));
	memset(&pi, 0, sizeof(pi));

	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = NULL;
	si.hStdOutput = hWriteOut;
	si.hStdError = hWriteError;

	//////////////////////////////////////////////////////////////////////////
	// запуск процесса
	wchar_t cmdline[1000];
	StringCchPrintf(cmdline, 1000, L"aucdtect.exe -m%d -v \"%s\"", mode, fname);
	
	if (!CreateProcess(NULL,cmdline, NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, L".", &si, &pi))
	{
		MessageBox(0, L"can not launch aucdtect.exe", NULL, MB_OK);
		return false;
	}
	CloseHandle(hWriteOut);
	CloseHandle(hWriteError);
	int oldpers = -1;

	SendMessage(hWnd, MES_SENDPROCID1, curtrack, GetProcessId(GetCurrentProcess()));
	SendMessage(hWnd, MES_SENDPROCID2, curtrack, pi.dwProcessId);

	while(ReadFile(hReadError,xxx,80,&dd,0))
	{
		for(i=0;i<(int)dd;i++){
			if(xxx[i]==13 || xxx[i]==10){
				if(xxx[i]==13)i++;
				tempbuf[pos] = 0;
				int pers = ParsePercentString(tempbuf);
				// сообщение
				if( pers != oldpers){
					if( !MySendMessage(hWnd, MES_PROGRESS, curtrack, pers) )
						return false;
					oldpers = pers;
				}
				pos=0;
			}else{
				if(xxx[i]==10)continue;
				tempbuf[pos++] = xxx[i]; 
			}
		}
	}
	pos=0;
	while(ReadFile(hReadOut,xxx,80,&dd,0))
	{
		for(int i=0;i<(int)dd;i++){
			if(xxx[i]==13 || xxx[i]==10){
				if(xxx[i]==13)i++;
				tempbuf[pos] = 0;
				StringCchCatA(tempbuf, 1000, "\r\n");
				pos+=2;
			}else{
				if(xxx[i]==10)continue;
				tempbuf[pos++] = xxx[i]; 
			}
		}
	}
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	if( !SendStdOut(hWnd, tempbuf, curtrack) )
		return false;

	return true;
}

bool TrimFile(LPCWSTR fname, int curtrack)
{ 
	HANDLE hFile = CreateFile(fname, FILE_ALL_ACCESS,NULL,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return false;
	DWORD D;
	WriteFile(hFile, &curtrack, sizeof(int), &D, NULL);
	CloseHandle(hFile);
	return true;
}


int GoProcessing(const tagCMDL & cmdl)
{
	int mode = cmdl.mode;

	CreateDirectory(cmdl.temppath, NULL);

	wchar_t tempfile[MAX_PATH];
	if( lstrlen(cmdl.temppath) == 0 )
	{
		// кодируем в файл по умолчанию
		StringCchCopy(tempfile, MAX_PATH, cmdl.outputfilename);
	}else
	{
		StringCchCopy(tempfile, MAX_PATH, cmdl.temppath);
		if( tempfile[ lstrlen(tempfile) -1 ] != L'\\')
			StringCchCat(tempfile, MAX_PATH, L"\\");
		StringCchCat(tempfile, MAX_PATH, PathFindFileName(cmdl.outputfilename));
	}


	HWND fwwnd = FindWindow(wndclassnameW, NULL);
	if(fwwnd == NULL) return 4;

	int pos = -1;
	while( pos < 0 )
	{
		if( !isProcessExist() ) // проверяем мьютекс
			return 0; // преждевременный выход
		pos = (int)SendMessage(fwwnd, MES_ONNEWTHREAD, 0, 0);
		if( pos >= 0 ) break;
		Sleep(200);
	}

	//////////////////////////////////////////////////////////////////////////
	// название файла
	if( !SendFileName(fwwnd, cmdl.outputfilename, pos) )
		return 0; // выход, лажа.

	//////////////////////////////////////////////////////////////////////////
	// читаем файл из stdin и создаём временный wav-файл
	if( !ReadAndCreateFile(tempfile) )
		return 5;
	DWORD len = CorrectHeader(tempfile);
	//////////////////////////////////////////////////////////////////////////
	// сообщаем длинну трэка: sec * 4 * 44100
	if( !MySendMessage(fwwnd, MES_SENDLENGTH, pos, len) )
	{
		DeleteFile(tempfile);
		return 0;
	}
	if( len )
	{
		UINT newpriority = (UINT)SendMessage(fwwnd, MES_GETPRIORITY, 0, 0);
		SetPriorityClass(GetCurrentProcess(), newpriority );

		if( !RunProcessing(tempfile, mode, fwwnd, pos) )
		{
			DeleteFile(tempfile);
			return 0; // пытаемся безболезненно свалить :D
		}
	}
	DeleteFile(tempfile);
	if( !TrimFile(cmdl.outputfilename, pos) )
		return 6;

	return 0;
}
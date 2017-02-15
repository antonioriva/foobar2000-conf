#include <windows.h>
#include "Shlwapi.h"

#include "onetime.h"
#include "MainWnd.h"
#include "foo_process.h"

#include <strsafe.h>

int  ParceCommandLine(int argc, LPWSTR argv[], tagCMDL& cmdl);
int  RunProcess(LPCWSTR image);
void WaitProcess();

const wchar_t *lpHelpString = {
	L"fooCDtect2 by baralgin, ver. 2.1 (12.09.2010)\n"
	L"\n"
	L"Switches:\n"
	L"--output <filename> \ttemp file name (this option is necessary)\n"
	L"--threads <integer> \tset maximum number active threads (max 8), default [0] - unlimited\n"
	L"--mode <integer> \t\tdetect mode [0..40], default 0,\n"
	L"\t\t\t0 - slow and most accurate, 40 - fast, but less accurate\n"
	L"--tpath <path> \t\ttemporary folder for wav-files (example: --tpath \"d:\\temp\")\n"
	L"--priority <0|1|2|3>\tchecking priority (0 - use converter settings, 1 - idle, 2 - below normal, 3 - normal)\n"
	L"\n"
	L"--lfor <1|2|3>\t\tlog format (1 - simple, 2 - normal, 3 - verbose)\n"
	L"--lenc <1|2|3>\t\tdefault log encoding (1 - ansi, 2 - utf16, 3 - utf8)\n"
	L"--autodel \t\t\tautodelete \"saved\" tracks\n"
	L"\n"
	L"foobar2000, converter settings:\n"
	L"\n"
	L"Encoder: fooCDtect2.exe\n"
	L"Extension: aucdtect\n"
	L"Parameters(for example):\n"
	L"\t--output %d --threads 2 --mode 0 --tpath \"R:\\temp\" --lfor 3 --lenc 1\n"
	L"or simply: --output %d\n"
	L"Bit Depth Control: Format is lossless(or hybrid)\n"
	L"And don't forget to disable dither."
};

int wmain(int argc, LPWSTR argv[])
{
	tagCMDL cmdl;
	ZeroMemory(&cmdl, sizeof(tagCMDL) );
	if( int rez = ParceCommandLine(argc, argv, cmdl) ) 
	{
		MessageBox(0, lpHelpString, L"fooCDtect", MB_OK);
		return rez; // выход, неправильные параметры
	}

	if( cmdl.isGUI )
	{
		if( isProcessRunnig() ) // запуск всего один раз.
			return 0;

		MainWnd wnd( cmdl );
		wnd.Run();

		//CloseMutex();
		return 0; // выход из оконной программы	
	}

	if( !isProcessExist() ) // запускаем процесс
	{
		//MessageBox(0, cmdl.imagepath, NULL, MB_OK);
		RunProcess(cmdl.imagepath);
	}
	WaitProcess();
	
	GoProcessing(cmdl);
	
	return 0;
}

int getopt(int argc, LPWSTR argv[], LPCWSTR opt)
{
	for( int i = 0; i < argc; i++ )
	{
		if( !lstrcmp(argv[i],opt) )
		{
			if( (i + 1) < argc ) 
				return (i + 1);
			else 
				return -1; // тоесть нашли, но продолжени€ нет
		}
	}
	return 0; // не нашли такого параметра
}
int getopt_int(int argc, LPWSTR argv[], LPCWSTR opt, int minval, int maxval, int defval)
{
	int o = getopt(argc, argv, opt);
	if( o <= 0 ) return defval;
	int val = _wtoi(argv[o]);
	if( val >= minval && val <= maxval ) return val;
	return defval;
}
bool getopt_bool(int argc, LPWSTR argv[], LPCWSTR opt)
{
	int o = getopt(argc, argv, opt);
	if( o != 0 ) 
		return true;
	else 
		return false;
}

int ParceCommandLine(int argc, LPWSTR argv[], tagCMDL& cmdl)
{
	int opt;

	// гуишный процесс или нет
	cmdl.isGUI = getopt_bool(argc, argv, L"--gui");

	// вы€сн€ем сколько потоков
	cmdl.threads = getopt_int(argc, argv, L"--threads", 0, 8, 0);

	// провер€ем gui или процесс проверки
	cmdl.mode = getopt_int(argc, argv, L"--mode", 0, 40, 0);

	// автоудаление того чего сохранили
	cmdl.autodel = getopt_bool(argc, argv, L"--autodel");

	// ищем им€ выходного файла
	opt = getopt(argc, argv, L"--output");
	if( opt > 0 )
	{
		if( argv[opt][0] == L'-' && argv[opt][1] == L'-' )
		return 1; // ошибка, веро€тно пропущено значение параметра
		StringCchCopy(cmdl.outputfilename, MAX_PATH, argv[opt]);
	}
	else 
		return 1; // ошибка
//////////////////////////////////////////////////////////////////////////
	// расширенный лог
	//cmdl.verlog = getopt_bool(argc, argv, L"--verlog");

	cmdl.lformat = getopt_int(argc, argv, L"--lfor", 1, 3, 2);

	cmdl.lencoding = getopt_int(argc, argv, L"--lenc", 1, 3, 3);

	cmdl.priority = getopt_int(argc, argv, L"--priority", 0, 3, 0);

	opt = getopt(argc, argv, L"--tpath");
	if( opt > 0 )
	{
		if( argv[opt][0] == L'-' && argv[opt][1] == L'-' )
			StringCchCopy(cmdl.temppath, MAX_PATH, L""); // ошибка, веро€тно пропущено значение параметра
		else
			StringCchCopy(cmdl.temppath, MAX_PATH, argv[opt]);
	}
	else 
		StringCchCopy(cmdl.temppath, MAX_PATH, L"");

	// узнаЄм им€ процесса
	GetModuleFileName(NULL, cmdl.imagepath, MAX_PATH);

	return 0;
}

int  RunProcess(LPCWSTR image)
{
	
	wchar_t newline[2000];
	StringCchCopy(newline, 2000, image);
	StringCchCat(newline, 2000, L" --gui ");
	StringCchCat(newline, 2000, GetCommandLine());
	

	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	memset(&si, 0, sizeof(si));
	si.cb = sizeof(si);

	if( !CreateProcess(NULL, newline, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi) )
	{
		return 3;
	}
	CloseHandle( pi.hProcess );
	CloseHandle( pi.hThread );

	return 0;
}

void WaitProcess()
{
	HANDLE hMut = NULL;
	do{
		Sleep(50);
		hMut = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MUTNAME);
	}while( hMut == NULL );
	WaitForSingleObject(hMut, INFINITE);
	ReleaseMutex(hMut);
	CloseHandle(hMut);
}










#define _USE_MATH_DEFINES
#define _WIN32_WINNT 0x500

#include <windows.h>
#include <CommCtrl.h>
#include <Shlwapi.h>
#include <math.h>
#include <psapi.h>


#include "onetime.h"
#include "messages.h"
#include "MainWnd.h"
#include "resource.h"
#include "foo_process.h"

#include <strsafe.h>

extern const wchar_t *lpHelpString;

const unsigned char BOM_UTF16LE[] = { 0xFF, 0xFE };
const unsigned char BOM_UTF8[] = { 0xEF, 0xBB, 0xBF };

const wchar_t foobar2000Class[] = L"{97E27FAA-C0B3-4b8e-A693-ED7881E99FC1}";

enum { LF_SIMPLE = 1, LF_NORMAL, LF_VERBOSE };
enum { LE_ANSI = 1, LE_UCS2, LE_UTF8 };

const int cvPriority[] = 
{ 
	IDLE_PRIORITY_CLASS, 
	BELOW_NORMAL_PRIORITY_CLASS, 
	NORMAL_PRIORITY_CLASS
};

const int cvLog_format[] = 
{ 
	ID_LOGFORMAT_SIMPLE, 
	ID_LOGFORMAT_NORMAL, 
	ID_LOGFORMAT_VERBOSE 
};
const int cvLog_encoding[] = 
{ 
	ID_LOGENCODING_ANSI, 
	ID_LOGENCODING_UCS2, 
	ID_LOGENCODING_UTF8
};

/////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK MainWnd::WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	LONG udata;
	udata = GetWindowLong(hwnd, GWL_USERDATA);
	MainWnd *wnd = reinterpret_cast<MainWnd*> (udata);
	return wnd->ClassProc(hwnd, message, wParam, lParam);
}
/////////////////////////////////////////////////////////////////////////////////
LRESULT MainWnd::ClassProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int item;
	switch(message)
	{
	case WM_COMMAND:
		{
			int wmId, wmEvent;
			wmId    = LOWORD(wParam); 
			wmEvent = HIWORD(wParam); 
			switch (wmId){
				case ID_EDIT_SORT: // потом вообще убрать/переименовать
					SortListByDirectory();
					break;
				case ID_EDIT_SELALL:
					SelectAll();
					break;
				case ID_EDIT_INVSEL:
					InvertSelection();
					break;
				case ID_FILE_EXIT:
					SendMessage(hwnd, WM_CLOSE, 0 , 0);
					break;
				case ID_FILE_SAVEALL:
					SaveAll();
					break;
				case ID_FILE_SAVESEL:
					SaveSelected();
					break;
				case ID_OPTIONS_AUTODEL:
					autodel = !autodel;
					CheckMenuItem(GetMenu(hWnd),ID_OPTIONS_AUTODEL, autodel ? MF_CHECKED : MF_UNCHECKED);
					break;
				case ID_HELP_ABOUT:
					MessageBox(hWnd, lpHelpString, L"fooCDtect", MB_OK);
					break;
				case ID_EDIT_DELSEL:
					DeleteTracks();
					break;
				case ID_FILE_AUTOMATICSAVE:
					AutoSaveAll();
					break;
				case ID_FILE_STATS:
					ShowStats();
					break;
				case ID_LOGFORMAT_SIMPLE:
				case ID_LOGFORMAT_NORMAL:
				case ID_LOGFORMAT_VERBOSE:
					ChangeMenuRadioCheck(wmId, cvLog_format);
					break;
				case ID_LOGENCODING_ANSI:
				case ID_LOGENCODING_UCS2:
				case ID_LOGENCODING_UTF8:
					ChangeMenuRadioCheck(wmId, cvLog_encoding);
					break;
			}
		}
		SetWinTitle();
		return (FALSE);
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_CLOSE:
		OnClose();
		break;
	case WM_SIZE:
		return OnSize();
	case WM_TIMER:
		GetNames();
		break;
	case WM_NOTIFY:
	{
		LPNMHDR pMHDR = (LPNMHDR)lParam;
		switch( pMHDR->idFrom )
		{
			case IDC_LISTVIEW:
				switch( pMHDR->code ){
					case NM_DBLCLK:
						OnListDbClick(lParam);
						break;
					case NM_RCLICK:
						OnListContextMenu(lParam);
						break;
					case LVN_ITEMCHANGED:
						OnListItemChanged(lParam);
						break;
					case NM_CUSTOMDRAW:
						return OnLVCustomDraw(lParam);
				}
				break;
			case IDC_STATUSBAR:
				switch( pMHDR->code ){
					case NM_DBLCLK:
						if( ((LPNMMOUSE)lParam)->dwItemSpec == 2 )
							StatusBarDBClick();
						break;
					case NM_RCLICK:
						if( ((LPNMMOUSE)lParam)->dwItemSpec == 0 )
							StatusBarChangePriority();
						break;
				}
				break;
		}
	}
		break;
	case MES_ONNEWTHREAD:
		return OnNewThread();
	case WM_COPYDATA:
		OnCopyData( (int)wParam, (PCOPYDATASTRUCT)lParam );
		return TRUE;
	case MES_SENDLENGTH:
		OnLength((int)wParam, (DWORD)lParam);
		return 1;
	case MES_PROGRESS:
		OnProgress((int)wParam, (int)lParam);
		return 1;
	case MES_GETPRIORITY:
		return GetPriorityClass( GetCurrentProcess() );
	case MES_SENDPROCID1:
		item = GetPosByIndex( (int)wParam );
		if( item == -1 ) return 0;
		tracks[item].dwProcId1 = (DWORD)lParam;
		break;
	case MES_SENDPROCID2:
		item = GetPosByIndex( (int)wParam );
		if( item == -1 ) return 0;
		tracks[item].dwProcId2 = (DWORD)lParam;
		break;
	default:
		return DefWindowProc(hwnd, message, wParam, lParam);
	}
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////
MainWnd::MainWnd(tagCMDL cmdl)
{
	max_thr = cmdl.threads;
	cur_thr = 0; 
	index = 0;
	(*file_ext) = L'\0';
	m_mode = cmdl.mode;
	shell_icon = NULL;

	autodel	= cmdl.autodel;

	lformat = cmdl.lformat;
	lencoding = cmdl.lencoding;
	priority = cmdl.priority;

	tFl = false;
	tDC = NULL;
	tFont = NULL;
	tBrush = NULL;
	StringCchCopy(m_temppath, MAX_PATH, cmdl.temppath);
	if( cmdl.priority > 0 )
		SetPriorityClass( GetCurrentProcess(), cvPriority[ cmdl.priority - 1 ] );
}
void MainWnd::OnClose()
{
	if( cur_thr )
	{
		int rez = MessageBox(hWnd, L"Some threads are still active, are you sure you want to terminate application?",
						  		   L"fooCDtect", MB_YESNO);
		if( rez != IDYES ) return;
	}
	DestroyWindow(hWnd);
}
void MainWnd::MyRegisterClass()
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX); 

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= (WNDPROC)WndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= NULL;
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= NULL; //(HBRUSH)(COLOR_BACKGROUND);
	wcex.lpszMenuName	= (LPCTSTR)IDC_FOOCDTECT;
	wcex.lpszClassName	= wndclassnameW;
	wcex.hIconSm		= NULL;

	RegisterClassEx(&wcex);
}

void MainWnd::CreateMainWnd()
{
	hInstance = GetModuleHandle(NULL);
	MyRegisterClass();
	RECT	rect;
	GetClientRect(GetDesktopWindow(),&rect);
	hWnd = CreateWindow(
			wndclassnameW,
			L"fooCDtect",
			WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
			(rect.right-_SIZE_X_)/2,
			(rect.bottom-_SIZE_Y_)/2,
			_SIZE_X_,
			_SIZE_Y_,
			FindWindow(foobar2000Class, NULL),
			NULL,
			hInstance,
			NULL
		);
	SetWindowLong(hWnd, GWL_USERDATA, reinterpret_cast<LONG> (this) );
	hStatus = CreateStatusWindow(WS_CHILD | WS_VISIBLE | SBARS_SIZEGRIP | SBT_TOOLTIPS,
		L"", hWnd, IDC_STATUSBAR);

	int parts[3] = { 35, 90, -1 };
	SendMessage(hStatus, SB_SETPARTS, 3, (LPARAM)parts );

	SetStatusText1();
	SendMessage(hStatus, SB_SETTIPTEXT, 0, (LPARAM)L"Threads. active : maximum. maximum = 0 - unlimited, auto" );
	SetStatusText2();
	SendMessage(hStatus, SB_SETTIPTEXT, 1, (LPARAM)L"detect mode [0..40], default 0, 0 - slow and most accurate, 40 - fast, but less accurate");

	
	if( ExtractIconEx(L"shell32.dll", 4, NULL, &shell_icon, 1) == 1 )
		SendMessage(hStatus, SB_SETICON, 2, (LPARAM)shell_icon );
	
	CreateListView();
	hAccel = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR) );

	CheckMenuItem(GetMenu(hWnd),ID_OPTIONS_AUTODEL, autodel ? MF_CHECKED : MF_UNCHECKED);

	ChangeMenuRadioCheck(cvLog_format[ lformat - 1 ], cvLog_format);
	ChangeMenuRadioCheck(cvLog_encoding[ lencoding - 1 ], cvLog_encoding);
	SetWinTitle();
}

void MainWnd::SetStatusText1()
{
	wchar_t str[50];
	StringCchPrintf(str, 50, L"%2d : %2d           ", cur_thr, max_thr);
	SendMessage(hStatus, SB_SETTEXT, 0, (LPARAM)str );
}

void MainWnd::SetStatusText2()
{
	wchar_t str[20];
	StringCchPrintf(str, 20, L"mode=%d       ", m_mode);
	SendMessage(hStatus, SB_SETTEXT, 1, (LPARAM)str );
}

void MainWnd::SetStatusText3(const wchar_t *str)
{
	SendMessage(hStatus, SB_SETTEXT, 2, (LPARAM)str );
	SendMessage(hStatus, SB_SETTIPTEXT, 2, (LPARAM)str );

	RECT rect;
	GetClientRect(hWnd,&rect);
	SetWindowPos(hStatus,0,0,rect.bottom-20,rect.right,rect.bottom,SWP_NOMOVE);
}

void MainWnd::StatusBarDBClick()
{
	wchar_t path[MAX_PATH];
	SendMessage(hStatus, SB_GETTEXT, 2, (LPARAM)path);
	if( SafeStrLen(path, MAX_PATH) )
		ShellExecute(hWnd, L"open", path, NULL, NULL, SW_SHOWNORMAL);
}

void MainWnd::CreateListView()
{
	LVCOLUMN col;
	hList = CreateWindow(WC_LISTVIEW,L"",
		WS_CHILD | WS_VISIBLE | WS_BORDER | WS_CLIPCHILDREN |
		WS_CLIPSIBLINGS| LVS_REPORT | LVS_NOSORTHEADER | LVS_SHOWSELALWAYS,
		0, 0, 50, 50, hWnd, (HMENU)IDC_LISTVIEW, hInstance, NULL );
	ListView_SetExtendedListViewStyle(hList,
		LVS_EX_FULLROWSELECT
//		LVS_EX_GRIDLINES
		);
	col.mask = LVCF_TEXT | LVCF_FMT | LVCF_WIDTH;
	col.pszText = L"Track name";
	col.fmt = LVCFMT_LEFT;
	col.cx = 35;
	ListView_InsertColumn(hList,0,&col);

	col.pszText = L"%";
	col.fmt = LVCFMT_CENTER;
	col.cx = 150;
	ListView_InsertColumn(hList,1,&col);

	col.pszText = L"Length";
	col.fmt = LVCFMT_CENTER;
	col.cx = 150;
	ListView_InsertColumn(hList,2,&col);

	col.pszText = L"Type";
	col.fmt = LVCFMT_CENTER;
	ListView_InsertColumn(hList,3,&col);
}

void MainWnd::SelectAll()
{
	int count = ListView_GetItemCount(hList);
	for( int i = 0; i < count; i++ )
		ListView_SetItemState(hList, i, LVIS_SELECTED, LVIS_SELECTED);
	SetFocus(hList);
}

void MainWnd::InvertSelection()
{
	int count = ListView_GetItemCount(hList);
	for( int i = 0; i < count; i++ )
	{
		UINT state = ListView_GetItemState(hList, i, LVIS_SELECTED);
		if( state & LVIS_SELECTED )
		{
			ListView_SetItemState(hList, i, 0, LVIS_SELECTED);
		}
		else
		{
			ListView_SetItemState(hList, i, LVIS_SELECTED, LVIS_SELECTED);
		}
	}
	SetFocus(hList);
}

LRESULT MainWnd::OnSize()
{
	RECT rect;
	GetClientRect(hWnd,&rect);
	SetWindowPos(hStatus,0,0,rect.bottom-20,rect.right,rect.bottom,SWP_NOMOVE);

	SetWindowPos(hList,0,0,0,rect.right,rect.bottom-20,SWP_NOMOVE);
	GetClientRect(hList,&rect);
	ListView_SetColumnWidth(hList,0,( rect.right ) * 0.55);
	ListView_SetColumnWidth(hList,1,( rect.right ) * 0.15);
	ListView_SetColumnWidth(hList,2,( rect.right ) * 0.15);
	ListView_SetColumnWidth(hList,3,( rect.right ) * 0.15);

	return 0;
}
void MainWnd::Run()
{
	CreateMainWnd();
	ShowWindow(hWnd, SW_SHOW);

	MReleaseMutex(); // сообщаем, что программа готова принимать собщения

	SetTimer(hWnd, 100, 2000, NULL);
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) 
	{
		if( !TranslateAccelerator(hWnd, hAccel, &msg) )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	} 
}

void MainWnd::ErrorMessageBox(const wchar_t *str)
{
	MessageBox(hWnd, str, NULL, MB_OK);
}

LRESULT MainWnd::OnNewThread()
{
	if( ( max_thr != 0 ) && ( cur_thr >= max_thr ) )	
		return -1; // типа не разрешаем
	
	cur_thr++;
	SetStatusText1();
	LISTITEM newitem;
	newitem.idx = index;
	tracks.push_back(newitem);
	AddListItem(L"waiting");
	ListView_Scroll(hList, 0, 10);
	index++;
	return ( index - 1 );
}

void MainWnd::OnCopyData(int item_idx, PCOPYDATASTRUCT lpSt)
{
	int item = GetPosByIndex(item_idx);
	if( item == -1 ) return;
	
	wchar_t temp[LENSTDOUT];
	StringCchCopy(temp, LENSTDOUT, static_cast<wchar_t*> (lpSt->lpData));

	switch( static_cast<int> (lpSt->dwData) )
	{
	case 1:{
			StringCchCopy(tracks[item].FileName, MAX_PATH, temp);
			SetItemFileName(item, temp);
			if( SafeStrLen(file_ext, MAX_PATH) == 0 )
				StringCchPrintf(file_ext, MAX_PATH, L"*%s", PathFindExtension(temp) );
		   }
		break;
	case 2:
		StringCchCopy(tracks[item].StdOut, 2000, temp);
		SetRez(temp, item);
		break;
	}
}
//////////////////////////////////////////////////////////////////////////
void MainWnd::SetItemFileName(int pos, const wchar_t* fname)
{
	wchar_t temp[MAX_PATH];
	PathFindFilenameNoExt(fname, temp, MAX_PATH);
	SetItemText(pos, 0, temp);
}

void MainWnd::AddListItem(const wchar_t* prog)
{
	int col = (int) SendMessage(hList, LVM_GETITEMCOUNT, 0, 0L);
	LVITEM item;
	memset(&item,0,sizeof(LVITEM));
	item.mask = LVIF_TEXT;
	item.iItem = col;
	item.pszText = L"...";
	ListView_InsertItem(hList,&item);
	SetItemText(col,1,(LPTSTR)prog);
	this->OnSize();
}

int MainWnd::GetPosByIndex(int idx)
{
	for( int i = 0; i < (int)tracks.size(); i++ )
	{
		if( tracks[i].idx == idx ) return i;
	}
	return -1; // не нашли
}

void MainWnd::OnLength(int item_idx, DWORD length)
{

	int item = GetPosByIndex(item_idx);
	if( item == -1 ) return;

	if( length)
	{
		tracks[item].len = length;
		SetItemLenght(item, length);
		SetItemText(item, 1, L"0");
	}else
	{
		tracks[item].rez_type = 4; // лажа
		StringCchCopy(tracks[item].StdOut, LENSTDOUT, 
			L"Invalid track format, not CDDA. Need 2ch, 16bits, 44100Hz\r\n");
		tracks[item].progress = 100;
		tracks[item].len = 0;

		SetItemProgress(item, 100);
		SetItemLenght(item, 0);
		SetItemResult(item, 4, L"");
		cur_thr--;
		SetStatusText1();
	}
}
void MainWnd::SetItemLenght(int pos, DWORD len)
{
	wchar_t str[100];
	int m  = len / 44100 / 60 / 4;
	double s  = len / 44100.0 / 4.0 - m * 60;
	StringCchPrintf(str, 100, L"%d:%02d.%02d", m, (int)s, (len / 588 / 4) % 75 );
	if(len)
		SetItemText(pos, 2, str);
}
void MainWnd::OnProgress(int item_idx, int pos)
{
	int item = GetPosByIndex(item_idx);
	if( item == -1 ) return;

	tracks[item].progress = pos;
	SetItemProgress(item, pos);
}
void MainWnd::SetItemProgress(int pos, int protz)
{
	if( protz == -1 )
		SetItemText(pos, 1, L"waiting");
	else
	{
		static const size_t strsize = 20;
		wchar_t str[strsize];
		_itow_s(protz, str, strsize, 10);
		SetItemText(pos, 1, str);
	}
}
void MainWnd::OnListDbClick(LPARAM lParam)
{
	int pos = ((LPNMITEMACTIVATE)lParam)->iItem;
	if( pos < 0 ) return;
	if( lstrlen(tracks[pos].StdOut) == 0 ) return;
	MessageBox(hWnd, tracks[pos].StdOut,L"StdOut",MB_OK);
	SetFocus(hList);
}

void MainWnd::SetRez(wchar_t *str, int item)
{
	if( wcsstr(str,L"CDDA") ) tracks[item].rez_type = 1;
	else
		if( wcsstr(str,L"MPEG") ) tracks[item].rez_type = 2;
		else
			tracks[item].rez_type = 3;
	if(tracks[item].rez_type != 3){
		wchar_t *pos = wcschr(str, L'%');
		if( pos == NULL ) return; // не может такого быть :) 

		tracks[item].rez_prob[4] = L'\0';
		tracks[item].rez_prob[3] = *(pos--);
		tracks[item].rez_prob[2] = *(pos--);
		tracks[item].rez_prob[1] = *(pos--);
		tracks[item].rez_prob[0] = *(pos--);
	}
	SetItemResult(item, tracks[item].rez_type, tracks[item].rez_prob);
	cur_thr--;
	SetStatusText1();
}

void MainWnd::SetItemResult(int pos, int rez_type, const wchar_t* prob)
{
	wchar_t rezstr[100];
	switch( rez_type )
	{
		case -1:
		case  0:
			SetItemText(pos, 3, L"");
			return;
		case 1:
			StringCchPrintf(rezstr, 100, L"CDDA-%s",prob);
			break;
		case 2:
			StringCchPrintf(rezstr, 100, L"MPEG-%s",prob);
			break;
		case 3:
			SetItemText(pos, 3, L"Unknown");
			return;
		case 4:
			SetItemText(pos, 3, L"Error");
			return;
	}
	SetItemText(pos, 3, rezstr);
}

bool MainWnd::isFileNameTemp(const wchar_t *fname)
{
	wchar_t tname[MAX_PATH], *fs;
	StringCchCopy(tname, MAX_PATH, fname);
	fs = PathFindExtension(tname);
	(*fs) = L'\0';
	if( SafeStrLen(tname, MAX_PATH) != 37 ) return false;
	if( 
		tname[0] == L't' &&
		tname[1] == L'e' &&
		tname[2] == L'm' &&
		tname[3] == L'p' &&
		tname[4] == L'-' ) return true;

	return false;
}

void MainWnd::GetNames()
{
	wchar_t stemp[MAX_PATH], fname[MAX_PATH];
	for( int i = 0; i < (int)tracks.size(); i++ )
	{
		StringCchCopy(stemp, MAX_PATH, tracks[i].FileName);
		
		// простейшая проверка на "правильность" имени .
		// дабы избежать лишних действий
		wchar_t *st = PathFindFileName(stemp);
		if( (tracks[i].rez_type == 0) || !isFileNameTemp(st) ) continue;  

		(*st) = L'\0';
		if( SafeStrLen(stemp, MAX_PATH) )
		{
			if( FindName(stemp, tracks[i].idx, fname) )
			{
				StringCchCat(stemp, MAX_PATH, fname);
				StringCchCopy(tracks[i].FileName, MAX_PATH, stemp);
				SetItemFileName(i, stemp);
				ChangeTempFNameToReal(i, stemp);
			}
		}
	}
}

void MainWnd::ChangeTempFNameToReal(int idx, const wchar_t fname[])
{
	wchar_t newstr[5000];
	wchar_t *st1 = wcsstr(tracks[idx].StdOut, L"Processing file");
	wchar_t *st2 = wcsstr(tracks[idx].StdOut, L"Detected average");
	if( st1 == NULL || st2 == NULL ) return;
	StringCchCopy(newstr, 5000, tracks[idx].StdOut );
	newstr[ st1 - tracks[idx].StdOut ] = L'\0';
	StringCchCat(newstr, 5000, L"Processing file:\t[");

	wchar_t temp[MAX_PATH];
	StringCchCopy(temp, MAX_PATH, fname);
	wchar_t *st = PathFindFileName(temp);
	StringCchCat(newstr, 5000, st);
	StringCchCat(newstr, 5000, L"]\r\n");
	StringCchCat(newstr, 5000, st2);

	StringCchCopy(tracks[idx].StdOut, LENSTDOUT, newstr);
}

bool MainWnd::FindName(wchar_t Directory[], int des_pos, wchar_t fname[])
{
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind, hFile;
	SetCurrentDirectory(Directory);
	hFind = FindFirstFile(file_ext, &FindFileData);
	if(hFind == INVALID_HANDLE_VALUE) return false;
	wchar_t findname[MAX_PATH]; // наденное имя файла
	while(true)
	{
		StringCchCopy(findname, MAX_PATH, FindFileData.cFileName);
		if( FindFileData.nFileSizeLow != 4 ) 
			return false; // ещё не время
		else{ // открываем файл и читаем позицию в листе
			hFile = CreateFile(findname, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
			if( hFile != INVALID_HANDLE_VALUE ){
				int pos; DWORD D = 0;
				if( ReadFile(hFile, &pos, 4, &D, NULL) ){
					if( D == 4 && pos == des_pos ){
						StringCchCopy(fname, MAX_PATH, findname);
						CloseHandle(hFile);
						return true;
					}
				}
				CloseHandle(hFile);
			}
		}
		if(!FindNextFile(hFind, &FindFileData)) break;
	}
	return false;
}

void MainWnd::Clean()
{
	wchar_t message[5000];
	message[0] = L'\0';

	Find_aucdtectProc(message, 5000);

	for( int i = 0; i < (int)tracks.size(); i++ )
	{
		DeleteFile(tracks[i].FileName);
	}
	if( lstrlen(message) )
		MessageBox(0, message, L"Clean", MB_OK);
}

int MainWnd::Find_aucdtectProc(wchar_t *log, size_t cchDest)
{
	DWORD aProcesses[1024], cbNeeded, cProcesses;

	if ( !EnumProcesses( aProcesses, sizeof(aProcesses), &cbNeeded ) )
		return 0;
	cProcesses = cbNeeded / sizeof(DWORD);

	wchar_t str[1024];
	wchar_t procname[MAX_PATH];

	int fcount = 0;
	for ( unsigned int i = 0; i < cProcesses; i++ ) 
		// перебираем процессы и ищем aucdtect.exe
	{
		GetProcessNameByID(aProcesses[i], procname, MAX_PATH);
		if( lstrcmpi(procname, L"aucdtect.exe") == 0 )
			// попытка удалить процесс
		{
			fcount++;
			if( Terminate_aucdtectProc(aProcesses[i]) == 0 )
				StringCchPrintf(str, 1024, L"process: %s, pid=%d, terminate failed\n",procname,aProcesses[i]);	
			else
				StringCchPrintf(str, 1024, L"process: %s, pid=%d, terminate success\n",procname,aProcesses[i]);	
			StringCchCat(log, cchDest, str);
		}
	}
	if( fcount )
		StringCchCat(log, cchDest, L"Terminate processes: Done.");
	return fcount;
}

bool MainWnd::GetSaveFile(wchar_t *s, size_t cchDest)
{
	FindLastPath();
	wchar_t str[MAX_PATH]= L"fooCDtect.log.txt";
	OPENFILENAME of;
	memset(&of,0,sizeof(of));
	of.lStructSize = sizeof(of);
	//of.lpstrFilter = L"Unicode Text Files(*.txt)\0*.txt\0\0";
	of.lpstrFilter = L"ANSI text\0*.txt\0UTF16-LE text\0*.txt\0UTF-8 text\0*.txt\0\0";
	of.nFilterIndex = GetPosEncoding();
	of.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_ENABLESIZING;
	of.lpstrDefExt = L"*.txt";
	of.lpstrFile = str;
	of.nMaxFile = MAX_PATH;
	of.lpstrInitialDir = last_path;
	of.hwndOwner = hWnd;
	if( !GetSaveFileName(&of) )
		return false;
	StringCchCopy(s, cchDest, str);

	switch( of.nFilterIndex )
	{
		case LE_ANSI:
			ChangeMenuRadioCheck(ID_LOGENCODING_ANSI, cvLog_encoding);
			break;
		case LE_UCS2:
			ChangeMenuRadioCheck(ID_LOGENCODING_UCS2, cvLog_encoding);
			break;
		case LE_UTF8:
			ChangeMenuRadioCheck(ID_LOGENCODING_UTF8, cvLog_encoding);
			break;
	}

	return true;
}

bool MainWnd::GetCheckerString(wchar_t out_str[], size_t cchDest)
{
	wchar_t tstd[LENSTDOUT];
	for( int i = 0; i < (int)tracks.size(); i++ )
	{
		if( SafeStrLen(tracks[i].StdOut, LENSTDOUT) > 100 )
		{
			StringCchCopy(tstd, LENSTDOUT, tracks[i].StdOut);
			wchar_t *st = wcsstr(tstd, L"-");
			if( st ) (*st) = L'\0';
			StringCchCopy(out_str, cchDest, tstd);
			return true;
		}
	}
	return false;
}

bool MainWnd::isDone() // пока не нужна функция
{
	for( int i = 0; i < (int)tracks.size(); i++ )
		if( !tracks[i].rez_type ) return false;
	return true;
}

void MainWnd::SaveAll()
{
	wchar_t fname[MAX_PATH];
	if( GetSaveFile(fname, MAX_PATH) )
	{
		if( SaveList(tracks, fname) )
		{
			if( autodel )
			{
				SelectAll();
				DeleteTracks();
			}
		}
	}
}
void MainWnd::SaveSelected()
{
	std::vector <LISTITEM> tr;
	for( int i = 0; i < (int)tracks.size(); i++ ) 
	{
		if( !ListView_GetItemState(hList, i, LVIS_SELECTED) ) continue;
		tr.push_back(tracks[i]);
	}
	if( tr.size() )
	{
		wchar_t fname[MAX_PATH];
		if( GetSaveFile(fname, MAX_PATH) )
		{
			if( SaveList(tr, fname) )
			{
				if( autodel )
					DeleteTracks();
			}
		}		
	}
}

bool MainWnd::SaveList(std::vector <LISTITEM> &trlist, const wchar_t *fname)
{
	if( tracks.size() == 0 ) return false; // очень простая проверка, быть внимательным нужно.
	wchar_t filename[MAX_PATH];

	wchar_t kstr[1000];
	if( !GetCheckerString(kstr, 1000) ) return false;

	if( fname )
		StringCchCopy(filename, MAX_PATH, fname);
	else
	{
		PathFindPath(trlist[0].FileName, filename, MAX_PATH);
		StringCchCat(filename, MAX_PATH, L"fooCDtect.log.txt");
	}

	int log_format = GetPosFormat();	
	VecStrings masstr;

	if( log_format > 1 )
	{
		masstr.AddString(L"fooCDtect - foobar2000 + auCDtect, baralgin.\r\n\r\n");
		masstr.AddString(kstr);
		StringCchPrintf(kstr, 1000, L"\r\n     mode: %d\r\n\r\n===========================================\r\n", m_mode);
		masstr.AddString(kstr);
	}

	wchar_t str_rez[100];
	for( int i = 0; i < (int)trlist.size(); i++ ) 
	{
		switch( trlist[i].rez_type )
		{
		case 1:
			StringCchPrintf(str_rez, 100, L"CDDA %s", trlist[i].rez_prob);
			break;
		case 2:
			StringCchPrintf(str_rez, 100, L"MPEG %s", trlist[i].rez_prob);
			break;
		case 3:
			StringCchCopy(str_rez, 100, L"Unknown");
			break;
		case 4:
			StringCchCopy(str_rez, 100, L"Track format is wrong");
			break;
		default:
			continue;
		}
		
		wchar_t fullnamenoext[MAX_PATH], fnamenoext[MAX_PATH];
		PathFindFullFilenameNoExt(trlist[i].FileName, fullnamenoext, MAX_PATH);
		PathFindFilenameNoExt(trlist[i].FileName, fnamenoext, MAX_PATH);
		switch( log_format )
		{
			case LF_SIMPLE:
				masstr.AddString(fullnamenoext);
				masstr.AddString(L" ** ");
				masstr.AddString(str_rez);
				masstr.AddString(L"\r\n");
				continue;
			case LF_NORMAL:
				masstr.AddString(L"    Track: ");
				masstr.AddString(fnamenoext);
				masstr.AddString(L"\r\n  Quality: ");
				masstr.AddString(str_rez);
				masstr.AddString(L"\r\n");
				break;
			case LF_VERBOSE:
			{
				masstr.AddString(L"    Track: ");
				masstr.AddString(fnamenoext);
				masstr.AddString(L"\r\n\r\n");
				wchar_t *st;
				st = wcsstr(trlist[i].StdOut, L"Processing");
				masstr.AddString(st);
			}
				break;
		}
		masstr.AddString(L"===========================================\r\n");
	}
	int sizef = masstr.getalllen(), colstrs = masstr.get_size();
	bool fSave = false;
	if( sizef )
	{
		wchar_t *strf = new wchar_t[ sizef + 1 ];
		strf[0] = L'\0';
		for( int i = 0; i < colstrs; i++ )
			StringCchCat(strf, sizef + 1, masstr[i]);
		
		fSave = SaveLogToFile(strf, filename);

		delete [] strf;
	}
	
	return fSave;
}

bool MainWnd::SaveLogToFile(const wchar_t * str, const wchar_t * filename)
{
	HANDLE hFile = CreateFile(filename,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
	if(hFile == INVALID_HANDLE_VALUE) return false;
	
	int log_encoding = GetPosEncoding();

	DWORD D;
	int inlen = lstrlen(str);
	switch( log_encoding )
	{
		case LE_UCS2:
			WriteFile(hFile, BOM_UTF16LE, 2, &D, NULL);
			WriteFile(hFile, str, inlen * sizeof(wchar_t), &D, NULL);
			break;
		case LE_ANSI:
			{
				int newlen = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, str, -1, NULL, 0, NULL, NULL);
				if( newlen == 0 ) break;
				char *newstr = new char[newlen];
				WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK, str, -1, newstr, newlen, NULL, NULL);
				WriteFile(hFile, newstr, newlen - 1, &D, NULL);
				delete [] newstr;
				break;
			}
		case LE_UTF8:
			{
				WriteFile(hFile, BOM_UTF8, 3, &D, NULL);
				int newlen = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
				if( newlen == 0 ) break;
				char *newstr = new char[newlen];
				WideCharToMultiByte(CP_UTF8, 0, str, -1, newstr, newlen, NULL, NULL);
				WriteFile(hFile, newstr, newlen - 1, &D, NULL);
				delete [] newstr;
				break;
			}
	}

	CloseHandle(hFile);
	return true;
}

void MainWnd::AutoSaveAll()
{
	SortListByDirectory(true);
}
void MainWnd::SortListByDirectory(bool save)
{
	int i, len, count = (int) tracks.size();
	len = count;
	
	std::vector <LISTITEM> sort_list, newtemplist;
	bool clrBk = true;
	while( count ) // ищем папки разные
	{
		i = 0; // поиск первой новой папки
		while( i < len )
		{
			if( tracks[i].tsort == 0 ) break;
			i++;
		}
		static wchar_t find_str[MAX_PATH], t_str[MAX_PATH];
		PathFindPath(tracks[i].FileName, find_str, MAX_PATH);

		for( ; i < len; i++ )
		{
			if ( tracks[i].tsort == 0 )
			{
				PathFindPath(tracks[i].FileName, t_str, MAX_PATH);
				if( lstrcmp(find_str, t_str) == 0 )
				{
					sort_list.push_back(tracks[i]);
					tracks[i].tsort = 1; // всё, добавили - больше не трогаем это
					count--;
				}
			}
		}
		// сортировка найдёных
		SortVectorByFileName(sort_list);
		if( save )
		{
			SaveList(sort_list, NULL);
		}

		for( i = 0; i < (int)sort_list.size(); i++ )
		{
			if( clrBk )
				sort_list[i].clBk = BK_EVEN;
			else
				sort_list[i].clBk = BK_ODD;
			newtemplist.push_back(sort_list[i]);
		}
		clrBk = !clrBk;
		sort_list.clear();
	}
	tracks.clear();
	ListView_DeleteAllItems(hList);
	for( i = 0; i < (int)newtemplist.size(); i++ )
	{
		newtemplist[i].tsort = 0;
		tracks.push_back(newtemplist[i]);
	}
	RedrawListFromTracks();
}

void MainWnd::RedrawListFromTracks()
{
	for( int i = 0; i < (int)tracks.size(); i++ )
	{
		AddListItem(L"");
		SetItemFileName(i, tracks[i].FileName);
		SetItemProgress(i, tracks[i].progress);
		SetItemLenght(i, tracks[i].len);
		SetItemResult(i, tracks[i].rez_type, tracks[i].rez_prob);
	}
}

void MainWnd::OnListItemChanged(LPARAM lP)
{
	int item = ((LPNMLISTVIEW)lP)->iItem;
	UINT state = ((LPNMLISTVIEW)lP)->uNewState;
	if( state & LVIS_SELECTED ){
		wchar_t path[MAX_PATH];
		PathFindPath(tracks[item].FileName, path, MAX_PATH);
		StringCchCopy(last_path, MAX_PATH, path);
		SetStatusText3(path);
	}
	else{ 
		SetStatusText3(L"");
		last_path[0] = L'\0';
	}
}


void MainWnd::FindLastPath()
{
	if ( tracks.size() && lstrlen(last_path) == 0 )
	{
		PathFindPath(tracks[0].FileName, last_path, MAX_PATH);
	}
}

//////////////////////////////////////////////////////////////////////////


LRESULT MainWnd::OnLVCustomDraw(LPARAM lParam)
{
	LPNMLVCUSTOMDRAW  lplvcd = (LPNMLVCUSTOMDRAW)lParam;
	LRESULT pResult = CDRF_DODEFAULT;

	HDC	hDC = lplvcd->nmcd.hdc;
	int item = (int)lplvcd->nmcd.dwItemSpec;
	int subitem = lplvcd->iSubItem;
//	RECT rc = lplvcd->nmcd.rc;

	switch( lplvcd->nmcd.dwDrawStage )
	{
		case CDDS_PREPAINT :
			pResult |= CDRF_NOTIFYITEMDRAW;
			break;
		case CDDS_ITEMPREPAINT:
			{
				lastDrawItem = item;
				if( lastDrawItem < 0 || lastDrawItem >= (int)tracks.size() )
					return pResult;
				if( lplvcd->nmcd.uItemState & LVIS_SELECTED )
					return pResult;

				pResult |= CDRF_NOTIFYSUBITEMDRAW | CDRF_NOTIFYPOSTPAINT;
			}
			break;
		case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
			switch( subitem )
			{
				case 1:{
					DrawItemProgress(item, lplvcd->nmcd.uItemState, hDC);
					pResult |= CDRF_SKIPDEFAULT;
					   }
					break;					
				case 2:{ 
					lplvcd->clrText = RGB(0, 0, 128);
					   }
					break;
				case 3: 
					switch( tracks[lastDrawItem].rez_type )
					{
						case 1: lplvcd->clrText = RGB(0, 128, 0); break;
						case 2: lplvcd->clrText = RGB(255, 0, 0); break;
						default: lplvcd->clrText = RGB(0, 0, 0);
					}
					break;
			}
			lplvcd->clrTextBk = tracks[lastDrawItem].clBk;
			break;
		case CDDS_ITEMPOSTPAINT:
			break;
	}
	return pResult;
}
void MainWnd::CreateItemProgress(HDC &t100, int iWidth, int iHeight, int item)
{
	RECT tRC = { 0, 0, iWidth - 1, iHeight - 1 };
	FillRect(t100, &tRC, tBrush);

	if( tracks[item].progress >= 0 )
	{
		int prz = (int) ( iWidth * tracks[item].progress / 100.0 );
		for( int i = 0; i < iHeight; i++ )
		{
			SetDCPenColor(t100, tGrad[i] );
			MoveToEx(t100, 0, i, NULL); 
			LineTo(t100, prz , i );
		}
	}	

	wchar_t str[20];
	ListView_GetItemText(hList, item, 1, str, 20);
	DrawText(t100, str, lstrlen(str), &tRC, DT_CENTER | DT_VCENTER);
}
void MainWnd::DrawItemProgress(int item, DWORD state, HDC hDC)
{
	RECT subrc;
	ListView_GetSubItemRect(hList, item, 1, LVIR_BOUNDS, &subrc);

	int iWidth = subrc.right - subrc.left + 1;
	int iHeight = subrc.bottom - subrc.top + 1;

	if( !tFl ) InitProgressDraw(hDC, iHeight);


	HBITMAP tBM = CreateCompatibleBitmap(hDC, iWidth, iHeight);
	SelectObject(tDC, tBM);
	CreateItemProgress(tDC, iWidth, iHeight, item);
	BitBlt(hDC, subrc.left, subrc.top, iWidth - 1, iHeight - 1, tDC, 0, 0, SRCCOPY);
	DeleteObject(tBM);
}

void MainWnd::InitProgressDraw(HDC hdc, int height)
{
	tDC = CreateCompatibleDC(hdc);
	SelectObject(tDC,GetStockObject(DC_PEN));
	tBrush = CreateSolidBrush(RGB(128, 128, 128));

	double step = 1.0 / (height - 1);
	for( int i = 0; i < height; i++ )
	{
		tGrad[i] = RGB( 64, 64, (int)( 255.0 * sin( i * step * M_PI ) ) );
	}

	HFONT oldFont = (HFONT) GetCurrentObject(hdc, OBJ_FONT);
	LOGFONT lfont;
	GetObject(oldFont, sizeof(LOGFONT), &lfont);
	lfont.lfHeight += 1;
	tFont = CreateFontIndirect(&lfont);

	SelectObject(tDC, tFont);
	SetTextColor(tDC, RGB(255, 255, 255) );
	SetBkMode(tDC, TRANSPARENT);

	tFl = true;
}

void MainWnd::ReleaseProgressDraw()
{
	if( tFl )
	{
		DeleteObject(tFont);
		DeleteObject(tBrush);
		DeleteDC(tDC);
		tFl = false;
	}
}

void MainWnd::OnListContextMenu(LPARAM lParam)
{
	LPNMITEMACTIVATE lpit = (LPNMITEMACTIVATE)lParam;

	POINT point;
	GetCursorPos(&point);
	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING, 1, L"Sort");
	if( lpit->iItem >= 0 )
	{
		AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
		AppendMenu(hMenu, MF_STRING, 2, L"StdOut");
		AppendMenu(hMenu, MF_STRING, 4, L"Save Selected");
		AppendMenu(hMenu, MF_STRING, 3, L"Open Directory");
		SetMenuDefaultItem(hMenu, 2, FALSE);
	}
	SetForegroundWindow(hWnd);
	int rez = TrackPopupMenu(hMenu,TPM_RETURNCMD,point.x,point.y,0,hWnd,0);
	PostMessage(hWnd,WM_NULL,0,0);
	DestroyMenu(hMenu);

	switch( rez )
	{
		case 1:
			SortListByDirectory();
			break;
		case 2:
			OnListDbClick(lParam);
			break;
		case 3:{
			wchar_t path[MAX_PATH];
			PathFindPath(tracks[lpit->iItem].FileName, path, MAX_PATH);
			ShellExecute(hWnd, L"open", path, NULL, NULL, SW_SHOWNORMAL);
			}
			break;
		case 4:
			SaveSelected();
			break;
	}
}

void MainWnd::DeleteTracks()
{
	std::vector <LISTITEM>::iterator iter;
	std::vector <LISTITEM> tvec;
	int i = 0;
	for(			iter  = tracks.begin(); 
					iter != tracks.end();
		 i++,		iter++) 
	{
		if( ListView_GetItemState(hList, i, LVIS_SELECTED) &&
			iter->rez_type > 0 && 
			!isFileNameTemp(PathFindFileName(iter->FileName)))
		{
			DeleteFile(iter->FileName);
			ListView_DeleteItem(hList,i--);
		}
		else
			tvec.push_back(*iter);
	}
	tracks = tvec;
}

void MainWnd::StatusBarChangePriority()
{
	POINT point;
	GetCursorPos(&point);
	HMENU hMenu = CreatePopupMenu();
	AppendMenu(hMenu, MF_STRING | MF_DISABLED, 1, L"max threads:");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, 2, L"  0 (unlimited)");
	AppendMenu(hMenu, MF_STRING, 3, L"  1");
	AppendMenu(hMenu, MF_STRING, 4, L"  2");
	AppendMenu(hMenu, MF_STRING, 5, L"  3");
	AppendMenu(hMenu, MF_STRING, 6, L"  4");
	AppendMenu(hMenu, MF_STRING, 7, L"  5");
	AppendMenu(hMenu, MF_STRING, 8, L"  6");
	AppendMenu(hMenu, MF_STRING, 9, L"  7");
	AppendMenu(hMenu, MF_STRING, 10, L"  8");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING | MF_DISABLED, 11, L"priority:");
	AppendMenu(hMenu, MF_SEPARATOR, 0, NULL);
	AppendMenu(hMenu, MF_STRING, 12, L"normal");	
	AppendMenu(hMenu, MF_STRING, 13, L"below normal");	
	AppendMenu(hMenu, MF_STRING, 14, L"idle");	

	CheckMenuItem(hMenu, max_thr + 2, MF_BYCOMMAND | MF_CHECKED );

	DWORD pry = GetPriorityClass(GetCurrentProcess());
	switch(pry)
	{
	case NORMAL_PRIORITY_CLASS:
		CheckMenuItem(hMenu, 12, MF_BYCOMMAND | MF_CHECKED );
		break;
	case BELOW_NORMAL_PRIORITY_CLASS:
		CheckMenuItem(hMenu, 13, MF_BYCOMMAND | MF_CHECKED );
		break;
	case IDLE_PRIORITY_CLASS:
		CheckMenuItem(hMenu, 14, MF_BYCOMMAND | MF_CHECKED );
		break;
	}

	SetForegroundWindow(hWnd);
	int rez = TrackPopupMenu(hMenu,TPM_RETURNCMD,point.x,point.y,0,hWnd,0);
	PostMessage(hWnd,WM_NULL,0,0);
	DestroyMenu(hMenu);

	switch(rez)
	{
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
			max_thr = rez - 2;
			SetStatusText1();
			break;
		case 12:
			ChangePriorityClasses(NORMAL_PRIORITY_CLASS);
			break;
		case 13:
			ChangePriorityClasses(BELOW_NORMAL_PRIORITY_CLASS);
			break;
		case 14:
			ChangePriorityClasses(IDLE_PRIORITY_CLASS);
			break;
	}
}

void MainWnd::ChangePriorityClasses(DWORD newpriority)
{
	SetPriorityClass(GetCurrentProcess(), newpriority);
	std::vector <LISTITEM>::iterator iter;
	for( iter  = tracks.begin(); iter != tracks.end();iter++ ) 
	{
		if( iter->rez_type == -1 )
		{
			if( iter->dwProcId1 != 0 )
			{
				HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, iter->dwProcId1);
				if( hProc )
				{
					SetPriorityClass(hProc, newpriority);
					CloseHandle(hProc);
				}
			}
			if( iter->dwProcId2 != 0 )
			{
				HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, iter->dwProcId2);
				if( hProc )
				{
					SetPriorityClass(hProc, newpriority);
					CloseHandle(hProc);
				}
			}
		}
	}
}
bool MainWnd::isMenuItemRadioChecked(const int nID)
{
	MENUITEMINFO info;
	info.cbSize = sizeof(MENUITEMINFO);
	info.fMask = MIIM_STATE;
	if( GetMenuItemInfo( GetMenu(hWnd), nID, FALSE, &info) )
		return (MFS_CHECKED & info.fState) ? true : false;
	return false;
}

int MainWnd::GetPosEncoding()
{
	if( isMenuItemRadioChecked(ID_LOGENCODING_ANSI) ) return 1;
	if( isMenuItemRadioChecked(ID_LOGENCODING_UCS2) ) return 2;
	if( isMenuItemRadioChecked(ID_LOGENCODING_UTF8) ) return 3;
	return 3; // на всякий случай
}
int MainWnd::GetPosFormat()
{
	if( isMenuItemRadioChecked(ID_LOGFORMAT_SIMPLE) ) return 1;
	if( isMenuItemRadioChecked(ID_LOGFORMAT_NORMAL) ) return 2;
	if( isMenuItemRadioChecked(ID_LOGFORMAT_VERBOSE) ) return 3;
	return 2; // на всякий случай
}
void MainWnd::ShowStats()
{
	int col_CDDA = 0, col_MPEG = 0, col_Unk = 0, col_Bad = 0;
	int total = 0;
	std::vector <LISTITEM>::iterator iter;
	for( iter  = tracks.begin(); iter != tracks.end();iter++ ) 
	{
		switch( iter->rez_type )
		{
			case 1: col_CDDA++; total++; break;
			case 2: col_MPEG++; total++; break;
			case 3: col_Unk++;  total++; break;
			case 4: col_Bad++;  total++; break;
		}
	}
	wchar_t mess[1000];
	StringCchPrintf(mess, 1000,
				   L"Total tracks checked:\t\t%d\n\n"
				   L"Result:\n"
				   L"=========================\n"
				   L"CDDA:\t\t\t\t%d\n"
				   L"MPEG:\t\t\t\t%d\n"
				   L"Unknown:\t\t\t%d\n"
				   L"Wrong track format:\t\t%d\n"
				   , total, col_CDDA, col_MPEG, col_Unk, col_Bad);
	MessageBox(hWnd, mess, L"Stats", MB_OK);
}

void MainWnd::SetWinTitle()
{
	
	wchar_t str[100];
	int posf = GetPosFormat();
	int pose = GetPosEncoding();
	StringCchPrintf(str, 99, 
		L"fooCDtect    ¤    format: %s,  encoding: %s, autodel %s", 
		posf == 1 ? L"simple" : posf == 2 ? L"normal" : L"verbose",
		pose == 1 ? L"ANSI" : pose == 2 ? L"UTF16" : L"UTF8",
		autodel ? L"on" : L"off"
		);
	SetWindowText(hWnd, str);
}

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
void SortVectorByFileName(std::vector <LISTITEM> &slist)
{
	// пузырёк
	int len = (int) slist.size();
	bool fstop = false;
	while( !fstop )
	{
		fstop = true;
		for(int i = 0; i < len - 1; i++ )
		{
			if( lstrcmp(slist[i].FileName, slist[i+1].FileName) > 0 )
			{
				LISTITEM it = slist[i];
				slist[i] = slist[i+1];
				slist[i+1] = it;
				fstop = false;
			}
		}
	}
}

void PathFindPath(const wchar_t* fname, wchar_t* outpath, size_t cchDest)
{
	StringCchCopy(outpath, cchDest, fname)	;
	wchar_t *ts = PathFindFileName(outpath);
	(*ts) = L'\0'; // оставляем только путь без имени файла
}
void PathFindFilenameNoExt(const wchar_t* fname, wchar_t* outpath, size_t cchDest)
{
	StringCchCopy(outpath, cchDest, PathFindFileName(fname));
	wchar_t *st2 = PathFindExtension(outpath);
	(*st2) = L'\0';
}
void PathFindFullFilenameNoExt(const wchar_t* fname, wchar_t* outpath, size_t cchDest)
{
	StringCchCopy(outpath, cchDest, fname)	;
	wchar_t *ts = PathFindExtension(outpath);
	(*ts) = L'\0'; // оставляем только путь без имени файла
}

void GetProcessNameByID( DWORD processID , wchar_t *szProcessNameOut, size_t cchDest)
{
	StringCchCopy(szProcessNameOut, cchDest, L"unknown" );
	wchar_t szProcessName[MAX_PATH];

	HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION |
		PROCESS_VM_READ,
		FALSE, processID );

	if (NULL != hProcess )
	{
		HMODULE hMod;
		DWORD cbNeeded;

		if ( EnumProcessModules( hProcess, &hMod, sizeof(hMod), 
			&cbNeeded) )
		{
			GetModuleBaseName( hProcess, hMod, szProcessName, 
				sizeof(szProcessName) );
		}
		else return;
	}
	else return;

	StringCchCopy(szProcessNameOut, cchDest, szProcessName);
	
	CloseHandle( hProcess );
}

int Terminate_aucdtectProc( DWORD processID )
{
	HANDLE hProcess = OpenProcess( PROCESS_ALL_ACCESS, FALSE, processID );
	if( hProcess )
	{
		DWORD ExitCode;
		if( GetExitCodeProcess(hProcess, &ExitCode) == 0 )
			return 0;
		if( TerminateProcess(hProcess, ExitCode) == 0)
			return 0;
		CloseHandle(hProcess);
		return 1;
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////
size_t SafeStrLen(const wchar_t *str, size_t cchMax)
{
	size_t rez;
	if( S_OK != StringCchLength(str, cchMax, &rez) )
		return 0;
	return rez;
}



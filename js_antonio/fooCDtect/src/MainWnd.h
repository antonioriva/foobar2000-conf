#pragma once
#include <vector>
#include <CommCtrl.h>
#include <strsafe.h>

#define _SIZE_X_	575
#define _SIZE_Y_	300

#define BK_EVEN		RGB(255, 255, 255)
#define BK_ODD		RGB(220, 220, 220)

#define LENSTDOUT	2000

struct LISTITEM
{
	int			idx;
	int			tsort;

	int			rez_type;			// результат (1 - CDDA, 2 - MPEG, 3 - unknown, 4 - неверный файл)
	wchar_t		rez_prob[5];		 // веро€тность
	wchar_t		FileName[MAX_PATH];  // полное им€ файла
	wchar_t		StdOut[LENSTDOUT];	// всЄ что было в stdout
	int			progress;
	DWORD		len;
	COLORREF	clBk;
	DWORD		dwProcId1;
	DWORD		dwProcId2;
	LISTITEM()
	{
		StringCchCopy(StdOut, LENSTDOUT, L"");
		StringCchCopy(FileName, MAX_PATH, L"");
		rez_type = -1;
		idx = 0;
		tsort = 0;
		progress = -1;
		len = 0;
		clBk = BK_EVEN;
		dwProcId1 = 0;
		dwProcId2 = 0;
	};
};
struct tagCMDL;

class VecStrings
{
	std::vector <wchar_t *> strings;
	int alllen;
public:
	VecStrings(){ alllen = 0; };
	~VecStrings(){ ClearMas(); }
	void AddString(wchar_t *str)
	{
		int inlen = lstrlen(str);
		if( inlen == 0 ) return;
		wchar_t *news = new wchar_t[ inlen + 1 ];
		StringCchCopy(news, inlen + 1, str);
		strings.push_back(news);
		alllen += inlen;
	}
	int get_size(){ return (int)strings.size(); }
	int getalllen(){ return alllen; }
	void ClearMas()
	{
		for( int i = 0; i < get_size(); i++ ) delete [] strings[i];
		strings.clear();
		alllen = 0;
	}
	const wchar_t *operator[] (int index)
	{
		if( index >= get_size() ) return NULL;
		return strings[index];
	}
};

class MainWnd
{
	HWND		hWnd;			// главное окно
	HWND		hList;			// лист
	HWND		hStatus;
	HINSTANCE	hInstance;

	int			max_thr;
	int			cur_thr;

	std::vector <LISTITEM> tracks;
	
	int			index;
	wchar_t		last_path[MAX_PATH];
	wchar_t		file_ext[MAX_PATH];
	wchar_t		m_temppath[MAX_PATH];
	int			m_mode;

	HICON		shell_icon;
	HACCEL		hAccel;

	int			lastDrawItem;
	bool		autodel;

	int			lformat;
	int			lencoding;
	int			priority;
	// хэндлы дл€ рисовани€ прогресса
	bool		tFl;
	HDC			tDC;
	HFONT		tFont;
	HBRUSH		tBrush;
	COLORREF	tGrad[1000];
public:
	MainWnd(tagCMDL cmdl);
	~MainWnd()
	{
		CloseMutex(); // тут уже ничего не принимаем, веро€тно провер€ющие потоки
					  // сами удал€т файлы..
		ReleaseProgressDraw();
		Clean(); 
	};
	void Run();					// этим запускаем извне
protected:
	void SetWinTitle();

	void MyRegisterClass();
	static LRESULT CALLBACK WndProc(HWND ,UINT ,WPARAM ,LPARAM );
	LRESULT ClassProc(HWND, UINT, WPARAM, LPARAM );
	void CreateMainWnd();

	void ErrorMessageBox(const wchar_t *str);

	void SetRez(wchar_t *StdOut, int item);

	bool isFileNameTemp(const wchar_t *);
	void GetNames();
	bool FindName(wchar_t Directory[], int des_pos, wchar_t fname[]);

	void Clean();
	int  Find_aucdtectProc(wchar_t *log, size_t cchDest);

	void ChangePriorityClasses(DWORD newpriority);

	void DeleteTracks();

	void FindLastPath();

	bool isDone();
	bool GetSaveFile(wchar_t *s, size_t cchDest);

	void ChangeTempFNameToReal(int idx, const wchar_t fname[]);

	void SaveAll();
	void SaveSelected();
	void AutoSaveAll();
//	bool SaveResult(bool f_all);
	bool SaveList(std::vector <LISTITEM> &trlist, const wchar_t *fname);
	
	void ShowStats();

	bool GetCheckerString(wchar_t out_str[], size_t cchDest);

	bool SaveLogToFile(const wchar_t * str, const wchar_t * filename);

	// статус
	void SetStatusText1();
	void SetStatusText2();
	void SetStatusText3(const wchar_t *);
	void StatusBarDBClick();
	void StatusBarChangePriority();

	//////////////////////////////////////////////////////////////////////////
	// функции листа
	void CreateListView();
	void AddListItem(const wchar_t* prog);
	void SetItemText(int pos, int subpos, const wchar_t *str){
		ListView_SetItemText(hList, pos, subpos, (LPWSTR)str);
	}
	void SelectAll();
	void InvertSelection();
	void DrawItemProgress(int item, DWORD state, HDC hDC);
	void CreateItemProgress(HDC &t100, int iw, int ih, int item);

	void SetItemFileName(int pos, const wchar_t* fname);
	void SetItemProgress(int pos, int protz);
	void SetItemLenght(int pos, DWORD len);
	void SetItemResult(int pos, int rez_type, const wchar_t* prob);

	int  GetPosByIndex(int idx);
	void SortListByDirectory(bool save = false);
	void RedrawListFromTracks();

	void InitProgressDraw(HDC hdc, int height);
	void ReleaseProgressDraw();

	void ChangeMenuRadioCheck(const int nID, const int *masIDs)
	{
		CheckMenuRadioItem( GetMenu(hWnd), 	masIDs[0], masIDs[2], nID, MF_BYCOMMAND);
	}
	bool isMenuItemRadioChecked(const int nID);
	int GetPosEncoding();
	int GetPosFormat();

	// собщени€
	void	OnClose();
	LRESULT OnSize();
	LRESULT OnNewThread();
	void OnCopyData(int item, PCOPYDATASTRUCT lParam);
	void OnLength(int item, DWORD length);
	void OnProgress(int item, int pos);
	
	void OnListDbClick(LPARAM lp);
	void OnListItemChanged(LPARAM lP);
	LRESULT OnLVCustomDraw(LPARAM lParam);
	void OnListContextMenu(LPARAM lParam);
};

void PathFindPath(const wchar_t* fname, wchar_t* outpath, size_t cchDest);
void PathFindFilenameNoExt(const wchar_t* fname, wchar_t* outpath, size_t cchDest);
void PathFindFullFilenameNoExt(const wchar_t* fname, wchar_t* outpath, size_t cchDest);
size_t	SafeStrLen(const wchar_t *, size_t cchMax);
void SortVectorByFileName(std::vector <LISTITEM> &slist);
void GetProcessNameByID( DWORD processID , wchar_t *szProcessNameOut, size_t cchDest);
int Terminate_aucdtectProc( DWORD processID );

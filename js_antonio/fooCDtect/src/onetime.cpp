#include <windows.h>
#include "onetime.h"

HANDLE hMutexOneInstance = NULL;

bool isProcessExist()
{
	HANDLE hMut = NULL;
	hMut = OpenMutex(MUTEX_ALL_ACCESS, FALSE, MUTNAME);
	if( hMut == NULL ) return FALSE;

	ReleaseMutex(hMut);
	CloseHandle(hMut);
	return true;
}

bool isProcessRunnig()
{
	hMutexOneInstance = 
		CreateMutex( NULL, TRUE, MUTNAME); // guidgen.com
	if( GetLastError() == ERROR_ALREADY_EXISTS )
		return true;
	return false;
}

void CloseMutex()
{
	if( hMutexOneInstance != NULL )
		CloseHandle( hMutexOneInstance );
}
void MReleaseMutex()
{
	if( hMutexOneInstance != NULL )
		ReleaseMutex( hMutexOneInstance );
}
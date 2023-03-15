#include <Windows.h>
#include <strsafe.h>
#include "resource.h"

// For making sure only one instance of the app is running at a time.
HANDLE appMutex= NULL;

// Flag for if we're currently using the search mode for a window
BOOL g_bSearchingNow = FALSE;

// Handles to the "bullseye" cursor (active while searching) and the user's regular cursor (to restore when done searching)
HCURSOR g_hBullseyeCursor, g_hRegularCursor;

BOOL InitApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, INT nShowCmd)
{
	//Attempt to init the app; if the mutex can't be registered or already exists, return FALSE, else return TRUE
	DWORD le;
	appMutex = CreateMutexW(NULL, TRUE, L"WFPROTOMUTEX");
	le = GetLastError();
	if(appMutex==NULL)
	{
		return FALSE;
	}
	if(le==ERROR_ALREADY_EXISTS)
	{
		CloseHandle(appMutex);
		appMutex = NULL;
		return FALSE;
	}
	return TRUE;
}

VOID UninitApp()
{
	// Release and free the mutex.
	if(appMutex)
	{
			ReleaseMutex(appMutex);
			CloseHandle(appMutex);
	}
}

VOID UpdateAppImg(HWND hDlg, BOOL ForE)
{
	// Update the "exe" icon in the dialog to "full" or "empty" based on the value of ForE
	HBITMAP hBmp;
	HINSTANCE hInstance = GetModuleHandleW(NULL);
	if(ForE==TRUE)
	{
		hBmp = (HBITMAP)LoadBitmapW(hInstance, MAKEINTRESOURCEW(IDB_FINDER_ACTIVE));
	}
	else
	{
		hBmp = (HBITMAP)LoadBitmapW(hInstance, MAKEINTRESOURCEW(IDB_FINDER_IDLE));
	}
	SendDlgItemMessageW(hDlg, IDC_STATIC_FINDERTOOL, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
}

BOOL IsValidWindow(HWND hwTarget, HWND hDlg)
{
	// Determine if the window in consideration is a valid window whose info to show.
	HWND hwTemp = NULL;
	if(hwTarget==NULL||hwTarget==INVALID_HANDLE_VALUE)
	{
		return FALSE;
	}
	else if(IsWindow(hwTarget)==FALSE)
	{
		return FALSE;
	}
	else if (hwTarget==hDlg)
	{
		return FALSE;
	}
	hwTemp = GetParent(hwTarget);
	if(hwTemp==hDlg)
	{
		return FALSE;
	}
	return TRUE;
}

// Mouse up (aka done searching): restore the cursor and "exe" icon in the app window.
VOID HandleMouseUp(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	SetCursor(g_hRegularCursor);
	UpdateAppImg(hWnd, TRUE);
	g_bSearchingNow = FALSE;
	
	// Seems like this forces it to update on my computer - other machines may see different results
	ShowWindow(hWnd, SW_HIDE);
	Sleep(10);
	ShowWindow(hWnd, SW_SHOW);
}


VOID HandleMouseMove(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	// Mouse moved: if the mouse is now under a valid window, put its info into our dialog box
	POINT p;
	HWND hwFound;
	
	GetCursorPos(&p);
	hwFound = WindowFromPoint(p);
	if(IsValidWindow(hwFound, hWnd))
	{
		WCHAR wszWndInfo[512];
		WCHAR wszCnBuf[128];
		RECT rect;
		GetClassNameW(hwFound, wszCnBuf, 128);
		GetWindowRect(hwFound, &rect);
		StringCchPrintfW(wszWndInfo, 512, L"Target: 0x%.8X\r\n\'%s\'\r\n%dx%d", (int)hwFound, wszCnBuf, rect.right - rect.left, rect.bottom - rect.top);
		SetDlgItemTextW(hWnd, IDC_STATIC_WNDINFO, wszWndInfo);
		
		
	}
}

// When the bullseye cursor is chosen, set the "point" under the mouse to be (16,16) pixels further than it normally is (so that the center of the bullseye is the center of the cursor)
VOID UpdateCursor(HWND hDlg)
{
	HWND hwFinder;
	RECT rect;
	POINT p;

	hwFinder = GetDlgItem(hDlg, IDC_STATIC_FINDERTOOL);
	GetWindowRect(hwFinder, &rect);
	p.x = rect.left+16;
	p.y = rect.top+16;
	SetCursorPos(p.x, p.y);
}
// Search handling proc
// Seach 
VOID SearchWindow(HWND hDlg)
{
	g_bSearchingNow = TRUE;
	UpdateAppImg(hDlg, FALSE);
	UpdateCursor(hDlg);
	g_hRegularCursor = SetCursor(g_hBullseyeCursor);
	SetCapture(hDlg);
	//ShowWindow(hDlg, SW_MINIMIZE);
}

BOOL CALLBACK WFDlgProc(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{	
	switch(Msg)
	{
	case WM_INITDIALOG:
		
		
		return TRUE;
	case WM_COMMAND:
		{
			if(LOWORD(wParam)==IDOK)
			{
				EndDialog(hWnd, LOWORD(wParam));
				return TRUE;
			}
			else if(LOWORD(wParam)==IDC_STATIC_FINDERTOOL)
			{
				//UpdateAppImg(hWnd, FALSE);
				SearchWindow(hWnd);
				return TRUE;
			}
		}
	case WM_MOUSEMOVE:
		{
			if(g_bSearchingNow)
			{
				HandleMouseMove(hWnd, Msg, wParam, lParam);
			}
			return TRUE;
		}
	case WM_LBUTTONUP:
		{
			if(g_bSearchingNow)
			{
				HandleMouseUp(hWnd, Msg, wParam, lParam);
				//g_bSearchingNow=FALSE;
			}
			return TRUE;
		}
	}
	return FALSE;
}


INT APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ INT nShowCmd)
{
	if (!InitApp(hInstance, hPrevInstance, lpCmdLine, nShowCmd))
	{
		goto cleanup;
	}

	g_bSearchingNow = FALSE;
	g_hRegularCursor = LoadCursorW(NULL, IDC_ARROW);
	g_hBullseyeCursor =  (HCURSOR)LoadCursorW(hInstance, MAKEINTRESOURCEW(IDC_BULLSEYE_CURSOR));
	DialogBoxW(hInstance, MAKEINTRESOURCEW(IDD_WNDFINDER), NULL, WFDlgProc);
cleanup:
	UninitApp();
	return 0;
}
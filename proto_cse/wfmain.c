
#include <Windows.h>
#include <tchar.h>
#include <strsafe.h>

#include "resource.h"

HANDLE appMutex= NULL;
BOOL g_bSearchingNow = FALSE;
HCURSOR g_hBullseyeCursor, g_hRegularCursor;
BOOL InitApp(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, INT nShowCmd)
{
	//BOOL rSuccess = FALSE;
	DWORD le;
	appMutex = CreateMutex(NULL, TRUE, _T("WFPROTOMUTEX"));
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
	if(appMutex)
	{
			ReleaseMutex(appMutex);
			CloseHandle(appMutex);
	}
}

VOID UpdateAppImg(HWND hDlg, BOOL ForE)
{
	HBITMAP hBmp;
	HINSTANCE hInstance = GetModuleHandle(NULL);
	if(ForE==TRUE)
	{
		hBmp = (HBITMAP)LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FINDER_ACTIVE));
	}
	else
	{
		hBmp = (HBITMAP)LoadBitmap(hInstance, MAKEINTRESOURCE(IDB_FINDER_IDLE));
	}
	SendDlgItemMessage(hDlg, IDC_STATIC_FINDERTOOL, STM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)hBmp);
}

BOOL IsValidWindow(HWND hwTarget, HWND hDlg)
{
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

VOID RefreshWindow(HWND hWnd)
{
	// Not needed.
	UNREFERENCED_PARAMETER(hWnd);
	//InvalidateRect(hWnd, NULL, TRUE);
	//UpdateWindow(hWnd);
	//RedrawWindow(hWnd, NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ALLCHILDREN | RDW_FRAME);
}

VOID HandleMouseUp(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	SetCursor(g_hRegularCursor);
	UpdateAppImg(hWnd, TRUE);
	g_bSearchingNow = FALSE;
	ShowWindow(hWnd, SW_HIDE);
	Sleep(10);
	ShowWindow(hWnd, SW_SHOW);
}

VOID HandleMouseMove(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
	POINT p;
	HWND hwFound;
	
	GetCursorPos(&p);
	hwFound = WindowFromPoint(p);
	if(IsValidWindow(hwFound, hWnd))
	{
		TCHAR szWndInfo[512];
		TCHAR szCnBuf[128];
		RECT rect;
		GetClassName(hwFound, szCnBuf, 128);
		GetWindowRect(hwFound, &rect);
		StringCchPrintf(szWndInfo, 512, _T("Target: 0x%.8X\r\n\'%s\'\r\n%dx%d"), (int)hwFound, szCnBuf, rect.right - rect.left, rect.bottom - rect.top);
		//_stprintf_s(szWndInfo, 512, _T("Target: 0x%.8X\r\n\'%s\'\r\n%dx%d"), (int)hwFound, szCnBuf, rect.right-rect.left, rect.bottom-rect.top);
		SetDlgItemText(hWnd, IDC_STATIC_WNDINFO, szWndInfo);
		RefreshWindow(hwFound);
		
	}
}


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


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	if(!InitApp(hInstance, hPrevInstance, lpCmdLine, nShowCmd)) goto quitapp;
	
	
	g_bSearchingNow = FALSE;
	g_hRegularCursor = LoadCursor(NULL, IDC_ARROW);
	g_hBullseyeCursor =  (HCURSOR)LoadCursor(hInstance, MAKEINTRESOURCE(IDC_BULLSEYE_CURSOR));
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_WNDFINDER), NULL, WFDlgProc);
quitapp:
	UninitApp();
	return 0;
}
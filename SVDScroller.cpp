//////////////////////////////////////////////////////////////////////////

#include "SVDScroller.h"

//////////////////////////////////////////////////////////////////////////

LPCTSTR pszTxt = _T("SVD Scroller™");
LPCTSTR pszInf = _T("Программа для прокрутки содержимого неактивных окон.");
LPCTSTR pszOn  = _T("Включено средство прокрутки содержимого неактивных окон.");
LPCTSTR pszOff = _T("Средство прокрутки содержимого неактивных окон отключено.");
LPCTSTR pszMsg = _T("\tВы действительно хотите завершить работу\r\nпрограммы для прокрутки содержимого неактивных окон?\r\n");
LPCTSTR pszErr = _T("Программа для прокрутки содержимого неактивных окон уже запущена!\r\n");
bool    gbUse  = false;
HHOOK   hHook  = NULL;
UINT    guMsg  = NULL;
HWND    ghWnd  = NULL;
DWORD   dwThr  = NULL;
HICON   ghIOn  = NULL;
HICON   ghIOff = NULL;
HICON   ghIUse = NULL;
HICON   ghIInf = NULL;
INT_PTR ipTID  = NULL;
HMENU   ghMenu = NULL;
HBITMAP ghBInf = NULL;
HBITMAP ghBOn  = NULL;
HBITMAP ghBOff = NULL;
HBITMAP ghBEnd = NULL;

//////////////////////////////////////////////////////////////////////////

NOTIFYICONDATA gNID = { sizeof(gNID), 0 };

//////////////////////////////////////////////////////////////////////////

void ShowTraiIcon( bool bCreate, bool bShowInfo )
{
	gNID.hWnd = ghWnd;
	gNID.uID = 100;      // Per WinCE SDK docs, values from 0 to 12 are reserved and should not be used.
	gNID.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
	gNID.hIcon = ipTID ? ghIUse : ( gbUse ? ghIOn : ghIOff );
	gNID.uCallbackMessage = guMsg;
	lstrcpyn( gNID.szTip, pszTxt, sizeof(gNID.szTip) );
	if( bShowInfo )
	{
		gNID.uFlags |= NIF_INFO;
		gNID.uTimeout = 10000; // 10 сукунд - системный минимум
		gNID.dwInfoFlags = NIIF_INFO;
		lstrcpyn( gNID.szInfoTitle, pszTxt, sizeof(gNID.szTip) );
		lstrcpyn( gNID.szInfo, gbUse ? pszOn : pszOff, sizeof(gNID.szTip) );
	}

	//Add the notification to the tray
	Shell_NotifyIcon( bCreate ? NIM_ADD : NIM_MODIFY, &gNID );
}

//////////////////////////////////////////////////////////////////////////

VOID CALLBACK TimerProc( HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime )
{
	KillTimer( ghWnd, ipTID );
	ipTID = NULL;

	ShowTraiIcon( false, false );
}

//////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK LowLevelMouseProc( int nCode, WPARAM wParam, LPARAM lParam )
{
	LRESULT lR = 0;

	if( gbUse && nCode >= 0 && wParam == WM_MOUSEWHEEL )
	{
		MSLLHOOKSTRUCT & msll = *LPMSLLHOOKSTRUCT(lParam);

		HWND hWnd = WindowFromPoint( msll.pt );

		if( IsWindow( hWnd ) )
		{
			    lR = PostMessage( hWnd, wParam, msll.mouseData, MAKELONG( msll.pt.x, msll.pt.y ) );
			if( lR )
			{
				KillTimer( ghWnd, ipTID );

				ShowTraiIcon( false, false );

				ipTID = SetTimer( ghWnd, 100, 200, TimerProc );
			}
			else
				Beep( 4000, 200 );
		}
	}

	if( nCode < 0 || lR == 0 )
		lR = CallNextHookEx( hHook, nCode, wParam, lParam );

	return lR;
}

//////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK WindowProc( HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
	LRESULT lR = -1;

	if( hwnd == ghWnd )
	{
		if( uMsg == guMsg )
		{
			if( lParam == WM_LBUTTONDOWN
			 || lParam == WM_RBUTTONDOWN )
			{
				if( IsWindowVisible( ghWnd ) )
				{
					SendMessage( ghWnd, WM_COMMAND, IDC_CLOSE, 0 );
				}
			}
			else
			if( lParam == WM_RBUTTONUP )
			{
				POINT          pt  ;
				GetCursorPos( &pt );

				HMENU hSub = ghMenu ? GetSubMenu( ghMenu, 0 ) : NULL;
				if(   hSub ) 
				{
					EnableMenuItem( hSub, IDC_SCROLLER  , MF_BYCOMMAND |           MF_GRAYED  | MF_DISABLED  );
					EnableMenuItem( hSub, IDC_SWITCH_ON , MF_BYCOMMAND | ( gbUse ? MF_GRAYED  : MF_ENABLED ) );
					EnableMenuItem( hSub, IDC_SWITCH_OFF, MF_BYCOMMAND | ( gbUse ? MF_ENABLED : MF_GRAYED  ) );

					UINT  uFlags = TPM_BOTTOMALIGN | TPM_CENTERALIGN | TPM_VERNEGANIMATION | TPM_RIGHTBUTTON;

					SetForegroundWindow( ghWnd );
					TrackPopupMenu( hSub, uFlags, pt.x, pt.y, NULL, ghWnd, NULL );
					PostMessage( ghWnd, WM_NULL, 0, 0 );
				}
				else
				{
					if( MessageBox( ghWnd, pszMsg, pszTxt, MB_ICONQUESTION | MB_YESNO ) == IDYES )
						PostThreadMessage( dwThr, WM_QUIT, 0, 0 );
				}
			}
			else
			if( lParam == WM_LBUTTONUP )
			{
				gbUse = !gbUse;

				ShowTraiIcon( false, true );
			}

			lR = 1;
		}
		else
		if( uMsg == WM_COMMAND )
		{
			if( wParam == IDCANCEL )
			{
				PostThreadMessage( dwThr, WM_QUIT, 0, 0 );
			}
			else
			if( wParam == IDC_SWITCH_ON
			 || wParam == IDC_SWITCH_OFF )
			{
				PostMessage( ghWnd, guMsg, 0, WM_LBUTTONUP );
			}
			else
			if( wParam == IDC_ABOUT )
			{
				ShowWindow( ghWnd, SW_SHOWNORMAL );
			}
			else
			if( wParam == IDC_CLOSE )
			{
				ShowWindow( ghWnd, SW_HIDE );
			}

			lR = 2;
		}
		else
		if( uMsg == WM_LBUTTONUP
		 || uMsg == WM_RBUTTONUP )
		{
			if( IsWindowVisible( ghWnd ) )
			{
				ShowWindow( ghWnd, SW_HIDE );
			}

			lR = 3;
		}
		else
		if( uMsg == WM_PAINT )
		{
			PAINTSTRUCT                   ps  ;
			HDC hDC = BeginPaint( ghWnd, &ps );

			DrawIconEx( hDC, 0, 0, ghIInf, 320, 240, 0, NULL, DI_NORMAL );

			EndPaint( ghWnd, &ps );
			lR = 4;
		}
	}

	if( lR < 0 )
		lR = DefWindowProc( hwnd, uMsg, wParam, lParam );

	return lR;
}

//////////////////////////////////////////////////////////////////////////

void CreateMainWindow( HINSTANCE hInstance )
{
	dwThr = GetCurrentThreadId();
	guMsg = RegisterWindowMessage( pszTxt );

	ghIOn  = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_ICON1 ) );
	ghIOff = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_ICON2 ) );
	ghIUse = LoadIcon( hInstance, MAKEINTRESOURCE( IDI_ICON3 ) );

	ghIInf = (HICON)LoadImage ( hInstance, MAKEINTRESOURCE( IDI_ICON4   ), IMAGE_ICON, 320, 240, 0 );

	ghMenu = LoadMenu( hInstance, MAKEINTRESOURCE( IDR_MENU1 ) );

	ghBOn  = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP1 ) );
	ghBOff = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP2 ) );
	ghBInf = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP3 ) );
	ghBEnd = LoadBitmap( hInstance, MAKEINTRESOURCE( IDB_BITMAP4 ) );

	HMENU hSub = ghMenu ? GetSubMenu( ghMenu, 0 ) : NULL;
	if(   hSub ) 
	{
		MENUITEMINFO mii = { sizeof(mii), MIIM_CHECKMARKS, 0 };

		mii.hbmpChecked = ghBInf; SetMenuItemInfo( hSub, IDC_ABOUT     , FALSE, &mii );
		mii.hbmpChecked = ghBOn ; SetMenuItemInfo( hSub, IDC_SWITCH_ON , FALSE, &mii );
		mii.hbmpChecked = ghBOff; SetMenuItemInfo( hSub, IDC_SWITCH_OFF, FALSE, &mii );
		mii.hbmpChecked = ghBEnd; SetMenuItemInfo( hSub, IDCANCEL      , FALSE, &mii );
	}

	int bx = GetSystemMetrics( SM_CXEDGE    );
	int by = GetSystemMetrics( SM_CYEDGE    );
	int ty = GetSystemMetrics( SM_CYCAPTION );

	int sx = 320 + bx * 4;
	int sy = 240 + by * 4 + ty;

	int wx = GetSystemMetrics( SM_CXFULLSCREEN ) / 2 - sx / 2;
	int wy = GetSystemMetrics( SM_CYFULLSCREEN ) / 2 - sy / 2;

	ghWnd = CreateWindowEx( WS_EX_TOOLWINDOW | WS_EX_CLIENTEDGE | WS_EX_NOPARENTNOTIFY | WS_EX_APPWINDOW | WS_EX_TOPMOST /*| WS_EX_RIGHT*/, _T("STATIC"), pszTxt, WS_CLIPSIBLINGS | WS_CAPTION, wx, wy, sx, sy, ::GetDesktopWindow(), NULL, hInstance, NULL );
	
	RECT                     rcCli  ;
	::GetClientRect( ghWnd, &rcCli );

	if( IsWindow( ghWnd ) )
		SetWindowLong( ghWnd, GWL_WNDPROC, LONG(WindowProc) );

	ShowTraiIcon( true, true );
}

//////////////////////////////////////////////////////////////////////////

int APIENTRY _tWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow )
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	SECURITY_ATTRIBUTES           sa = { sizeof(sa), NULL,   TRUE };
	HANDLE hMutex = CreateMutex( &sa               , TRUE, pszTxt );

	if( GetLastError() == ERROR_ALREADY_EXISTS )
		MessageBox( NULL, pszErr, pszTxt, MB_ICONWARNING | MB_OK );
	else
	{
		DWORD  dwPID = GetCurrentProcessId();
		HANDLE hPID  = OpenProcess( GENERIC_ALL, FALSE, dwPID );

		SetPriorityClass( hPID, ABOVE_NORMAL_PRIORITY_CLASS );

		CreateMainWindow( hInstance );

		hHook = SetWindowsHookEx( WH_MOUSE_LL, LowLevelMouseProc, hInstance, NULL );
		gbUse = true;

		ShowTraiIcon( false, true );

		MSG msg;

		while( GetMessage( &msg, 0, 0, 0 ) )
		{
			TranslateMessage( &msg );
			DispatchMessage( &msg );
		}

		KillTimer( ghWnd, ipTID );

		if( hHook )
			UnhookWindowsHookEx( hHook );

		Shell_NotifyIcon( NIM_DELETE, &gNID );

		if( ghMenu )
			DestroyMenu( ghMenu );

		if( IsWindow( ghWnd ) )
			DestroyWindow( ghWnd ) ;
	}

	ReleaseMutex( hMutex );
	CloseHandle ( hMutex );

	return 0;
}

//////////////////////////////////////////////////////////////////////////
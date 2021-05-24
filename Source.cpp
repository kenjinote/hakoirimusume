#pragma comment(lib, "gdiplus")

#include <windows.h>
#include <gdiplus.h>
#include "resource.h"

TCHAR szClassName[] = TEXT("Window");

#define MASU_WIDTH 32
#define MASU_HEIGHT 32

#define X_NUM 10
#define Y_NUM 12

#define WINDOW_WIDTH (MASU_WIDTH*X_NUM)
#define WINDOW_HEIGHT (MASU_HEIGHT*Y_NUM)

int* g_x, * g_y;
int* g_sx, * g_sy;
int* g_imgno;
int* g_field;

Gdiplus::Image* LoadImageFromResource(HMODULE hModule, LPCWSTR lpName, LPCWSTR lpType)
{
	const HRSRC hResInfo = FindResource(hModule, lpName, lpType);
	if (hResInfo)
	{
		const int nSize = SizeofResource(hModule, hResInfo);
		if (nSize)
		{
			const HGLOBAL hResData = LoadResource(hModule, hResInfo);
			if (hResData)
			{
				const void* pData = (void*)LockResource(hResData);
				if (pData)
				{
					IStream* pIStream;
					ULARGE_INTEGER LargeUInt;
					LARGE_INTEGER LargeInt;
					Gdiplus::Image* pImage;
					CreateStreamOnHGlobal(0, 1, &pIStream);
					LargeUInt.QuadPart = nSize;
					pIStream->SetSize(LargeUInt);
					LargeInt.QuadPart = 0;
					pIStream->Seek(LargeInt, STREAM_SEEK_SET, NULL);
					pIStream->Write(pData, nSize, NULL);
					pImage = Gdiplus::Image::FromStream(pIStream);
					pIStream->Release();
					if (pImage)
					{
						if (pImage->GetLastStatus() == Gdiplus::Ok)
						{
							return pImage;
						}
						delete pImage;
					}
				}
			}
		}
	}
	return 0;
}
void clear_field(int nNo)
{
	for (int xx = 0; xx < g_sx[nNo]; xx++)
	{
		for (int yy = 0; yy < g_sy[nNo]; yy++)
		{
			g_field[g_x[nNo] + xx + (g_y[nNo] + yy) * X_NUM] = 0;
		}
	}
}

void put_field(int nNo, LPBOOL pbIsWin)
{
	for (int xx = 0; xx < g_sx[nNo]; xx++)
	{
		for (int yy = 0; yy < g_sy[nNo]; yy++)
		{
			g_field[g_x[nNo] + xx + (g_y[nNo] + yy) * X_NUM] = nNo;
		}
	}
	if (g_x[1] == 3 && g_y[1] == 7)*pbIsWin = TRUE;
}

BOOL DownKoma(int nNo, int xx, int yy, LPBOOL pbIsWin)
{
	for (int i = 0; i < g_sx[nNo]; i++)
	{
		if (g_field[(g_x[nNo] + i) + (g_y[nNo] + g_sy[nNo]) * X_NUM] != 0)
		{
			return FALSE;
		}
	}
	clear_field(nNo);
	g_y[nNo]++;
	put_field(nNo, pbIsWin);
	return TRUE;
}

BOOL UpKoma(int nNo, int xx, int yy, LPBOOL pbIsWin)
{
	for (int i = 0; i < g_sx[nNo]; i++)
	{
		if (g_field[(g_x[nNo] + i) + (g_y[nNo] - 1) * X_NUM] != 0)
		{
			return FALSE;
		}
	}
	clear_field(nNo);
	g_y[nNo]--;
	put_field(nNo, pbIsWin);
	return TRUE;
}

BOOL RightKoma(int nNo, int xx, int yy, LPBOOL pbIsWin)
{
	for (int i = 0; i < g_sy[nNo]; i++)
	{
		if (g_field[(g_x[nNo] + g_sx[nNo]) + (g_y[nNo] + i) * X_NUM] != 0)
		{
			return FALSE;
		}
	}
	clear_field(nNo);
	g_x[nNo]++;
	put_field(nNo, pbIsWin);
	return TRUE;
}

BOOL LeftKoma(int nNo, int xx, int yy, LPBOOL pbIsWin)
{
	for (int i = 0; i < g_sy[nNo]; i++)
	{
		if (g_field[(g_x[nNo] - 1) + (g_y[nNo] + i) * X_NUM] != 0)
		{
			return FALSE;
		}
	}
	clear_field(nNo);
	g_x[nNo]--;
	put_field(nNo, pbIsWin);
	return TRUE;
}

void Level1(int* pnLast)
{
	const int m_x[] = { 0,3,1,7,1,7,3,5,3,5,1,7 };
	const int m_y[] = { 0,1,1,1,5,5,5,5,7,7,9,9 };
	const int m_sx[] = { 0,4,2,2,2,2,2,2,2,2,2,2 };
	const int m_sy[] = { 0,4,4,4,4,4,2,2,2,2,2,2 };
	const int m_imgno[] = { 0,1,2,3,4,5,11,11,11,11,11,11 };
	*pnLast = 12;
	for (int i = 0; i < *pnLast; i++)
	{
		g_x[i] = m_x[i];
		g_y[i] = m_y[i];
		g_sx[i] = m_sx[i];
		g_sy[i] = m_sy[i];
		g_imgno[i] = m_imgno[i];
	}
}

void Level2(int* pnLast)
{
	const int m_x[] = { 0,3,1,5,1,5,1,1,7,7,1,7 };
	const int m_y[] = { 0,1,5,5,7,7,1,3,1,3,9,9 };
	const int m_sx[] = { 0,4,4,4,4,4,2,2,2,2,2,2 };
	const int m_sy[] = { 0,4,2,2,2,2,2,2,2,2,2,2 };
	const int m_imgno[] = { 0,1,6,7,8,9,11,11,11,11,11,11 };
	*pnLast = 12;
	for (int i = 0; i < *pnLast; i++)
	{
		g_x[i] = m_x[i];
		g_y[i] = m_y[i];
		g_sx[i] = m_sx[i];
		g_sy[i] = m_sy[i];
		g_imgno[i] = m_imgno[i];
	}
}

void Level3(int* pnLast)
{
	const int m_x[] = { 0,3,1,7,1,5,1,3,5,7,1,7 };
	const int m_y[] = { 0,1,1,1,7,7,5,5,5,5,9,9 };
	const int m_sx[] = { 0,4,2,2,4,4,2,2,2,2,2,2 };
	const int m_sy[] = { 0,4,4,4,2,2,2,2,2,2,2,2 };
	const int m_imgno[] = { 0,1,2,3,8,9,11,11,11,11,11,11 };
	*pnLast = 12;
	for (int i = 0; i < *pnLast; i++)
	{
		g_x[i] = m_x[i];
		g_y[i] = m_y[i];
		g_sx[i] = m_sx[i];
		g_sy[i] = m_sy[i];
		g_imgno[i] = m_imgno[i];
	}
}

void Level4(int* pnLast)
{
	const int m_x[] = { 0,3,1,7,1,5,1,3,5,7,1,7 };
	const int m_y[] = { 0,1,1,1,5,5,7,7,7,7,9,9 };
	const int m_sx[] = { 0,4,2,2,4,4,2,2,2,2,2,2 };
	const int m_sy[] = { 0,4,4,4,2,2,2,2,2,2,2,2 };
	const int m_imgno[] = { 0,1,2,3,8,9,11,11,11,11,11,11 };
	*pnLast = 12;
	for (int i = 0; i < *pnLast; i++)
	{
		g_x[i] = m_x[i];
		g_y[i] = m_y[i];
		g_sx[i] = m_sx[i];
		g_sy[i] = m_sy[i];
		g_imgno[i] = m_imgno[i];
	}
}

void Level5(int* pnLast)
{
	const int m_x[] = { 0,3,1,7,1,7,3,3,5,1,7 };
	const int m_y[] = { 0,1,1,1,5,5,5,7,7,9,9 };
	const int m_sx[] = { 0,4,2,2,2,2,4,2,2,2,2 };
	const int m_sy[] = { 0,4,4,4,4,4,2,2,2,2,2 };
	const int m_imgno[] = { 0,1,2,3,4,5,10,11,11,11,11 };
	*pnLast = 11;
	for (int i = 0; i < *pnLast; i++)
	{
		g_x[i] = m_x[i];
		g_y[i] = m_y[i];
		g_sx[i] = m_sx[i];
		g_sy[i] = m_sy[i];
		g_imgno[i] = m_imgno[i];
	}
}

//レベル(nLevel)に応じてゲーム画面を初期化する
VOID initGame(int nLevel, int* pnLast)
{
	switch (nLevel)
	{
	case 1:Level1(pnLast); break;
	case 2:Level2(pnLast); break;
	case 3:Level3(pnLast); break;
	case 4:Level4(pnLast); break;
	case 5:Level5(pnLast); break;
	default:Level1(pnLast); break;
	}
	for (int i = 0; i < (X_NUM * Y_NUM); i++)g_field[i] = 0;
	for (int i = 0; i < X_NUM; i++)g_field[i] = 99;
	for (int i = 0; i < X_NUM; i++)g_field[i + 110] = 99;
	for (int i = 0; i < Y_NUM; i++)g_field[X_NUM * i] = 99;
	for (int i = 0; i < Y_NUM; i++)g_field[9 + X_NUM * i] = 99;
	for (int i = 1; i < *pnLast; i++)
	{
		for (int xx = 0; xx < g_sx[i]; xx++)
		{
			for (int yy = 0; yy < g_sy[i]; yy++)
			{
				g_field[g_x[i] + xx + (g_y[i] + yy) * X_NUM] = i;
			}
		}
	}
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	int i;
	static BOOL bCapture;
	static int nStage = 1;
	static BOOL bSetTimered;
	static Gdiplus::Image* pImage[14];
	static BOOL bIsWin;
	static int nLast;
	static int nNo;
	static int g_oldx, g_oldy;
	switch (msg)
	{
	case WM_CREATE:
		g_field = (int*)GlobalAlloc(GMEM_FIXED, sizeof(int) * X_NUM * Y_NUM);
		g_x = (int*)GlobalAlloc(GMEM_FIXED, sizeof(int) * Y_NUM);
		g_y = (int*)GlobalAlloc(GMEM_FIXED, sizeof(int) * Y_NUM);
		g_sx = (int*)GlobalAlloc(GMEM_FIXED, sizeof(int) * Y_NUM);
		g_sy = (int*)GlobalAlloc(GMEM_FIXED, sizeof(int) * Y_NUM);
		g_imgno = (int*)GlobalAlloc(GMEM_FIXED, sizeof(int) * Y_NUM);
		for (i = 0; i < 14; i++)
		{
			pImage[i] = LoadImageFromResource(((LPCREATESTRUCT)lParam)->hInstance, MAKEINTRESOURCE(IDB_PNG1 + i), TEXT("PNG"));
		}
		initGame(nStage, &nLast);
		break;
	case WM_ERASEBKGND:
		return 1;
	case WM_LBUTTONDOWN:
		if (bIsWin == FALSE)
		{
			const WORD xPos = LOWORD(lParam);
			const WORD yPos = HIWORD(lParam);
			g_oldx = xPos;
			g_oldy = yPos;
			nNo = g_field[yPos / MASU_HEIGHT * X_NUM + xPos / MASU_WIDTH];
			if (nNo == 99)
			{
				nNo = 0;
			}
			else if (nNo)
			{
				bCapture = TRUE;
				SetCapture(hWnd);
			}
		}
		break;
	case WM_MOUSEMOVE:
		if (bCapture)
		{
			if (nNo)
			{
				const WORD xPos = LOWORD(lParam);
				const WORD yPos = HIWORD(lParam);
				if (
					yPos - g_oldy > MASU_HEIGHT * 20 / 32 && DownKoma(nNo, xPos, yPos, &bIsWin) ||
					g_oldy - yPos > MASU_HEIGHT * 20 / 32 && UpKoma(nNo, xPos, yPos, &bIsWin) ||
					xPos - g_oldx > MASU_WIDTH * 20 / 32 && RightKoma(nNo, xPos, yPos, &bIsWin) ||
					g_oldx - xPos > MASU_WIDTH * 20 / 32 && LeftKoma(nNo, xPos, yPos, &bIsWin)
					)
				{
					g_oldx = xPos;
					g_oldy = yPos;
					InvalidateRect(hWnd, 0, 0);
				}
			}
		}
		break;
	case WM_LBUTTONUP:
		if (bCapture)
		{
			nNo = 0;
			ReleaseCapture();
			bCapture = FALSE;
		}
		break;
	case WM_TIMER:
		KillTimer(hWnd, 0x1234);
		nStage++;
		if (nStage >= 6)
		{
			MessageBox(hWnd,
				TEXT("全面クリアーしました。\nあなたはすごい！"),
				TEXT("おめでとう！"), 0);
			PostMessage(hWnd, WM_CLOSE, 0, 0);
		}
		else
		{
			bIsWin = FALSE;
			bSetTimered = FALSE;
			TCHAR szTitle[64];
			wsprintf(szTitle, TEXT("箱入り娘。（レベル %d）"), nStage);
			SetWindowText(hWnd, szTitle);
			initGame(nStage, &nLast);
			InvalidateRect(hWnd, 0, 0);
		}
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		const HDC hdc = BeginPaint(hWnd, &ps);
		Gdiplus::Graphics graphics(hdc);
		if (bIsWin == TRUE)
		{
			//クリアーしたとき「見事」と2秒間表示する
			graphics.DrawImage(pImage[12], 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
			if (!bSetTimered)
			{
				bSetTimered = TRUE;
				SetTimer(hWnd, 0x1234, 1000 * 2, NULL);
			}
		}
		else
		{
			graphics.DrawImage(pImage[0], 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
			for (i = 1; i < nLast; i++)
			{
				graphics.DrawImage(pImage[g_imgno[i]], g_x[i] * MASU_WIDTH, g_y[i] * MASU_HEIGHT);
			}
		}
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_DESTROY:
		KillTimer(hWnd, 0x1234);
		for (i = 0; i < 14; i++)
		{
			delete pImage[i];
		}
		GlobalFree(g_field);
		GlobalFree(g_x);
		GlobalFree(g_y);
		GlobalFree(g_sx);
		GlobalFree(g_sy);
		GlobalFree(g_imgno);
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	ULONG_PTR gdiToken;
	Gdiplus::GdiplusStartupInput gdiSI;
	Gdiplus::GdiplusStartup(&gdiToken, &gdiSI, 0);
	MSG msg;
	const WNDCLASS wndclass = { 0,WndProc,0,0,hInstance,0,LoadCursor(0,IDC_ARROW),0,0,szClassName };
	RegisterClass(&wndclass);

	RECT rect;
	SetRect(&rect, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, 0);

	const HWND hWnd = CreateWindowEx(WS_EX_COMPOSITED, szClassName, TEXT("箱入り娘。（レベル 1）"),
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0, 0, hInstance, 0);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	Gdiplus::GdiplusShutdown(gdiToken);
	return (int)msg.wParam;
}

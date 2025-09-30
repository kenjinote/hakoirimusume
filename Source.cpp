#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wincodec.h>
#include <sstream>

// Direct2DおよびWICのライブラリをリンク
#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "dwrite.lib")
#pragma comment(lib, "windowscodecs.lib")

#include "resource.h" // リソース定義ファイル (IDB_PNG1など)

// インターフェースポインターを安全に解放するマクロ
template <class T> inline void SafeRelease(T** ppT)
{
	if (*ppT != NULL)
	{
		(*ppT)->Release();
		*ppT = NULL;
	}
}

TCHAR szClassName[] = TEXT("Window");

#define MASU_WIDTH 32
#define MASU_HEIGHT 32

#define X_NUM 10
#define Y_NUM 12

#define WINDOW_WIDTH (MASU_WIDTH*X_NUM)
#define WINDOW_HEIGHT (MASU_HEIGHT*Y_NUM)

// --- グローバルゲーム変数 (元のコードから維持) ---
int* g_x = nullptr;
int* g_y = nullptr;
int* g_sx = nullptr;
int* g_sy = nullptr;
int* g_imgno = nullptr;
int* g_field = nullptr;
int g_nLast = 0; // 駒の総数
int g_nStage = 1; // 現在のレベル

// --- Direct2D/WIC/DWrite グローバル変数 ---
ID2D1Factory* pD2DFactory = nullptr;
IWICImagingFactory* pWICFactory = nullptr;
ID2D1HwndRenderTarget* pRT = nullptr;
ID2D1Bitmap* pBitmap[14] = { nullptr }; // Direct2Dビットマップを格納する配列
ID2D1SolidColorBrush* pTextBrush = nullptr;
IDWriteFactory* pDWriteFactory = nullptr;
IDWriteTextFormat* pTextFormat = nullptr;

// --------------------------------------------------------------------------------
// --- WIC/Direct2D リソース管理ヘルパー関数 ---
// --------------------------------------------------------------------------------

// WICとDirect2Dを使用してリソースからビットマップを読み込む
HRESULT LoadBitmapFromResource(
	ID2D1RenderTarget* pRenderTarget,
	IWICImagingFactory* pIWICFactory,
	HMODULE hModule,
	LPCWSTR lpName,
	LPCWSTR lpType,
	ID2D1Bitmap** ppBitmap
)
{
	HRSRC hResInfo = FindResource(hModule, lpName, lpType);
	if (!hResInfo) return E_FAIL;

	DWORD nSize = SizeofResource(hModule, hResInfo);
	if (!nSize) return E_FAIL;

	HGLOBAL hResData = LoadResource(hModule, hResInfo);
	if (!hResData) return E_FAIL;

	void* pData = LockResource(hResData);
	if (!pData) return E_FAIL;

	// IStreamを作成
	IStream* pStream = nullptr;
	HRESULT hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
	if (SUCCEEDED(hr))
	{
		// リソースデータをストリームに書き込む
		hr = pStream->Write(pData, nSize, NULL);
	}

	IWICBitmapDecoder* pDecoder = nullptr;
	IWICBitmapFrameDecode* pSource = nullptr;
	IWICFormatConverter* pConverter = nullptr;

	if (SUCCEEDED(hr))
	{
		// ストリームからWICデコーダーを作成
		hr = pIWICFactory->CreateDecoderFromStream(pStream, NULL, WICDecodeMetadataCacheOnLoad, &pDecoder);
	}

	if (SUCCEEDED(hr))
	{
		// 最初のフレームを取得
		hr = pDecoder->GetFrame(0, &pSource);
	}

	if (SUCCEEDED(hr))
	{
		// フォーマットコンバーターを作成
		hr = pIWICFactory->CreateFormatConverter(&pConverter);
	}

	if (SUCCEEDED(hr))
	{
		// RENDER TARGETのピクセルフォーマットに変換
		hr = pConverter->Initialize(
			pSource,
			GUID_WICPixelFormat32bppPBGRA, // D2D1_ALPHA_MODE_PREMULTIPLIED に対応
			WICBitmapDitherTypeNone,
			NULL,
			0.0,
			WICBitmapPaletteTypeCustom
		);
	}

	if (SUCCEEDED(hr))
	{
		// Direct2Dビットマップを作成
		hr = pRenderTarget->CreateBitmapFromWicBitmap(pConverter, NULL, ppBitmap);
	}

	// WICオブジェクトの解放
	SafeRelease(&pStream);
	SafeRelease(&pDecoder);
	SafeRelease(&pSource);
	SafeRelease(&pConverter);

	return hr;
}

// ハードウェアに依存するリソースを作成
HRESULT CreateDeviceResources(HWND hWnd)
{
	HRESULT hr = S_OK;

	if (!pRT)
	{
		RECT rc;
		GetClientRect(hWnd, &rc);

		D2D1_SIZE_U size = D2D1::SizeU(rc.right - rc.left, rc.bottom - rc.top);

		// HWNDレンダーターゲットを作成
		hr = pD2DFactory->CreateHwndRenderTarget(
			D2D1::RenderTargetProperties(),
			D2D1::HwndRenderTargetProperties(hWnd, size),
			&pRT
		);

		if (SUCCEEDED(hr))
		{
			// テキストブラシを作成
			hr = pRT->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &pTextBrush);
		}

		if (SUCCEEDED(hr))
		{
			// 全ての画像をDirect2Dビットマップとして読み込む
			HMODULE hModule = (HMODULE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
			for (int i = 0; i < 14; i++)
			{
				hr = LoadBitmapFromResource(pRT, pWICFactory, hModule, MAKEINTRESOURCE(IDB_PNG1 + i), TEXT("PNG"), &pBitmap[i]);
				if (FAILED(hr)) break;
			}
		}
	}
	return hr;
}

// ハードウェアに依存するリソースを破棄
void DiscardDeviceResources()
{
	SafeRelease(&pRT);
	SafeRelease(&pTextBrush);
	for (int i = 0; i < 14; i++)
	{
		SafeRelease(&pBitmap[i]);
	}
}

// --------------------------------------------------------------------------------
// --- ゲームロジック関数 (元のコードから維持) ---
// --------------------------------------------------------------------------------

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

// --------------------------------------------------------------------------------
// --- ウィンドウプロシージャ (Direct2D描画に変更) ---
// --------------------------------------------------------------------------------

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static BOOL bCapture;
	static BOOL bSetTimered;
	static BOOL bIsWin = FALSE;
	static int nNo;
	static int g_oldx, g_oldy;
	HRESULT hr;

	switch (msg)
	{
	case WM_CREATE:
		// Direct2D/WIC/DWrite ファクトリの初期化
		hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
		if (FAILED(hr)) return -1;

		hr = CoInitialize(NULL); // WICのためにCOMを初期化
		if (FAILED(hr)) return -1;
		hr = CoCreateInstance(CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pWICFactory));
		if (FAILED(hr)) return -1;

		hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&pDWriteFactory));
		if (FAILED(hr)) return -1;

		// DWrite TextFormatの作成
		hr = pDWriteFactory->CreateTextFormat(
			L"Arial",                   // フォント名
			NULL,                       // フォントコレクション (NULL = システムデフォルト)
			DWRITE_FONT_WEIGHT_BOLD,    // フォントの太さ
			DWRITE_FONT_STYLE_NORMAL,   // フォントスタイル
			DWRITE_FONT_STRETCH_NORMAL, // フォントストレッチ
			48.0f,                      // フォントサイズ
			L"ja-jp",                   // ロケール
			&pTextFormat
		);
		if (SUCCEEDED(hr))
		{
			pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER);
			pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_CENTER);
		}

		// ゲームメモリの割り当て
		g_field = (int*)GlobalAlloc(GMEM_FIXED, sizeof(int) * X_NUM * Y_NUM);
		g_x = (int*)GlobalAlloc(GMEM_FIXED, sizeof(int) * Y_NUM);
		g_y = (int*)GlobalAlloc(GMEM_FIXED, sizeof(int) * Y_NUM);
		g_sx = (int*)GlobalAlloc(GMEM_FIXED, sizeof(int) * Y_NUM);
		g_sy = (int*)GlobalAlloc(GMEM_FIXED, sizeof(int) * Y_NUM);
		g_imgno = (int*)GlobalAlloc(GMEM_FIXED, sizeof(int) * Y_NUM);

		// ゲーム初期化
		initGame(g_nStage, &g_nLast);
		break;

	case WM_SIZE:
		// サイズ変更時はRenderTargetを破棄して再作成
		DiscardDeviceResources();
		InvalidateRect(hWnd, NULL, FALSE);
		break;

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
				// 移動量が多い場合にのみ移動処理を実行
				if (
					yPos - g_oldy > MASU_HEIGHT * 20 / 32 && DownKoma(nNo, xPos, yPos, &bIsWin) ||
					g_oldy - yPos > MASU_HEIGHT * 20 / 32 && UpKoma(nNo, xPos, yPos, &bIsWin) ||
					xPos - g_oldx > MASU_WIDTH * 20 / 32 && RightKoma(nNo, xPos, yPos, &bIsWin) ||
					g_oldx - xPos > MASU_WIDTH * 20 / 32 && LeftKoma(nNo, xPos, yPos, &bIsWin)
					)
				{
					g_oldx = xPos;
					g_oldy = yPos;
					InvalidateRect(hWnd, 0, FALSE); // GDIのWM_ERASEBKGND回避のためFALSE
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
		g_nStage++;
		if (g_nStage >= 6)
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
			wsprintf(szTitle, TEXT("箱入り娘。（レベル %d）"), g_nStage);
			SetWindowText(hWnd, szTitle);
			initGame(g_nStage, &g_nLast);
			InvalidateRect(hWnd, 0, FALSE);
		}
		break;

	case WM_PAINT:
	{
		// Direct2D描画
		hr = CreateDeviceResources(hWnd);
		if (SUCCEEDED(hr))
		{
			PAINTSTRUCT ps;
			BeginPaint(hWnd, &ps);

			pRT->BeginDraw();

			// 背景をクリア (白)
			pRT->Clear(D2D1::ColorF(D2D1::ColorF::White));

			if (bIsWin == TRUE)
			{
				// クリアーしたとき「見事」と2秒間表示する
				if (pBitmap[12])
				{
					// 勝利画面の背景
					pRT->DrawBitmap(pBitmap[12], D2D1::RectF(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT));
				}

				// DirectWriteで「見事」を描画
				if (pTextBrush && pTextFormat)
				{
					pTextBrush->SetColor(D2D1::ColorF(D2D1::ColorF::Black));
					pRT->DrawText(
						L"見事！\n次のレベルへ",
						(UINT32)wcslen(L"見事！\n次のレベルへ"),
						pTextFormat,
						D2D1::RectF(0, WINDOW_HEIGHT / 4.0f, WINDOW_WIDTH, WINDOW_HEIGHT / 2.0f),
						pTextBrush
					);
				}

				if (!bSetTimered)
				{
					bSetTimered = TRUE;
					SetTimer(hWnd, 0x1234, 1000 * 2, NULL);
				}
			}
			else
			{
				// 背景画像 (0番)
				if (pBitmap[0])
				{
					pRT->DrawBitmap(pBitmap[0], D2D1::RectF(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT));
				}

				// 各駒を描画
				for (int i = 1; i < g_nLast; i++)
				{
					int imgIndex = g_imgno[i];
					if (pBitmap[imgIndex])
					{
						D2D1_RECT_F destRect = D2D1::RectF(
							(FLOAT)g_x[i] * MASU_WIDTH,
							(FLOAT)g_y[i] * MASU_HEIGHT,
							(FLOAT)(g_x[i] + g_sx[i]) * MASU_WIDTH,
							(FLOAT)(g_y[i] + g_sy[i]) * MASU_HEIGHT
						);
						pRT->DrawBitmap(pBitmap[imgIndex], destRect);
					}
				}
			}

			hr = pRT->EndDraw();

			// デバイスが消失した場合、リソースを破棄
			if (hr == D2DERR_RECREATE_TARGET)
			{
				hr = S_OK;
				DiscardDeviceResources();
			}

			EndPaint(hWnd, &ps);
		}
	}
	break;

	case WM_DESTROY:
		KillTimer(hWnd, 0x1234);

		// Direct2D/WIC/DWrite リソースの解放
		DiscardDeviceResources();
		SafeRelease(&pD2DFactory);
		SafeRelease(&pWICFactory);
		SafeRelease(&pDWriteFactory);
		SafeRelease(&pTextFormat);
		CoUninitialize(); // COMの終了

		// ゲームメモリの解放
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

// --------------------------------------------------------------------------------
// --- WinMain (GDI+の初期化/終了を削除) ---
// --------------------------------------------------------------------------------

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPWSTR pCmdLine, int nCmdShow)
{
	// GDI+ の初期化/終了コードは削除しました

	MSG msg;
	const WNDCLASS wndclass = { 0,WndProc,0,0,hInstance,0,LoadCursor(0,IDC_ARROW),(HBRUSH)(COLOR_WINDOW + 1),0,szClassName };
	if (!RegisterClass(&wndclass)) return 0;

	RECT rect;
	SetRect(&rect, 0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
	AdjustWindowRect(&rect, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU, FALSE);

	const HWND hWnd = CreateWindowEx(0, szClassName, TEXT("箱入り娘。（レベル 1）"), // WS_EX_COMPOSITED はDirect2Dでは不要
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0, 0, hInstance, 0);

	if (!hWnd) return 0;

	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);

	while (GetMessage(&msg, 0, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return (int)msg.wParam;
}

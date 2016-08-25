/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include <Windows.h>
#include <comdef.h>
#include <strsafe.h>
#include "..\include\libcore.h"

_declspec(dllexport) void PrintComError(HRESULT hr, TCHAR *pszExtra)
{
    _com_error err(hr);
    LPCTSTR szErrorText = err.ErrorMessage();
    TCHAR szDump[MAX_PATH];
    StringCchPrintf(szDump, 100, L"%s: %s (0x%x)\n", pszExtra, szErrorText, hr);
    OutputDebugString(szDump);
}

_declspec(dllexport) HRESULT GetComTextError(HRESULT hr, wchar_t *pszOut, DWORD *pcchLen)
{
    if (!pcchLen) return E_INVALIDARG;
    if (*pcchLen < 1) return E_INVALIDARG;

    TCHAR szTrace[MAX_PATH];
    _com_error err(hr);
    LPCTSTR szErrorText = err.ErrorMessage();

    StringCchPrintf(szTrace, *pcchLen, L"%s (0x%x)", szErrorText, hr);

    return StringCchCopy(pszOut, *pcchLen, szTrace);
}

static BYTE Clip(int clr)
{
    return (BYTE)(clr < 0 ? 0 : (clr > 255 ? 255 : clr));
}

static RGBQUAD ConvertYCrCbToRGB(int y, int cr, int cb)
{
    RGBQUAD rgbq = { 0 };

    int c = y - 16;
    int d = cb - 128;
    int e = cr - 128;

    rgbq.rgbRed = Clip((298 * c + 409 * e + 128) >> 8);
    rgbq.rgbGreen = Clip((298 * c - 100 * d - 208 * e + 128) >> 8);
    rgbq.rgbBlue = Clip((298 * c + 516 * d + 128) >> 8);

    return rgbq;
}

_declspec(dllexport) void FromYUY2ToRGB32(LPVOID lpDest, LPVOID lpSource, LONG lWidth, LONG lHeight)
{
    RGBQUAD *pDestPel = (RGBQUAD*)lpDest;
    WORD *pSrcPel = (WORD*)lpSource;

    for (int y = 0; y < lHeight; y++)
    {
        for (int x = 0; x < lWidth; x += 2)
        {
            //
            // Byte order is U0 Y0 V0 Y1
            //
            int y0 = (int)LOBYTE(pSrcPel[x]);
            int u0 = (int)HIBYTE(pSrcPel[x]);
            int y1 = (int)LOBYTE(pSrcPel[x + 1]);
            int v0 = (int)HIBYTE(pSrcPel[x + 1]);

            pDestPel[x] = ConvertYCrCbToRGB(y0, v0, u0);
            pDestPel[x + 1] = ConvertYCrCbToRGB(y1, v0, u0);
        }

        pSrcPel += lWidth;
        pDestPel += lWidth;
    }
}
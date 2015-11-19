#pragma once

#include <Windows.h>
#include <time.h>
#include <vector>
using namespace std;
#include <DShow.h>
#include <Ks.h>
#include <KsMedia.h>
#include <mfidl.h>

struct __declspec(uuid("BE660B75-1315-4A97-8297-1E678E28953D")) IBufferLock;
struct __declspec(uuid("5B57A953-D677-4073-B64B-78BD270E56CB")) ICameraMf;

typedef HRESULT(CALLBACK *FrameCallback)(HRESULT hrStatus,
	DWORD dwStreamIndex,
	DWORD dwStreamFlags,
	LONGLONG llTimestamp,
	IMFSample *pSample
);

struct ICameraMf : public IUnknown {
	virtual HRESULT Initialize(LONG lWidth, LONG lHeight, FrameCallback pfnFrameCallback) = 0;
	virtual HRESULT StartRenderAsync(wchar_t *pszFriendlyName) = 0;
	virtual HRESULT StopRenderAsync() = 0;
	virtual HRESULT IsSystemCamera(wchar_t *pszFriendlyName, PBOOL pbSystemCamera) = 0;
	virtual HRESULT MfDumpCameraInfo(wchar_t *pszFriendlyName) = 0;
};

#ifdef __cplusplus
extern "C" {
#endif
	//
	// Factory functions.
	//
	_declspec(dllexport) HRESULT CreateBufferLockInstance(IMFMediaBuffer *pBuffer, IBufferLock **ppObj);
	_declspec(dllexport) HRESULT CreateCameraMfInstance(ICameraMf **ppObj);
#ifdef __cplusplus
}
#endif
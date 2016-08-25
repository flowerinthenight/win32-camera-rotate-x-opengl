/*
* Copyright(c) 2016 Chew Esmero
* All rights reserved.
*/

#include "stdafx.h"
#include <Windows.h>
#include <comdef.h>
#include "..\include\libcore.h"
#include "..\include\libcamera.h"
#include <strsafe.h>

class VideoBufferLock : public IBufferLock {
public:
	VideoBufferLock(IMFMediaBuffer *pBuffer) :
		m_p2DBuffer(NULL),
		m_bLocked(FALSE),
		m_lRefCount(0)
	{
		m_pBuffer = pBuffer;
		
		m_pBuffer->AddRef();

		//
        // Query for the 2-D buffer interface. OK if this fails.
		//
        m_pBuffer->QueryInterface(IID_PPV_ARGS(&m_p2DBuffer));
    }

	virtual ~VideoBufferLock()
	{
        UnlockBuffer();
        SafeRelease(&m_pBuffer);
        SafeRelease(&m_p2DBuffer);
    }

	//
	// IUnknown methods.
	//
	STDMETHODIMP QueryInterface(REFIID iid, void** ppv)
	{
		if ((iid == __uuidof(IUnknown)) || (iid == __uuidof(IBufferLock)))
		{
			*ppv = static_cast<VideoBufferLock*>(this);
		}
		else
		{
			*ppv = NULL;
			return E_NOINTERFACE;
		}
	
		AddRef();
		return S_OK;
	}

	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_lRefCount);
	}

	STDMETHODIMP_(ULONG) Release()
	{
		ULONG uCount = InterlockedDecrement(&m_lRefCount);
 
		if (uCount == 0)
		{
			delete this;
		}

		return uCount;
	}

	//
	// LockBuffer
	// 
	// Locks the buffer. Returns a pointer to scan line 0 and returns the stride.
	// 
	// The caller must provide the default stride as an input parameter, in case the buffer does not expose IMF2DBuffer.
	// You can calculate the default stride from the media type.
	// 
    HRESULT LockBuffer(
		LONG lDefaultStride,			// minimum stride (with no padding).
		DWORD dwHeightInPixels,			// height of the image, in pixels.
		BYTE **ppbScanLine0,			// receives a pointer to the start of scan line 0.
		LONG  *plStride)				// receives the actual stride.
	{
        HRESULT hr = S_OK;
		
		//
        // Use the 2-D version if available.
		//
        if (m_p2DBuffer)
		{
            hr = m_p2DBuffer->Lock2D(ppbScanLine0, plStride);
		}
		else
		{
			//
            // Use non-2D version.
			//
            BYTE *pData = NULL;

            hr = m_pBuffer->Lock(&pData, NULL, NULL);
            
			if (SUCCEEDED(hr))
			{
                *plStride = lDefaultStride;
                
				if (lDefaultStride < 0)
				{
					//
                    // Bottom-up orientation. Return a pointer to the start of the last row *in memory* which is the top row of the image.
					//
                    *ppbScanLine0 = pData + abs(lDefaultStride) * (dwHeightInPixels - 1);
				}
				else
				{
					//
                    // Top-down orientation. Return a pointer to the start of the buffer.
					//
                    *ppbScanLine0 = pData;
                }
            }
        }

        m_bLocked = (SUCCEEDED(hr));

        return hr;
    }

    //
	// Unlocks the buffer. Called automatically by the destructor.
	//
    void UnlockBuffer()
	{
        if (m_bLocked)
		{
			if (m_p2DBuffer)
			{
				m_p2DBuffer->Unlock2D();
			}
			else
			{
				m_pBuffer->Unlock();
			}

			m_bLocked = FALSE;
        }
    }

private:
    IMFMediaBuffer *m_pBuffer;
    IMF2DBuffer *m_p2DBuffer;
    BOOL m_bLocked;
	LONG m_lRefCount;
};

//
// Our exported VideoBufferLock factory function.
//
_declspec(dllexport) HRESULT CreateBufferLockInstance(IMFMediaBuffer *pBuffer, IBufferLock **ppObj)
{
	if (ppObj == NULL) return E_POINTER;
    
	IBufferLock *pObj = new (std::nothrow) VideoBufferLock(pBuffer);

	if (pObj == NULL) return E_OUTOFMEMORY;

	*ppObj = pObj;

	(*ppObj)->AddRef();

	return S_OK;
}

#include "stdafx.h"
#include <initguid.h>    // declares DEFINE_GUID to declare an EXTERN_C const.
#if (_MSC_VER < 1100)
#include <olectlid.h>
#else
#include <olectl.h>
#endif

#include "T264Enc.h"
#include "malloc.h"
#include "T264EncProp.h"
#include "dvdmedia.h"

// Conversion from RGB to YUV420
int RGB2YUV_YR[256], RGB2YUV_YG[256], RGB2YUV_YB[256];
int RGB2YUV_UR[256], RGB2YUV_UG[256], RGB2YUV_UBVR[256];
int RGB2YUV_VG[256], RGB2YUV_VB[256];
void InitLookupTable();
int ConvertRGB2YUV(int w,int h,unsigned char *bmp,unsigned int *yuv);
unsigned char* pTmp = new unsigned char[120*160*3];


static const WCHAR g_wszEncName[] = L"T264 Encoder";
static const WCHAR g_wszEncPropName[] = L"T264 Properties";
static const WCHAR g_wszDecName[] = L"T264 Decoder";
static const WCHAR g_wszDecPropName[] = L"T264 decoder properties";
static const WCHAR g_wszSplitterName[] = L"T264 splitter";

// dshow needs this
STDAPI
DllRegisterServer()
{
    return AMovieDllRegisterServer2( TRUE );
}

STDAPI
DllUnregisterServer()
{
    return AMovieDllRegisterServer2( FALSE );
}

//
// DllEntryPoint
//
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL APIENTRY DllMain(HANDLE hModule, 
                      DWORD  dwReason, 
                      LPVOID lpReserved)
{
    return DllEntryPoint((HINSTANCE)(hModule), dwReason, lpReserved);
}

// {6D2616AA-3C89-4703-9CEB-693C051C9E5F}
DEFINE_GUID(CLSID_T264ENC, 
            0x6d2616aa, 0x3c89, 0x4703, 0x9c, 0xeb, 0x69, 0x3c, 0x5, 0x1c, 0x9e, 0x5f);
// {EAB4831B-121B-4568-86A7-E8058BABA62E}
DEFINE_GUID(CLSID_T264DEC, 
            0xeab4831b, 0x121b, 0x4568, 0x86, 0xa7, 0xe8, 0x5, 0x8b, 0xab, 0xa6, 0x2e);
// {0CA6ED63-793D-4a4e-BF64-6E37F6F4D44E}
DEFINE_GUID(CLSID_T264SUBTYPE, 
            0xca6ed63, 0x793d, 0x4a4e, 0xbf, 0x64, 0x6e, 0x37, 0xf6, 0xf4, 0xd4, 0x4e);
// {E795D14A-879D-4ea9-95EE-B6884276AEEF}
DEFINE_GUID(CLSID_T264PropPage, 
            0xe795d14a, 0x879d, 0x4ea9, 0x95, 0xee, 0xb6, 0x88, 0x42, 0x76, 0xae, 0xef);
// {F3957FA3-98EE-4568-942D-953BBF79136F}
DEFINE_GUID(CLSID_T264Splitter, 
            0xf3957fa3, 0x98ee, 0x4568, 0x94, 0x2d, 0x95, 0x3b, 0xbf, 0x79, 0x13, 0x6f);

// setup data - allows the self-registration to work.
const AMOVIESETUP_MEDIATYPE sudInPinTypes[] =
{
    { 
        &MEDIATYPE_Video,
        &MEDIASUBTYPE_NULL 
    }
};

const AMOVIESETUP_MEDIATYPE sudOutPinTypes[] =
{
    { 
        &MEDIATYPE_Stream,
        &MEDIASUBTYPE_NULL 
    }
};

const AMOVIESETUP_MEDIATYPE sudInPinTypes1[] = 
{
    {
        &MEDIATYPE_Stream,
        &MEDIASUBTYPE_NULL
    }
};

// encoder
const AMOVIESETUP_PIN psudPins[] =
{ 
    { 
        L"Input",            // strName
        FALSE,               // bRendered
        FALSE,               // bOutput
        FALSE,               // bZero
        FALSE,               // bMany
        &CLSID_NULL,         // clsConnectsToFilter
        L"",                 // strConnectsToPin
        1,                   // nTypes
        sudInPinTypes,       // lpTypes
    },
    { 
        L"Output",           // strName
        FALSE,               // bRendered
        TRUE,                // bOutput
        FALSE,               // bZero
        FALSE,               // bMany
        &CLSID_NULL,         // clsConnectsToFilter
        L"",                 // strConnectsToPin
        1,                   // nTypes
        sudOutPinTypes,      // lpTypes
    }
};

// decoder
const AMOVIESETUP_PIN psudPinsdec[] =
{ 
    { 
        L"Input",            // strName
        FALSE,               // bRendered
        FALSE,               // bOutput
        FALSE,               // bZero
        FALSE,               // bMany
        &CLSID_NULL,         // clsConnectsToFilter
        L"",                 // strConnectsToPin
        1,                   // nTypes
        sudInPinTypes,       // lpTypes
    },
    { 
        L"Output",           // strName
        FALSE,               // bRendered
        TRUE,                // bOutput
        FALSE,               // bZero
        FALSE,               // bMany
        &CLSID_NULL,         // clsConnectsToFilter
        L"",                 // strConnectsToPin
        1,                   // nTypes
        sudInPinTypes,      // lpTypes
    }
};

// splitter
const AMOVIESETUP_PIN psudPinssplitter[] =
{
    { 
        L"Input",            // strName
        FALSE,               // bRendered
        FALSE,               // bOutput
        FALSE,               // bZero
        FALSE,               // bMany
        &CLSID_NULL,         // clsConnectsToFilter
        L"",                 // strConnectsToPin
        1,                   // nTypes
        sudInPinTypes1,       // lpTypes
    },
    { 
        L"Output",           // strName
        FALSE,               // bRendered
        TRUE,                // bOutput
        FALSE,               // bZero
        FALSE,               // bMany
        &CLSID_NULL,         // clsConnectsToFilter
        L"",                 // strConnectsToPin
        1,                   // nTypes
        sudInPinTypes,      // lpTypes
    }
};

const AMOVIESETUP_FILTER sudDST264ENC =
{ 
    &CLSID_T264ENC,                  // clsID
    g_wszEncName,                    // strName
    MERIT_DO_NOT_USE,                // dwMerit
    2,                               // nPins
    psudPins, 
};

const AMOVIESETUP_FILTER sudDST264DEC =
{ 
    &CLSID_T264DEC,                  // clsID
    g_wszDecName,                    // strName
    MERIT_NORMAL,                    // dwMerit
    2,                               // nPins
    psudPinsdec, 
};

const AMOVIESETUP_FILTER subDST264Splitter = 
{
    &CLSID_T264Splitter,             // clsID
    g_wszSplitterName,               // strName
    MERIT_NORMAL + 3,                // dwMerit
    2,                               // nPins
    psudPinssplitter, 
};

CFactoryTemplate g_Templates[]=
{   
    { 
        g_wszEncName,          
        &CLSID_T264ENC,
        CT264Enc::CreateInstance,  // function called by class factory
        NULL,
        &sudDST264ENC 
    },
    { 
        g_wszEncPropName,
        &CLSID_T264PropPage,
        CT264EncProp::CreateInstance 
    },
    { 
        g_wszDecName,          
        &CLSID_T264DEC,
        CT264Dec::CreateInstance,  // function called by class factory
        NULL,
        &sudDST264DEC 
    },
    {
        g_wszSplitterName,
        &CLSID_T264Splitter,
        CT264Splitter::CreateInstance,
        NULL,
        &subDST264Splitter
    }
};
int g_cTemplates = sizeof(g_Templates)/sizeof(g_Templates[0]);  

//////////////////////////////////////////////////////////////////////////
// CT264Enc
CT264Enc::CT264Enc(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr) :
    CTransformFilter(tszName, punk, CLSID_T264ENC)
{
    m_t264 = 0;
    memset(&m_param, 0, sizeof(m_param));
    put_Default();
    m_hWnd = 0;

	InitLookupTable();
}

CT264Enc::~CT264Enc()
{
    if (m_t264 != 0)
    {
        T264_close(m_t264);
        _aligned_free(m_pBuffer);
    }
}

//
// CreateInstance
//
// Provide the way for COM to create a CT264Enc object
//
CUnknown * WINAPI CT264Enc::CreateInstance(LPUNKNOWN punk, HRESULT *phr) 
{
    ASSERT(phr);

    CT264Enc *pNewObject = new CT264Enc(NAME("T264Enc"), punk, phr);
    if (pNewObject == NULL) {
        if (phr)
            *phr = E_OUTOFMEMORY;
    }

    return pNewObject;

} // CreateInstance

//
// NonDelegatingQueryInterface
//
// Reveals IContrast and ISpecifyPropertyPages
//
STDMETHODIMP CT264Enc::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv, E_POINTER);

    if (riid == IID_IProp)
    {
        return GetInterface((IProp*) this, ppv);
    } 
    else if (riid == IID_ISpecifyPropertyPages) 
    {
        return GetInterface((ISpecifyPropertyPages*) this, ppv);
    } 
    else 
    {
        return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface

HRESULT CT264Enc::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
    INT plane = m_param.width * m_param.height;
    HRESULT hr;
    // some decoder does not reset emms(such as mainconcept mpeg2 decoder), and we need compute floating ...
    __asm emms
    BYTE* pSrc, *pDst;
    LONG lDstSize, lActualSize;
    hr = pIn->GetPointer(&pSrc);
    ASSERT(hr == S_OK);
    hr = pOut->GetPointer(&pDst);
    ASSERT(hr == S_OK);
    lDstSize = pOut->GetSize();

	//ConvertRGB2YUV(160,120,pSrc,(unsigned int*)pTmp);

    // convert yv12 ==> iyuv
    CopyMemory(m_pBuffer, pSrc, plane);
    CopyMemory(m_pBuffer + plane, pSrc + plane + (plane >> 2), plane >> 2);
    CopyMemory(m_pBuffer + plane + (plane >> 2), pSrc + plane, plane >> 2);
    lActualSize = T264_encode(m_t264, m_pBuffer, pDst, lDstSize);
    ASSERT(lActualSize <= lDstSize);
    pOut->SetActualDataLength(lActualSize);

    if (m_hWnd && lActualSize)
    {
        wsprintf(m_szInfo, "Frame = %d, Qp = %d, Length = %d.", m_t264->frame_id, m_t264->qp_y, lActualSize);
        SendMessage(m_hWnd, WM_SETTEXT, 0, (LPARAM)m_szInfo);
    }

    return hr;
} // Transform

HRESULT CT264Enc::Copy(IMediaSample *pSource, IMediaSample *pDest) const
{
    CheckPointer(pSource,E_POINTER);
    CheckPointer(pDest,E_POINTER);

    // Copy the sample data
    BYTE *pSourceBuffer, *pDestBuffer;
    long lSourceSize = pSource->GetActualDataLength();

#ifdef DEBUG
    long lDestSize = pDest->GetSize();
    ASSERT(lDestSize >= lSourceSize);
#endif

    pSource->GetPointer(&pSourceBuffer);
    pDest->GetPointer(&pDestBuffer);

    CopyMemory((PVOID) pDestBuffer,(PVOID) pSourceBuffer,lSourceSize);

    // Copy the sample times

    REFERENCE_TIME TimeStart, TimeEnd;
    if(NOERROR == pSource->GetTime(&TimeStart, &TimeEnd))
    {
        pDest->SetTime(&TimeStart, &TimeEnd);
    }

    LONGLONG MediaStart, MediaEnd;
    if(pSource->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR)
    {
        pDest->SetMediaTime(&MediaStart,&MediaEnd);
    }

    // Copy the Sync point property

    HRESULT hr = pSource->IsSyncPoint();
    if(hr == S_OK)
    {
        pDest->SetSyncPoint(TRUE);
    }
    else if(hr == S_FALSE)
    {
        pDest->SetSyncPoint(FALSE);
    }
    else
    {  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the media type

    AM_MEDIA_TYPE *pMediaType;
    pSource->GetMediaType(&pMediaType);
    pDest->SetMediaType(pMediaType);
    DeleteMediaType(pMediaType);

    // Copy the preroll property

    hr = pSource->IsPreroll();
    if(hr == S_OK)
    {
        pDest->SetPreroll(TRUE);
    }
    else if(hr == S_FALSE)
    {
        pDest->SetPreroll(FALSE);
    }
    else
    {  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the discontinuity property

    hr = pSource->IsDiscontinuity();

    if(hr == S_OK)
    {
        pDest->SetDiscontinuity(TRUE);
    }
    else if(hr == S_FALSE)
    {
        pDest->SetDiscontinuity(FALSE);
    }
    else
    {  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the actual data length

    long lDataLength = pSource->GetActualDataLength();
    pDest->SetActualDataLength(lDataLength);

    return NOERROR;

} // Copy



void InitLookupTable()
{
	int i;

	for (i = 0; i < 256; i++) RGB2YUV_YR[i] = (float)65.481 * (i<<8);
	for (i = 0; i < 256; i++) RGB2YUV_YG[i] = (float)128.553 * (i<<8);
	for (i = 0; i < 256; i++) RGB2YUV_YB[i] = (float)24.966 * (i<<8);
	for (i = 0; i < 256; i++) RGB2YUV_UR[i] = (float)37.797 * (i<<8);
	for (i = 0; i < 256; i++) RGB2YUV_UG[i] = (float)74.203 * (i<<8);
	for (i = 0; i < 256; i++) RGB2YUV_VG[i] = (float)93.786 * (i<<8);
	for (i = 0; i < 256; i++) RGB2YUV_VB[i] = (float)18.214 * (i<<8);
	for (i = 0; i < 256; i++) RGB2YUV_UBVR[i] = (float)112 * (i<<8);
}

int ConvertRGB2YUV(int w,int h,unsigned char *bmp,unsigned int *yuv)
{

unsigned int *u,*v,*y,*uu,*vv;
unsigned int *pu1,*pu2,*pu3,*pu4;
unsigned int *pv1,*pv2,*pv3,*pv4;
unsigned char *r,*g,*b;
int i,j;

uu=new unsigned int[w*h];
vv=new unsigned int[w*h];

if(uu==NULL || vv==NULL)
return 0;

y=yuv;
u=uu;
v=vv;

// Get r,g,b pointers from bmp image data....
r=bmp;
g=bmp+1;
b=bmp+2;


//Get YUV values for rgb values...

	for(i=0;i<h;i++)
	{
	
		for(j=0;j<w;j++)
		{
		*y++=( RGB2YUV_YR[*r]  +RGB2YUV_YG[*g]+RGB2YUV_YB[*b]+1048576)>>16;
		*u++=(-RGB2YUV_UR[*r]  -RGB2YUV_UG[*g]+RGB2YUV_UBVR[*b]+8388608)>>16;
		*v++=( RGB2YUV_UBVR[*r]-RGB2YUV_VG[*g]-RGB2YUV_VB[*b]+8388608)>>16;

		r+=3;
		g+=3;
		b+=3;
		}

	}



	// Now sample the U & V to obtain YUV 4:2:0 format

	// Sampling mechanism...
/*	  @  ->  Y
	  #  ->  U or V
	  
	  @   @   @   @
		#       #
	  @   @   @   @
	
	  @   @   @   @
		#       #
	  @   @   @   @

*/

	// Get the right pointers...
	u=yuv+w*h;
	v=u+(w*h)/4;

	// For U
	pu1=uu;
	pu2=pu1+1;
	pu3=pu1+w;
	pu4=pu3+1;

	// For V
	pv1=vv;
	pv2=pv1+1;
	pv3=pv1+w;
	pv4=pv3+1;

	// Do sampling....
	for(i=0;i<h;i+=2)
	{
	
		for(j=0;j<w;j+=2)
		{
		*u++=(*pu1+*pu2+*pu3+*pu4)>>2;
		*v++=(*pv1+*pv2+*pv3+*pv4)>>2;

		pu1+=2;
		pu2+=2;
		pu3+=2;
		pu4+=2;

		pv1+=2;
		pv2+=2;
		pv3+=2;
		pv4+=2;
		}
	
	pu1+=w;
	pu2+=w;
	pu3+=w;
	pu4+=w;

	pv1+=w;
	pv2+=w;
	pv3+=w;
	pv4+=w;
		
	}


	delete uu;
	delete vv;

return 1;
}



HRESULT CT264Enc::CheckInputType(const CMediaType *mtIn)
{
    CheckPointer(mtIn,E_POINTER);

    if(*mtIn->FormatType() == FORMAT_VideoInfo)
    {                                    
      if(IsEqualGUID(*mtIn->Subtype(), MEDIASUBTYPE_YV12))
        {
            if(mtIn->FormatLength() < sizeof(VIDEOINFOHEADER))
                return E_INVALIDARG;

            VIDEOINFO *pInput  = (VIDEOINFO *) mtIn->Format();
            m_param.width = pInput->bmiHeader.biWidth;
            m_param.height = pInput->bmiHeader.biHeight;
            m_param.framerate = (float)(INT)((float)10000000 / pInput->AvgTimePerFrame + 0.5);
            m_avgFrameTime = pInput->AvgTimePerFrame;
            return NOERROR;
        }
    }
    else if (*mtIn->FormatType() == FORMAT_VideoInfo2)
    {
        if(IsEqualGUID(*mtIn->Subtype(), MEDIASUBTYPE_YV12))
        {
            if(mtIn->FormatLength() < sizeof(VIDEOINFOHEADER2))
                return E_INVALIDARG;

            VIDEOINFOHEADER2 *pInput  = (VIDEOINFOHEADER2*) mtIn->Format();
            m_param.width = pInput->bmiHeader.biWidth;
            m_param.height = pInput->bmiHeader.biHeight;
            m_avgFrameTime = (LONGLONG)((float)10000000 / m_param.framerate);//pInput->AvgTimePerFrame;
            m_param.framerate = (float)(INT)(m_param.framerate + 0.5f);
            return NOERROR;
        }
    }

    return E_INVALIDARG;

} // CheckInputType


HRESULT CT264Enc::CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut)
{
    CheckPointer(mtIn,E_POINTER);
    CheckPointer(mtOut,E_POINTER);

    HRESULT hr;
    if(FAILED(hr = CheckInputType(mtIn)))
    {
        return hr;
    }

    if(*mtOut->FormatType() != FORMAT_VideoInfo)
    {
        return E_INVALIDARG;
    }

    // formats must be big enough 
    if(mtIn->FormatLength() < sizeof(VIDEOINFOHEADER) ||
        mtOut->FormatLength() < sizeof(VIDEOINFOHEADER))
        return E_INVALIDARG;

    return NOERROR;

} // CheckTransform

HRESULT CT264Enc::InitOutMediaType(CMediaType* pmt)
{
    pmt->InitMediaType();

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetSubtype(&CLSID_T264SUBTYPE);

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));
    DWORD fcc = '462T';
    pvi->dwBitRate = m_param.bitrate;
    pvi->AvgTimePerFrame = m_avgFrameTime;
    pvi->bmiHeader.biCompression = fcc;
    pvi->bmiHeader.biBitCount    = 12;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = m_param.width;
    pvi->bmiHeader.biHeight     = m_param.height;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    SetRectEmpty(&(pvi->rcSource));
    SetRectEmpty(&(pvi->rcTarget));
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetVariableSize();
    pmt->SetTemporalCompression(true);

    return S_OK;
}

//
// DecideBufferSize
//
// Tell the output pin's allocator what size buffers we
// require. Can only do this when the input is connected
//
HRESULT CT264Enc::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    CheckPointer(pAlloc,E_POINTER);
    CheckPointer(pProperties,E_POINTER);

    // Is the input pin connected

    if(m_pInput->IsConnected() == FALSE)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = NOERROR;
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = m_pInput->CurrentMediaType().GetSampleSize();

    ASSERT(pProperties->cbBuffer);

    // If we don't have fixed sized samples we must guess some size

    if(!m_pInput->CurrentMediaType().bFixedSizeSamples)
    {
        if(pProperties->cbBuffer < OUTPIN_BUFFER_SIZE)
        {
            // nothing more than a guess!!
            pProperties->cbBuffer = OUTPIN_BUFFER_SIZE;
        }
    }

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;

    hr = pAlloc->SetProperties(pProperties,&Actual);
    if(FAILED(hr))
    {
        return hr;
    }

    ASSERT(Actual.cBuffers == 1);

    if(pProperties->cBuffers > Actual.cBuffers ||
        pProperties->cbBuffer > Actual.cbBuffer)
    {
        return E_FAIL;
    }

    return NOERROR;

} // DecideBufferSize


//
// GetMediaType
//
// I support one type, namely the type of the input pin
// We must be connected to support the single output type
//
HRESULT CT264Enc::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    // Is the input pin connected

    if(m_pInput->IsConnected() == FALSE)
    {
        return E_UNEXPECTED;
    }

    // This should never happen

    if(iPosition < 0)
    {
        return E_INVALIDARG;
    }

    // Do we have more items to offer

    if(iPosition > 0)
    {
        return VFW_S_NO_MORE_ITEMS;
    }

    CheckPointer(pMediaType,E_POINTER);

    InitOutMediaType(pMediaType);

    return NOERROR;

} // GetMediaType

HRESULT CT264Enc::StartStreaming()
{
    _asm emms
    if (m_t264 == NULL)
    {
        INT plane = m_param.width * m_param.height;
        m_t264 = T264_open(&m_param);
        m_pBuffer = (BYTE*)_aligned_malloc(plane + (plane >> 1), 16);
        ASSERT(m_t264);
    }
    return CTransformFilter::StartStreaming();
}

HRESULT CT264Enc::StopStreaming()
{
    if (m_t264 != NULL)
    {
        T264_close(m_t264);
        m_t264 = 0;
        _aligned_free(m_pBuffer);
        m_pBuffer = 0;
    }
    return CTransformFilter::StopStreaming();
}

//
// GetPages
//
// This is the sole member of ISpecifyPropertyPages
// Returns the clsid's of the property pages we support
//
STDMETHODIMP CT264Enc::GetPages(CAUUID *pPages)
{
    CheckPointer(pPages,E_POINTER);

    pPages->cElems = 1;
    pPages->pElems = (GUID *) CoTaskMemAlloc(sizeof(GUID));
    if(pPages->pElems == NULL)
    {
        return E_OUTOFMEMORY;
    }

    *(pPages->pElems) = CLSID_T264PropPage;
    return NOERROR;
} // GetPages

HRESULT CT264Enc::get_Para(INT** pPara)
{
    *pPara = (INT*)&m_param;
    return S_OK;
}

HRESULT CT264Enc::put_Default()
{
    m_param.flags  = //USE_INTRA16x16|
        USE_INTRA4x4| 
        USE_HALFPEL|
        USE_QUARTPEL|
        USE_SUBBLOCK|
        //    USE_FULLSEARCH|
        USE_DIAMONDSEACH|
        USE_FORCEBLOCKSIZE|
        USE_FASTINTERPOLATE|
        USE_SAD|
        USE_EXTRASUBPELSEARCH|
        USE_INTRAININTER|
        USE_SCENEDETECT;

    m_param.iframe = 30;
    m_param.idrframe = 3000 * 300;
    m_param.qp       = 28;
    m_param.min_qp   = 8;
    m_param.max_qp   = 34;
    m_param.bitrate  = 600 * 1024;
    m_param.framerate= 30;

    m_param.search_x = 16;
    m_param.search_y = 16;

    m_param.block_size = SEARCH_16x16P
        |SEARCH_16x8P
        |SEARCH_8x16P
//        |SEARCH_8x8P
//        |SEARCH_8x4P
//        |SEARCH_4x8P
//        |SEARCH_4x4P;
        |SEARCH_16x16B
        |SEARCH_16x8B
        |SEARCH_8x16B;

    m_param.disable_filter = 0;
    m_param.aspect_ratio = 2;
    m_param.video_format = 1;

    m_param.ref_num  = 3;
    m_param.luma_coeff_cost = 4;    // default 4, min qp please decrease this value
    m_param.cpu      = 0;
    m_param.cabac = 1;
    m_param.b_num = 1;
    if (m_param.bitrate != 0)
        m_param.enable_rc = 1;
    else
        m_param.enable_rc = 0;

    return S_OK;
}

HRESULT CT264Enc::put_InfoWnd(INT hWnd)
{
    m_hWnd = (HWND)hWnd;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// CT264Dec
CT264Dec::CT264Dec(TCHAR *tszName,LPUNKNOWN punk,HRESULT *phr) :
CTransformFilter(tszName, punk, CLSID_T264ENC)
{
    m_t264 = 0;
    m_hWnd = 0;
}

CT264Dec::~CT264Dec()
{
    if (m_t264 != 0)
    {
        T264dec_close(m_t264);
    }
}

//
// CreateInstance
//
// Provide the way for COM to create a CT264Dec object
//
CUnknown * WINAPI CT264Dec::CreateInstance(LPUNKNOWN punk, HRESULT *phr) 
{
    ASSERT(phr);

    CT264Dec *pNewObject = new CT264Dec(NAME("T264Dec"), punk, phr);
    if (pNewObject == NULL) {
        if (phr)
            *phr = E_OUTOFMEMORY;
    }

    return pNewObject;

} // CreateInstance

//
// NonDelegatingQueryInterface
//
// Reveals IContrast and ISpecifyPropertyPages
//
STDMETHODIMP CT264Dec::NonDelegatingQueryInterface(REFIID riid, void **ppv)
{
    CheckPointer(ppv, E_POINTER);

    //if (riid == IID_IProp)
    //{
    //    return GetInterface((IProp*) this, ppv);
    //} 
    //else if (riid == IID_ISpecifyPropertyPages) 
    //{
    //    return GetInterface((ISpecifyPropertyPages*) this, ppv);
    //} 
    //else 
    {
        return CTransformFilter::NonDelegatingQueryInterface(riid, ppv);
    }

} // NonDelegatingQueryInterface

HRESULT CT264Dec::Transform(IMediaSample *pIn, IMediaSample *pOut)
{
    return S_OK;
} // Transform

HRESULT CT264Dec::SendSample(T264_t* t, T264_frame_t *frame, IMediaSample* pSample)
{
    INT	i;
    BYTE* p, *pDst;
    HRESULT hr;
    IMediaSample* pOutSample;

    // Set up the output sample
    hr = InitializeOutputSample(pSample, &pOutSample);

    if (FAILED(hr)) 
    {
        return hr;
    }
    hr = pOutSample->GetPointer(&pDst);
    ASSERT(hr == S_OK);

    p = frame->Y[0];
    for(i = 0 ; i < t->height ; i ++)
    {
        memcpy(pDst, p, t->width);
        pDst += m_nStride;
        p += t->edged_stride;
    }
    p = frame->V;
    for(i = 0 ; i < t->height >> 1 ; i ++)
    {
        memcpy(pDst, p, t->width);
        pDst += m_nStride >> 1;
        p += t->edged_stride_uv;
    }
    p = frame->U;
    for(i = 0 ; i < t->height >> 1 ; i ++)
    {
        memcpy(pDst, p, t->width);
        pDst += m_nStride >> 1;
        p += t->edged_stride_uv;
    }

    pOutSample->SetActualDataLength(t->width * t->height + (t->width * t->height >> 1));
    pOutSample->SetSyncPoint(TRUE);
    CRefTime rtEnd = m_time + m_avgFrameTime;
    pOutSample->SetTime(&m_time.m_time, &rtEnd.m_time);
    m_time = rtEnd;

    hr = m_pNextFilterInputpin->Receive(pOutSample);
    m_bSampleSkipped = FALSE;	// last thing no longer dropped

    // release the output buffer. If the connected pin still needs it,
    // it will have addrefed it itself.
    pOutSample->Release();

    return hr;
}

HRESULT CT264Dec::CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin)
{
    HRESULT hr = CTransformFilter::CompleteConnect(direction, pReceivePin);
    if (direction == PINDIR_OUTPUT)
    {
        hr = pReceivePin->QueryInterface(__uuidof(m_pNextFilterInputpin), (VOID**)&m_pNextFilterInputpin);
        ASSERT(hr == S_OK);
        // we do not want to hold the reference of the input pin
        m_pNextFilterInputpin->Release();
    }
    return hr;
}

HRESULT CT264Dec::Receive(IMediaSample* pSample)
{
    /*  Check for other streams and pass them on */
    AM_SAMPLE2_PROPERTIES * const pProps = m_pInput->SampleProps();
    if (pProps->dwStreamId != AM_STREAM_MEDIA) {
        return m_pNextFilterInputpin->Receive(pSample);
    }
    HRESULT hr;
    ASSERT(pSample);

    ASSERT (m_pOutput != NULL) ;
    {
        // some decoder does not reset emms(such as mainconcept mpeg2 decoder), and we need compute floating ...
        BYTE* pSrc;
        LONG lSrcSize;
        INT run = 1;
        hr = pSample->GetPointer(&pSrc);
        ASSERT(hr == S_OK);
        lSrcSize = pSample->GetSize();

        T264dec_buffer(m_t264, pSrc, lSrcSize);
        while (run)
        {
            decoder_state_t state = T264dec_parse(m_t264);
            switch(state) 
            {
            case DEC_STATE_SLICE:
                {
                    if (m_t264->output.poc >= 0)
                    {
                        SendSample(m_t264, &m_t264->output, pSample);
                    }
                }
                break;
            case DEC_STATE_BUFFER:
                /* read more data */
                return S_OK;
            case DEC_STATE_SEQ:
                if (m_t264->frame_id > 0)
                {
                    SendSample(m_t264, T264dec_flush_frame(m_t264), pSample);
                }
                break;
            /*case DEC_STATE_PIC:*/
            default:
                /* do not care */
                break;
            }
        }
    }

    return hr;
}

HRESULT CT264Dec::Copy(IMediaSample *pSource, IMediaSample *pDest) const
{
    CheckPointer(pSource,E_POINTER);
    CheckPointer(pDest,E_POINTER);

    // Copy the sample data
    BYTE *pSourceBuffer, *pDestBuffer;
    long lSourceSize = pSource->GetActualDataLength();

#ifdef DEBUG
    long lDestSize = pDest->GetSize();
    ASSERT(lDestSize >= lSourceSize);
#endif

    pSource->GetPointer(&pSourceBuffer);
    pDest->GetPointer(&pDestBuffer);

    CopyMemory((PVOID) pDestBuffer,(PVOID) pSourceBuffer,lSourceSize);

    // Copy the sample times

    REFERENCE_TIME TimeStart, TimeEnd;
    if(NOERROR == pSource->GetTime(&TimeStart, &TimeEnd))
    {
        pDest->SetTime(&TimeStart, &TimeEnd);
    }

    LONGLONG MediaStart, MediaEnd;
    if(pSource->GetMediaTime(&MediaStart,&MediaEnd) == NOERROR)
    {
        pDest->SetMediaTime(&MediaStart,&MediaEnd);
    }

    // Copy the Sync point property

    HRESULT hr = pSource->IsSyncPoint();
    if(hr == S_OK)
    {
        pDest->SetSyncPoint(TRUE);
    }
    else if(hr == S_FALSE)
    {
        pDest->SetSyncPoint(FALSE);
    }
    else
    {  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the media type

    AM_MEDIA_TYPE *pMediaType;
    pSource->GetMediaType(&pMediaType);
    pDest->SetMediaType(pMediaType);
    DeleteMediaType(pMediaType);

    // Copy the preroll property

    hr = pSource->IsPreroll();
    if(hr == S_OK)
    {
        pDest->SetPreroll(TRUE);
    }
    else if(hr == S_FALSE)
    {
        pDest->SetPreroll(FALSE);
    }
    else
    {  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the discontinuity property

    hr = pSource->IsDiscontinuity();

    if(hr == S_OK)
    {
        pDest->SetDiscontinuity(TRUE);
    }
    else if(hr == S_FALSE)
    {
        pDest->SetDiscontinuity(FALSE);
    }
    else
    {  // an unexpected error has occured...
        return E_UNEXPECTED;
    }

    // Copy the actual data length

    long lDataLength = pSource->GetActualDataLength();
    pDest->SetActualDataLength(lDataLength);

    return NOERROR;

} // Copy

HRESULT CT264Dec::CheckInputType(const CMediaType *mtIn)
{
    CheckPointer(mtIn,E_POINTER);

    if(*mtIn->FormatType() == FORMAT_VideoInfo)
    {
//        if (*mtIn->Subtype() == CLSID_T264SUBTYPE)
        {
            if(mtIn->FormatLength() < sizeof(VIDEOINFOHEADER))
                return E_INVALIDARG;

            VIDEOINFO *pInput  = (VIDEOINFO *) mtIn->Format();
            if (pInput->bmiHeader.biCompression == 'HSSV' || pInput->bmiHeader.biCompression == '462T')
            {
                m_nWidth = pInput->bmiHeader.biWidth;
                m_nHeight = pInput->bmiHeader.biHeight;
                m_framerate = (float)(INT)((float)10000000 / pInput->AvgTimePerFrame + 0.5);
                m_avgFrameTime = pInput->AvgTimePerFrame;
                return NOERROR;
            }
        }
    }
    else if (*mtIn->FormatType() == FORMAT_VideoInfo2)
    {
//        if (*mtIn->Subtype() == CLSID_T264SUBTYPE)
        {
            if(mtIn->FormatLength() < sizeof(VIDEOINFOHEADER2))
                return E_INVALIDARG;

            VIDEOINFOHEADER2 *pInput  = (VIDEOINFOHEADER2*) mtIn->Format();
            if (pInput->bmiHeader.biCompression == 'HSSV' || pInput->bmiHeader.biCompression == '462T')
            {
                m_nWidth = pInput->bmiHeader.biWidth;
                m_nHeight = pInput->bmiHeader.biHeight;
                m_avgFrameTime = pInput->AvgTimePerFrame;
                m_framerate = (float)(INT)((float)10000000 / pInput->AvgTimePerFrame + 0.5);
                return NOERROR;
            }
        }
    }

    return E_INVALIDARG;

} // CheckInputType

HRESULT CT264Dec::CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut)
{
    CheckPointer(mtIn,E_POINTER);
    CheckPointer(mtOut,E_POINTER);

    HRESULT hr;
    if(FAILED(hr = CheckInputType(mtIn)))
    {
        return hr;
    }

    if(*mtOut->FormatType() != FORMAT_VideoInfo)
    {
        return E_INVALIDARG;
    }

    // formats must be big enough 
    if(mtIn->FormatLength() < sizeof(VIDEOINFOHEADER) ||
        mtOut->FormatLength() < sizeof(VIDEOINFOHEADER))
        return E_INVALIDARG;

    VIDEOINFO* pInfo = (VIDEOINFO*)mtOut->Format();
    m_nStride = pInfo->bmiHeader.biWidth;
    return NOERROR;

} // CheckTransform

HRESULT CT264Dec::InitOutMediaType(CMediaType* pmt)
{
    pmt->InitMediaType();

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetSubtype(&MEDIASUBTYPE_YV12);

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER*)pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER));
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));
    pvi->AvgTimePerFrame = m_avgFrameTime;
    pvi->bmiHeader.biCompression = '21VY';
    pvi->bmiHeader.biBitCount    = 12;
    pvi->bmiHeader.biPlanes = 1;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth      = m_nWidth;
    pvi->bmiHeader.biHeight     = m_nHeight;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    SetRectEmpty(&(pvi->rcSource));
    SetRectEmpty(&(pvi->rcTarget));
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetVariableSize();
    pmt->SetTemporalCompression(true);

    return S_OK;
}

//
// DecideBufferSize
//
// Tell the output pin's allocator what size buffers we
// require. Can only do this when the input is connected
//
HRESULT CT264Dec::DecideBufferSize(IMemAllocator *pAlloc,ALLOCATOR_PROPERTIES *pProperties)
{
    CheckPointer(pAlloc,E_POINTER);
    CheckPointer(pProperties,E_POINTER);

    // Is the input pin connected

    if(m_pInput->IsConnected() == FALSE)
    {
        return E_UNEXPECTED;
    }

    HRESULT hr = NOERROR;
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = (m_nWidth * m_nHeight >> 1) + m_nWidth * m_nHeight;

    ASSERT(pProperties->cbBuffer);

    // If we don't have fixed sized samples we must guess some size

    if(!m_pOutput->CurrentMediaType().bFixedSizeSamples)
    {
        if(pProperties->cbBuffer < OUTPIN_BUFFER_SIZE)
        {
            // nothing more than a guess!!
            pProperties->cbBuffer = OUTPIN_BUFFER_SIZE;
        }
    }

    // Ask the allocator to reserve us some sample memory, NOTE the function
    // can succeed (that is return NOERROR) but still not have allocated the
    // memory that we requested, so we must check we got whatever we wanted

    ALLOCATOR_PROPERTIES Actual;

    hr = pAlloc->SetProperties(pProperties,&Actual);
    if(FAILED(hr))
    {
        return hr;
    }

    ASSERT(Actual.cBuffers == 1);

    if(pProperties->cBuffers > Actual.cBuffers ||
        pProperties->cbBuffer > Actual.cbBuffer)
    {
        return E_FAIL;
    }

    return NOERROR;

} // DecideBufferSize


//
// GetMediaType
//
// I support one type, namely the type of the input pin
// We must be connected to support the single output type
//
HRESULT CT264Dec::GetMediaType(int iPosition, CMediaType *pMediaType)
{
    // Is the input pin connected

    if(m_pInput->IsConnected() == FALSE)
    {
        return E_UNEXPECTED;
    }

    // This should never happen

    if(iPosition < 0)
    {
        return E_INVALIDARG;
    }

    // Do we have more items to offer

    if(iPosition > 0)
    {
        return VFW_S_NO_MORE_ITEMS;
    }

    CheckPointer(pMediaType,E_POINTER);

    InitOutMediaType(pMediaType);

    return NOERROR;

} // GetMediaType

HRESULT CT264Dec::StartStreaming()
{
    _asm emms
    if (m_t264 == NULL)
    {
        m_t264 = T264dec_open();
        ASSERT(m_t264);
    }
    m_time = 0;
    return CTransformFilter::StartStreaming();
}

HRESULT CT264Dec::StopStreaming()
{
    if (m_t264 != NULL)
    {
        T264dec_close(m_t264);
        m_t264 = 0;
    }
    return CTransformFilter::StopStreaming();
}

//////////////////////////////////////////////////////////////////////////
// CT264Splitter
CT264Splitter::CT264Splitter(
    LPUNKNOWN pUnk,
    HRESULT *phr) :
    CBaseSplitterFilter(
                    TEXT("CT264Splitter"),
                    pUnk,
                    CLSID_T264Splitter,
                    phr)
{
    //  Create our input pin
    m_pInput = new CSplitterInputPin(this, phr);
}

CUnknown *CT264Splitter::CreateInstance(LPUNKNOWN pUnk, HRESULT *phr)
{
    CUnknown *pUnkRet = new CT264Splitter(pUnk, phr);
    return pUnkRet;
}

//  Override type checking
HRESULT CT264Splitter::CheckInputType(const CMediaType *pmt)
{
    /*  We'll accept our preferred type or a wild card for the subtype */

    /*if (pmt->majortype != MEDIATYPE_Stream ||
        pmt->subtype != MEDIASUBTYPE_NULL) {
            return S_FALSE;
        } else {
            return S_OK;
        }*/
    // Async. source filter just send majortype equal null
    return S_OK;
}

LPAMOVIESETUP_FILTER CT264Splitter::GetSetupData()
{
    return (LPAMOVIESETUP_FILTER)&subDST264Splitter;
}

/*  Complete connection and instantiate parser
This involves:

Instatiate the parser with for the type and have it check the format
*/

CBaseParser *CT264Splitter::CreateParser(
    CParserNotify *pNotify,
    CMediaType *pType
    )
{
    HRESULT hr = S_OK;
    return new CT264Parser(pNotify, &hr);
}

/*  Cheap'n nasty parser - DON'T do yours like this! */
/*  Initialize a parser

pmt     - type of stream if known - can be NULL
pRdr    - way to read the source medium - can be NULL
*/
HRESULT CT264Parser::Init(CParseReader *pRdr)
{
    const DWORD dwLen = 128;
    /*  Just read 32K and look for interesting stuff */
    PBYTE pbData = new BYTE[dwLen];
    if (pbData == NULL) {
        return E_OUTOFMEMORY;
    }
    HRESULT hr = pRdr->Read(pbData, dwLen);
    if (S_OK != hr) {
        delete [] pbData;
        return hr;
    }

    /*  Now just loop looking for start codes */
    DWORD dwLeft = dwLen;
    int is_t264 = 0;
    PBYTE pbCurrent = pbData;
    {
        DWORD dwCode = *(UNALIGNED DWORD *)pbCurrent;

        /*  Check if it's a valid start code */
        if (dwCode == 0x01000000) 
        {
            int run = 1;
            /*  Create the media type from the stream
            only support Payload streams for now
            */
            VIDEOINFO* pInfo;
            CMediaType cmt;
            cmt.InitMediaType();

            T264_t* t = T264dec_open();
            T264dec_buffer(t, pbData, dwLen);

            while (run)
            {
                decoder_state_t state = T264dec_parse(t);
                switch(state) 
                {
                case DEC_STATE_CUSTOM_SET:
                    is_t264 = true;
                    break;
                case DEC_STATE_SEQ:
                    run = 0;
                    cmt.SetType(&MEDIATYPE_Video);
                    cmt.SetSubtype(&CLSID_T264SUBTYPE);
                    cmt.SetFormatType(&FORMAT_VideoInfo);
                    cmt.AllocFormatBuffer(sizeof(VIDEOINFO));
                    ZeroMemory(cmt.pbFormat, sizeof(VIDEOINFO));
                    pInfo = (VIDEOINFO*)cmt.Format();
                    pInfo->bmiHeader.biWidth = t->width;
                    pInfo->bmiHeader.biHeight = t->height;
                    pInfo->bmiHeader.biBitCount = 12;
                    pInfo->bmiHeader.biCompression = '462T';
                    pInfo->bmiHeader.biPlanes = 1;
                    pInfo->bmiHeader.biSizeImage  = GetBitmapSize(&pInfo->bmiHeader);
                    SetRectEmpty(&(pInfo->rcSource));
                    SetRectEmpty(&(pInfo->rcTarget));
                    cmt.SetVariableSize();
                    cmt.SetTemporalCompression(true);
                    if (t->aspect_ratio == 2)
                        pInfo->AvgTimePerFrame = 400000;
                    else
                        pInfo->AvgTimePerFrame = 333333;
                    break;
                case DEC_STATE_SLICE:
                case DEC_STATE_BUFFER:
                    ASSERT(false);
                    break;
                case DEC_STATE_PIC:
                    break;
                default:
                    /* do not care */
                    break;
                }
            };
            T264dec_close(t);

            /*  Create our video stream */
            m_pNotify->CreateStream(L"Video", &m_Video.m_pNotify);
            m_Video.m_pNotify->AddMediaType(&cmt);
        }
    }
    delete [] pbData;
    if (!is_t264 || !m_Video.Initialized()) 
    {
        return VFW_E_TYPE_NOT_ACCEPTED;
    } 
    else 
    {
        return S_OK;
    }
}


/*  Get the size and count of buffers preferred based on the
actual content
*/
void CT264Parser::GetSizeAndCount(LONG *plSize, LONG *plCount)
{
    *plSize = 32768;
    *plCount = 4;
}

/*  Call this to reinitialize for a new stream */
void CT264Parser::StreamReset()
{
}

/*  Call this to pass new stream data :

pbData        - pointer to data
lData         - length of data
plProcessed   - Amount of data consumed
*/
HRESULT CT264Parser::Process(
                              const BYTE * pbData,
                              LONG lData,
                              LONG *plProcessed
                              )
{
    /*  Just loop processing packets until we run out of data
    We should do a lot more to sync up than just eat a start
    code !
    */
    // we do not to parse anything, the decoder do itself!
    m_Video.m_pNotify->SendSample(
        pbData,
        lData,
        0,
        false);
    *plProcessed = lData;
/*
    DWORD dwLeft = lData;
    const BYTE * pbCurrent = pbData;

    pbCurrent += 4;
    DWORD dwCode = 0xffffff00;
    BOOL bSend = false;
    DWORD dwSend = 0;
    while (dwLeft > 4) 
    {
        //  Find a start code 
        dwCode = (dwCode << 8) | (*pbCurrent ++);
        if (dwCode == 0x00000001)
        {
            m_Video.m_pNotify->SendSample(
                pbData,
                pbCurrent - pbData - 4,
                0,
                false);
            bSend = true;
            dwSend += pbCurrent - pbData - 4;
            pbData = pbCurrent - 4;
        }
        dwLeft --;
    }
    if (bSend)
    {
        *plProcessed = dwSend;
    }
    else
    {
        m_Video.m_pNotify->SendSample(
            pbData,
            lData,
            0,
            false);
        *plProcessed = lData;
    }
    */
    return S_OK;
}

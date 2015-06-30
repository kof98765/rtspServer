#define OUTPIN_BUFFER_SIZE (1024*1024)

#include "..\common\t264.h"
#include "IProp.h"
#include "pullpin.h"
#include "alloc.h"
#include "splitter.h"

class CT264Enc: public CTransformFilter,
                public IProp,
                public ISpecifyPropertyPages
{
public:
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    // Reveals IContrast & ISpecifyPropertyPages
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    DECLARE_IUNKNOWN;

    HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
    HRESULT CheckInputType(const CMediaType *mtIn);
    HRESULT CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc,
        ALLOCATOR_PROPERTIES *pProperties);
    HRESULT StartStreaming();
    HRESULT StopStreaming();

    // ISpecifyPropertyPages method
    STDMETHODIMP GetPages(CAUUID *pPages);

    // IProp
    HRESULT __stdcall get_Para(INT** pPara);
    HRESULT __stdcall put_Default();
    HRESULT __stdcall put_InfoWnd(INT hWnd);

    CT264Enc(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
    ~CT264Enc();
    HRESULT Transform(IMediaSample *pMediaSample);
    HRESULT Copy(IMediaSample *pSource, IMediaSample *pDest) const;
    HRESULT InitOutMediaType(CMediaType* pOut);
private:
    T264_t* m_t264;
    T264_param_t m_param;
    BYTE* m_pBuffer;
    LONGLONG m_avgFrameTime;
    HWND m_hWnd;
    TCHAR m_szInfo[_MAX_PATH];
};

class CT264Dec: public CTransformFilter
{
public:
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN punk, HRESULT *phr);

    // Reveals IContrast & ISpecifyPropertyPages
    STDMETHODIMP NonDelegatingQueryInterface(REFIID riid, void ** ppv);

    DECLARE_IUNKNOWN;

    HRESULT Transform(IMediaSample *pIn, IMediaSample *pOut);
    HRESULT CheckInputType(const CMediaType *mtIn);
    HRESULT CheckTransform(const CMediaType *mtIn,const CMediaType *mtOut);
    HRESULT GetMediaType(int iPosition, CMediaType *pMediaType);
    HRESULT DecideBufferSize(IMemAllocator *pAlloc,
        ALLOCATOR_PROPERTIES *pProperties);
    HRESULT StartStreaming();
    HRESULT StopStreaming();

    CT264Dec(TCHAR *tszName, LPUNKNOWN punk, HRESULT *phr);
    ~CT264Dec();
    HRESULT Transform(IMediaSample *pMediaSample);
    HRESULT Copy(IMediaSample *pSource, IMediaSample *pDest) const;
    HRESULT InitOutMediaType(CMediaType* pOut);

    HRESULT SendSample(T264_t* t, T264_frame_t *frame, IMediaSample* pSample);
    HRESULT Receive(IMediaSample* pSample);
    HRESULT CompleteConnect(PIN_DIRECTION direction, IPin *pReceivePin);

private:
    T264_t* m_t264;
    LONGLONG m_avgFrameTime;
    HWND m_hWnd;
    TCHAR m_szInfo[_MAX_PATH];
    INT m_nWidth;
    INT m_nHeight;
    float m_framerate;
    CRefTime m_time;
    INT m_nStride;
    IMemInputPin* m_pNextFilterInputpin;
};

class CT264Splitter: public CBaseSplitterFilter
{
public:
    /* This goes in the factory template table to create new instances */
    static CUnknown * WINAPI CreateInstance(LPUNKNOWN, HRESULT *);

    //  Constructor and destructor
    CT264Splitter(
        LPUNKNOWN pUnk,
        HRESULT *phr);

    //  Support self-registration
    LPAMOVIESETUP_FILTER GetSetupData();

    //  Override type checking
    HRESULT CheckInputType(const CMediaType *pmt);

    //  Create specific parser
    CBaseParser *CreateParser(CParserNotify *pNotify, CMediaType *pType);
};

/*  Parser */
class CT264Parser : public CBaseParser
{
public:
    CT264Parser(CParserNotify *pNotify, HRESULT *phr) :
      CBaseParser(pNotify, phr),
          m_bGotFirstPts(FALSE),
          m_llFirstPts(0)
      {
      }

      /*  Initialize a parser

      pmt     - type of stream if known - can be NULL
      pRdr    - way to read the source medium - can be NULL
      */
      HRESULT Init(CParseReader *pRdr);


      /*  Get the size and count of buffers preferred based on the
      actual content
      */
      void GetSizeAndCount(LONG *plSize, LONG *plCount);

      /*  Call this to reinitialize for a new stream */
      void StreamReset();

      /*  Call this to pass new stream data :

      pbData        - pointer to data
      lData         - length of data
      plProcessed   - Amount of data consumed
      */
      HRESULT Process(
          const BYTE * pbData,
          LONG lData,
          LONG *plProcessed
          );

private:
    /*  Parsing structures */
    class CStream
    {
    public:
        CStream():
            m_pNotify(NULL) {}
            BOOL Initialized()
            {
                return m_pNotify != NULL;
            }
            CStreamNotify *m_pNotify;
    };
    CStream  m_Video;
    LONGLONG m_llFirstPts;
    BOOL     m_bGotFirstPts;
};

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

    // {fd5010a2-8ebe-11ce-8183-00aa00577da1}
    DEFINE_GUID(IID_IProp,
        0xfd5010a2, 0x8ebe, 0x11ce, 0x81, 0x83, 0x00, 0xaa, 0x00, 0x57, 0x7d, 0xa1);

    DECLARE_INTERFACE_(IProp, IUnknown)
    {
        STDMETHOD(get_Para) (THIS_
            INT** pPara
            ) PURE;

        STDMETHOD(put_Default) (THIS) PURE;

        STDMETHOD(put_InfoWnd)(THIS_
            INT hWnd
            ) PURE;
    };

#ifdef __cplusplus
}
#endif


#pragma once

interface IProp;

class CT264EncProp : public CBasePropertyPage
{
public:
    CT264EncProp(LPUNKNOWN lpunk, HRESULT *phr);
    ~CT264EncProp();

    static CUnknown * WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT *phr);
private:

    BOOL OnReceiveMessage(HWND hwnd,UINT uMsg,WPARAM wParam,LPARAM lParam);
    HRESULT OnConnect(IUnknown *pUnknown);
    HRESULT OnDisconnect();
    HRESULT OnActivate();
    HRESULT OnDeactivate();
    HRESULT OnApplyChanges();

    void SetDirty();

    IProp *m_pProp;
    HWND m_hWnd;
};

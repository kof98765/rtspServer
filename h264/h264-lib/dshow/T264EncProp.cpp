#include "StdAfx.h"
#include "t264encprop.h"
#include "initguid.h"
#include "iprop.h"
#include "resource.h"
#include "..\common\t264.h"
#include "stdio.h"

//
// CreateInstance
//
// This goes in the factory template table to create new filter instances
//
CUnknown * WINAPI CT264EncProp::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);

    CUnknown *punk = new CT264EncProp(lpunk, phr);

    if(punk == NULL)
    {
        if (phr)
            *phr = E_OUTOFMEMORY;
    }

    return punk;

} // CreateInstance


//
// Constructor
//
CT264EncProp::CT264EncProp(LPUNKNOWN pUnk, HRESULT *phr) :
CBasePropertyPage(NAME("T264 Property Page"), pUnk,
                  IDD_DIALOG_PROP, IDS_TITLE)
{
    m_pProp = 0;
} // (Constructor)

CT264EncProp::~CT264EncProp(void)
{
}

//
// SetDirty
//
// Sets m_bDirty and notifies the property page site of the change
//
void CT264EncProp::SetDirty()
{
    m_bDirty = TRUE;

    if(m_pPageSite)
    {
        m_pPageSite->OnStatusChange(PROPPAGESTATUS_DIRTY);
    }

} // SetDirty


//
// OnReceiveMessage
//
// Virtual method called by base class with Window messages
//
BOOL CT264EncProp::OnReceiveMessage(HWND hwnd,
                                           UINT uMsg,
                                           WPARAM wParam,
                                           LPARAM lParam)
{
    switch(uMsg)
    {
    case WM_INITDIALOG:
        {
            m_hWnd = hwnd;
            return (LRESULT) 1;
        }

    case WM_VSCROLL:
        {
            return (LRESULT) 1;
        }

    case WM_COMMAND:
        {
            INT lcode = LOWORD(wParam);
            INT hcode = HIWORD(wParam);
            if(lcode == IDC_BUTTON_DEFAULT)
            {
                m_pProp->put_Default();
                SetDirty();
                OnActivate();
            }
            if (hcode == EN_CHANGE)
            {
                SetDirty();
            }

            return (LRESULT) 1;
        }

    case WM_DESTROY:
        {
            return (LRESULT) 1;
        }

    }

    return CBasePropertyPage::OnReceiveMessage(hwnd,uMsg,wParam,lParam);

} // OnReceiveMessage

//
// OnConnect
//
// Called when the property page connects to a filter
//
HRESULT CT264EncProp::OnConnect(IUnknown *pUnknown)
{
    ASSERT(m_pProp == NULL);
    CheckPointer(pUnknown,E_POINTER);

    HRESULT hr = pUnknown->QueryInterface(IID_IProp, (void **) &m_pProp);
    if(FAILED(hr))
    {
        return E_NOINTERFACE;
    }

    ASSERT(m_pProp);

    OnActivate();
    
    return NOERROR;

} // OnConnect


//
// OnDisconnect
//
// Called when we're disconnected from a filter
//
HRESULT CT264EncProp::OnDisconnect()
{
    // Release of Interface after setting the appropriate contrast value
    if (!m_pProp)
        return E_UNEXPECTED;

    m_pProp->Release();
    m_pProp = NULL;

    return NOERROR;

} // OnDisconnect


//
// OnDeactivate
//
// We are being deactivated
//
HRESULT CT264EncProp::OnDeactivate(void)
{
    // Remember the present contrast level for the next activate
    TCHAR szBuf[20];
    T264_param_t* param;

    ASSERT(m_pProp);

    m_pProp->get_Para((INT**)&param);

    BOOL bSuceed;
    param->iframe = GetDlgItemInt(m_hWnd, IDC_EDIT_IFRAME, &bSuceed, false);
    param->bitrate = GetDlgItemInt(m_hWnd, IDC_EDIT_BITRATE, &bSuceed, false);
    param->qp = GetDlgItemInt(m_hWnd, IDC_EDIT_IQP, &bSuceed, false);
    param->min_qp = GetDlgItemInt(m_hWnd, IDC_EDIT_MINQP, &bSuceed, false);
    param->max_qp = GetDlgItemInt(m_hWnd, IDC_EDIT_MAXQP, &bSuceed, false);
    param->search_x = param->search_y = GetDlgItemInt(m_hWnd, IDC_EDIT_SEARCH, &bSuceed, false);
    param->ref_num = GetDlgItemInt(m_hWnd, IDC_EDIT_REFNUM, &bSuceed, false);
    param->b_num = GetDlgItemInt(m_hWnd, IDC_EDIT_BFRAMENUM, &bSuceed, false);
    GetDlgItemText(m_hWnd, IDC_EDIT_FRAMEFATE, szBuf, 20);
    param->framerate = (float)atof(szBuf);

    m_pProp->put_InfoWnd(0);
    if (param->bitrate != 0)
        param->enable_rc = 1;

    return NOERROR;

} // OnDeactivate

HRESULT CT264EncProp::OnActivate(void)
{
    // Remember the present contrast level for the next activate
    HWND hWnd = GetDlgItem(m_hWnd, IDC_STATIC_INFO);
    m_pProp->put_InfoWnd((INT)hWnd);
    
    T264_param_t* param;
    TCHAR szBuf[20];

    ASSERT(m_pProp);

    m_pProp->put_InfoWnd((INT)hWnd);
    m_pProp->get_Para((INT**)&param);

    SetDlgItemInt(m_hWnd, IDC_EDIT_IFRAME, param->iframe, false);
    SetDlgItemInt(m_hWnd, IDC_EDIT_BITRATE, param->bitrate, false);
    SetDlgItemInt(m_hWnd, IDC_EDIT_IQP, param->qp, false);
    SetDlgItemInt(m_hWnd, IDC_EDIT_MINQP, param->min_qp, false);
    SetDlgItemInt(m_hWnd, IDC_EDIT_MAXQP, param->max_qp, false);
    SetDlgItemInt(m_hWnd, IDC_EDIT_SEARCH, param->search_x, false);
    SetDlgItemInt(m_hWnd, IDC_EDIT_REFNUM, param->ref_num, false);
    SetDlgItemInt(m_hWnd, IDC_EDIT_BFRAMENUM, param->b_num, false);
    sprintf(szBuf, "%.2f", param->framerate);
    SetDlgItemText(m_hWnd, IDC_EDIT_FRAMEFATE, szBuf);
    sprintf(szBuf, "T264 ver: %d.%02d", T264_MAJOR, T264_MINOR);
    SetDlgItemText(m_hWnd, IDC_STATIC_VER, szBuf);

    return NOERROR;

} // OnDeactivate

//
// OnApplyChanges
//
// Changes made should be kept. Change the m_cContrastOnExit variable
//
HRESULT CT264EncProp::OnApplyChanges()
{
    T264_param_t* param;
    TCHAR szBuf[20];

    ASSERT(m_pProp);

    m_pProp->get_Para((INT**)&param);
    m_bDirty = FALSE;

    BOOL bSuceed;
    param->iframe = GetDlgItemInt(m_hWnd, IDC_EDIT_IFRAME, &bSuceed, false);
    param->bitrate = GetDlgItemInt(m_hWnd, IDC_EDIT_BITRATE, &bSuceed, false);
    param->qp = GetDlgItemInt(m_hWnd, IDC_EDIT_IQP, &bSuceed, false);
    param->min_qp = GetDlgItemInt(m_hWnd, IDC_EDIT_MINQP, &bSuceed, false);
    param->max_qp = GetDlgItemInt(m_hWnd, IDC_EDIT_MAXQP, &bSuceed, false);
    param->search_x = param->search_y = GetDlgItemInt(m_hWnd, IDC_EDIT_SEARCH, &bSuceed, false);
    param->ref_num = GetDlgItemInt(m_hWnd, IDC_EDIT_REFNUM, &bSuceed, false);
    param->b_num = GetDlgItemInt(m_hWnd, IDC_EDIT_BFRAMENUM, &bSuceed, false);
    GetDlgItemText(m_hWnd, IDC_EDIT_FRAMEFATE, szBuf, 20);
    param->framerate = (float)atof(szBuf);
    if (param->bitrate != 0)
        param->enable_rc = 1;
    else
        param->enable_rc = 0;

    return(NOERROR);

} // OnApplyChanges


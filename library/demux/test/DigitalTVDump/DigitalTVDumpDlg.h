// DigitalTVDumpDlg.h : header file
//

#pragma once
#include "afxwin.h"


// CDigitalTVDumpDlg dialog
class CDigitalTVDumpDlg : public CDialog
{
// Construction
public:
	CDigitalTVDumpDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_DIGITALTVDUMP_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDropFiles(HDROP hDropInfo);
public:
	CButton m_AudioCheck;
public:
	CEdit m_InputFileEdit;
public:
	CEdit m_AudioDumpEdit;
public:
	CEdit m_VisualDumpEdit;
public:
	CButton m_VisualCheck;
public:
	afx_msg void OnBnClickedButtonDump();
public:
public:
public:
	afx_msg void OnBnClickedRadioOneseg();
public:
	afx_msg void OnBnClickedRadioTdmb();
public:
	CButton m_DumpButton;
};

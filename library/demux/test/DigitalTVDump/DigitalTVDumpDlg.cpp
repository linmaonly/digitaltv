/*
* Copyright (c) 2006 BIT Everest, 
* Author: Lin Ma, 
* linmaonly@gmail.com
* htpp://biteverest.googlepages.com
* 
* Permission is hereby granted, free of charge, to any person
* obtaining a copy of this software and associated documentation
* files (the "Software"), to deal in the Software without
* restriction, including without limitation the rights to use,
* copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following
* conditions:
* 
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
* OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
* HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
* WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
* OTHER DEALINGS IN THE SOFTWARE.
*/


#include "stdafx.h"
#include "DigitalTVDump.h"
#include "DigitalTVDumpDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CDigitalTVDumpDlg dialog




CDigitalTVDumpDlg::CDigitalTVDumpDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CDigitalTVDumpDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDigitalTVDumpDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_CHECK_AUDIO, m_AudioCheck);
	DDX_Control(pDX, IDC_EDIT_INPUT, m_InputFileEdit);
	DDX_Control(pDX, IDC_EDIT_AUDIO_DUMP, m_AudioDumpEdit);
	DDX_Control(pDX, IDC_EDIT_VIDEO_DUMP, m_VisualDumpEdit);
	DDX_Control(pDX, IDC_CHECK_VIDEO, m_VisualCheck);
	DDX_Control(pDX, IDC_BUTTON_DUMP, m_DumpButton);
}

BEGIN_MESSAGE_MAP(CDigitalTVDumpDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_DROPFILES()
	ON_BN_CLICKED(IDC_BUTTON_DUMP, &CDigitalTVDumpDlg::OnBnClickedButtonDump)
	ON_BN_CLICKED(IDC_RADIO_ONESEG, &CDigitalTVDumpDlg::OnBnClickedRadioOneseg)
	ON_BN_CLICKED(IDC_RADIO_TDMB, &CDigitalTVDumpDlg::OnBnClickedRadioTdmb)
END_MESSAGE_MAP()


// CDigitalTVDumpDlg message handlers

BOOL CDigitalTVDumpDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here
	m_AudioCheck.SetCheck(BST_CHECKED);
	m_VisualCheck.SetCheck(BST_CHECKED);
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CDigitalTVDumpDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CDigitalTVDumpDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CDigitalTVDumpDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

static TCHAR g_pszFileName[1024*4];
static TCHAR g_pszAudioDump[1024*4];
static TCHAR g_pszVisualDump[1024*4];
static BOOL g_bValidFileName = FALSE;
static BOOL g_bOneSeg = FALSE;
static BOOL g_bTDMB = FALSE;
void CDigitalTVDumpDlg::OnDropFiles(HDROP hDropInfo)
{
	// TODO: Add your message handler code here and/or call default
	UINT uCount = DragQueryFile(hDropInfo, 0xFFFFFFFF, NULL, 0);
	if (uCount > 1)
	{
		MessageBox(TEXT("No more than file."));
	}
	uCount = DragQueryFile(hDropInfo, 0, g_pszFileName, 1024*4);
	memcpy(g_pszAudioDump, g_pszFileName, uCount);
	memcpy(g_pszAudioDump+uCount, ".audio.dump\0", 12);
	memcpy(g_pszVisualDump, g_pszFileName, uCount);
	memcpy(g_pszVisualDump+uCount, ".264\0", 5);
	m_InputFileEdit.SetWindowText(g_pszFileName);
	m_AudioDumpEdit.SetWindowText(g_pszAudioDump);
	m_VisualDumpEdit.SetWindowText(g_pszVisualDump);
	g_bValidFileName = TRUE;
	CDialog::OnDropFiles(hDropInfo);
	DragFinish(hDropInfo);
}
extern "C"  int TdmbDump(char *pszInput, char *pszAudioDump, char *pszVisualDump);
extern "C"  int OneSegDump(char *pszInput, char *pszAudioDump, char *pszVisualDump);

void CDigitalTVDumpDlg::OnBnClickedButtonDump()
{
	// TODO: Add your control notification handler code here
	BOOL bAudioDump = m_AudioCheck.GetCheck() == BST_CHECKED;
	BOOL bVisualDump = m_VisualCheck.GetCheck() == BST_CHECKED;
	if (!g_bValidFileName)
	{
		MessageBox(TEXT("Please drop a digital TV stream file to this dialog"));
		return;
	}
	if (!g_bTDMB && !g_bOneSeg)
	{
		MessageBox(TEXT("Select the type."));
		return;
	}
	::SetDlgItemText(m_hWnd, IDC_STATIC_INFO, TEXT("Please wait for a while. Dumping is done in the GUI thread"));
	m_DumpButton.EnableWindow(FALSE);
	if (g_bTDMB)
		TdmbDump(g_pszFileName, g_pszAudioDump, g_pszVisualDump);
	else
		OneSegDump(g_pszFileName, g_pszAudioDump, g_pszVisualDump);
	::SetDlgItemText(m_hWnd, IDC_STATIC_INFO, TEXT("Dump finished"));
	m_DumpButton.EnableWindow(TRUE);
}

void CDigitalTVDumpDlg::OnBnClickedRadioOneseg()
{
	// TODO: Add your control notification handler code here
	g_bOneSeg = TRUE;
	g_bTDMB = FALSE;
}

void CDigitalTVDumpDlg::OnBnClickedRadioTdmb()
{
	// TODO: Add your control notification handler code here
	g_bTDMB = TRUE;
	g_bOneSeg = FALSE;
}

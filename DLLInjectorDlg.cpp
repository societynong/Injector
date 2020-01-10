
// DLLInjectorDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "DLLInjector.h"
#include "DLLInjectorDlg.h"
#include "afxdialogex.h"
#include "Winbase.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CDLLInjectorDlg 对话框



CDLLInjectorDlg::CDLLInjectorDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DLLINJECTOR_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CDLLInjectorDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CDLLInjectorDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUT_SELECT, &CDLLInjectorDlg::OnBnClickedButSelect)
	ON_BN_CLICKED(IDC_BUT_INJECT, &CDLLInjectorDlg::OnBnClickedButInject)
END_MESSAGE_MAP()


// CDLLInjectorDlg 消息处理程序

BOOL CDLLInjectorDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CDLLInjectorDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CDLLInjectorDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CDLLInjectorDlg::OnBnClickedButSelect()
{
	// TODO: 在此添加控件通知处理程序代码
	CFileDialog  fdlg(TRUE,NULL,NULL, OFN_READONLY,0,NULL);
	if (fdlg.DoModal())
	{
		m_sDLLPath = fdlg.GetPathName();
		SetDlgItemText(IDC_EDIT_DLLPATH, m_sDLLPath);
	}


}





void CDLLInjectorDlg::OnBnClickedButInject()
{
	// TODO: 在此添加控件通知处理程序代码
	HWND hwd = FindWindowA(NULL, "红警2修改器");
	DWORD pid = 0;
	GetWindowThreadProcessId(hwd, &pid);
	CString strPID;
	strPID.Format(_T("%d"), pid);
	//GetDlgItemText(IDC_EDIT_PID, strPID);
	SetDlgItemText(IDC_SHOW_PID, strPID);
	//int pid = _ttoi(strPID);
	
	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
	
	DWORD Len_AppPath = lstrlen(m_sDLLPath) + 1;
	// Allocate space in the remote process for the pathname
	LPVOID pszLibFileRemote = (PWSTR)VirtualAllocEx(hProcess, NULL, Len_AppPath, MEM_COMMIT, PAGE_READWRITE);
	if (pszLibFileRemote == NULL)
	{
		MessageBox(NULL,TEXT("[-] Error: Could not allocate memory inside PID (%d).\n"));
		return;
	}

	WCHAR* cDLLPath = m_sDLLPath.GetBuffer();
	// Copy the DLL's pathname to the remote process address space
	DWORD n = WriteProcessMemory(hProcess, pszLibFileRemote, m_sDLLPath, Len_AppPath * sizeof(WCHAR), NULL);
	if (n == 0)
	{
		MessageBox(NULL, TEXT("[-] Error: Could not write any bytes into the PID [%d] address space.\n"));
		return ;
	}

	// 获得 LoadLibraryW 在 kernel32.dll 中的地址
	typedef HMODULE(WINAPI* pfnLoadLibrary)(LPCWSTR);
	// 这里注意要载入宽字节版本还是普通版本的 LoadLibrary  
	PTHREAD_START_ROUTINE pfnThreadRtn2 = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("Kernel32.dll")), "LoadLibraryW");

	// Get the real address of LoadLibraryW in Kernel32.dll
	PTHREAD_START_ROUTINE pfnThreadRtn = (PTHREAD_START_ROUTINE)GetProcAddress(GetModuleHandle(TEXT("Kernel32")), "LoadLibrary");
	if (pfnThreadRtn2 == NULL)
	{
		MessageBox(TEXT("[-] Error: Could not find LoadLibraryA function inside kernel32.dll library.\n"));
		return ;
	}

	// Create a remote thread that calls LoadLibraryW(DLLPathname)
	HANDLE hThread = CreateRemoteThread(hProcess, NULL, 0, pfnThreadRtn2, pszLibFileRemote, 0, NULL);
	if (hThread == NULL)
	{
		MessageBox(TEXT("[-] Error: Could not create the Remote Thread.\n"));
		return ;
	}

	// Wait for the remote thread to terminate
	WaitForSingleObject(hThread, INFINITE);

	// Free the remote memory that contained the DLL's pathname and close Handles
	if (pszLibFileRemote != NULL)
		VirtualFreeEx(hProcess, pszLibFileRemote, 0, MEM_RELEASE);

	if (hThread != NULL)
		CloseHandle(hThread);

	if (hProcess != NULL)
		CloseHandle(hProcess);;
}

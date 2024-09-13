/******************************************************
 *                  QuickPanel.cpp                    *
 *                 系统快速操纵面板                   *
 *                Author: Wormwaker                   *
 *              StartDate: 2023/12/6                  *
 *              All Rights Reserved.                  *
 *               Do Not Distribute.                   *
 ******************************************************/
#define CURRENT_VERSION "v1.0"
#define NO_GENSHIN_IMPACT_BOOT_HOTKEY
//#define SHOWCONSOLE
//Extra Compile Options:
//-lgdi32 -lmsimg32 -lwer
#define DRAGACCEPT
#define _WIN32_WINNT 0x602
#define HREPORT void*
#define PWER_SUBMIT_RESULT int*
#define WER_MAX_PREFERRED_MODULES_BUFFER 10
#include <Windows.h>
#include <winternl.h>
#include <tlhelp32.h>
#include <werapi.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <array>
#include <queue>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <sys/stat.h>
using namespace std;
#define PI 3.14159265358979323846f
#define fequ(f1,f2)	(abs(f1-f2) < 0.001f)
#define FOCUSED		  (GetForegroundWindow()==hwnd)
#define KEY_DOWN(sth) (GetAsyncKeyState(sth) & 0x8000 ? 1 : 0)
#define K(sth)		  (KEY_DOWN(sth) && FOCUSED)
#define CJZAPI __stdcall
#define DESTRUCTIVE
#define DANGEROUS

typedef USHORT DIR,*PDIR;	//方向
#define UP 0x01
#define DIR_FIRST UP
#define RIGHTUP 0x02
#define RIGHT 0x03
#define RIGHTDOWN 0x04
#define DOWN 0x05
#define LEFTDOWN 0x06
#define LEFT 0x07
#define LEFTUP 0x08
#define DIR_LAST LEFTUP

#define QPWND_CLASS_NAME "QPWC"
#define QPWND_CAPTION "QuickPanel " CURRENT_VERSION

HINSTANCE _hInstance{nullptr};
HINSTANCE _hPrevInstance{nullptr};
LPSTR _lpCmdLine{nullptr};
int _nShowCmd;

int scr_w = 0, scr_h = 0, taskbar_h = 0;
HWND hwnd = nullptr;
HWND hwnd_hiddenEdit{nullptr}; 
HWND hwnd_console{nullptr};
bool focused = false;

template <typename _T>
string CJZAPI str(const _T& value)
{
	stringstream ss;
	ss << value;
	string res;
	ss >> res;
	return res;
}
template <typename _T>
inline LPCSTR CJZAPI cstr(const _T& value)
{
	return str(value).c_str();
}
string CJZAPI strtail(const string& s, int cnt = 1) {
	//012 len=3
	//abc   s.substr(2,1)
	if (cnt > s.size())
		return s;
	return s.substr(s.size() - cnt, cnt);
}
string CJZAPI strhead(const string& s, int cnt = 1) {
	if (cnt > s.size())
		return s;
	return s.substr(0, cnt);
}
string CJZAPI strxtail(const string& s, int cnt = 1) {
	if (cnt > s.size())
		return "";
	return s.substr(0, s.size() - cnt);
}
string CJZAPI strxhead(const string& s, int cnt = 1) {
	if (cnt > s.size())
		return "";
	return s.substr(cnt, s.size() - cnt);
}
string CJZAPI strip(const string& s)
{
	string res = s;
	while(!res.empty() && (res.at(0) == '\r' || res.at(0) == '\n' || res.at(0) == '\0'))
		res = res.substr(1, res.size() - 1);
	while(!res.empty() && (res.at(res.size()-1) == '\r' || res.at(res.size()-1) == '\n' || res.at(0) == '\0'))
		res = res.substr(0, res.size() - 1);
	return res;
}
string CJZAPI sprintf2(const char* szFormat, ...)
{
	va_list _list;
	va_start(_list, szFormat);
	char szBuffer[1024] = "\0";
	_vsnprintf(szBuffer, 1024, szFormat, _list);
	va_end(_list);
	return string{szBuffer};
}
// wstring=>string
string WString2String(const wstring &ws)
{
	string strLocale = setlocale(LC_ALL, "");
	const wchar_t *wchSrc = ws.c_str();
	size_t nDestSize = wcstombs(NULL, wchSrc, 0) + 1;
	char *chDest = new char[nDestSize];
	memset(chDest, 0, nDestSize);
	wcstombs(chDest, wchSrc, nDestSize);
	string strResult = chDest;
	delete[] chDest;
	setlocale(LC_ALL, strLocale.c_str());
	return strResult;
}

// string => wstring
wstring String2WString(const string &s)
{
	string strLocale = setlocale(LC_ALL, "");
	const char *chSrc = s.c_str();
	size_t nDestSize = mbstowcs(NULL, chSrc, 0) + 1;
	wchar_t *wchDest = new wchar_t[nDestSize];
	wmemset(wchDest, 0, nDestSize);
	mbstowcs(wchDest, chSrc, nDestSize);
	wstring wstrResult = wchDest;
	delete[] wchDest;
	setlocale(LC_ALL, strLocale.c_str());
	return wstrResult;
}
//from https://blog.csdn.net/qq_30386941/article/details/126814596
int CJZAPI RandomRange(int Min, int Max, bool rchMin, bool rchMax)
{ //随机数值区间 
	int a;	
	a = rand();
	if(rchMin && rchMax)	//[a,b]
		return (a%(Max - Min + 1)) + Min;
	else if(rchMin && !rchMax)		//[a,b)
		return (a%(Max - Min)) + Min;
	else if(!rchMin && rchMax)		//(a,b]
		return (a%(Max - Min)) + Min + 1;
	else							//(a,b)
		return (a%(Max - Min - 1)) + Min + 1;
}
int CJZAPI Randint(int Min, int Max)
{	//闭区间随机值
	return RandomRange(Min, Max, true, true);
}
template <typename _T>
inline _T CJZAPI ClampA(_T& val, _T min=0, _T max=2147483647) {	//限定范围
	if(val < min) val = min;
	else if(val > max) val = max;
	return val;
}
template <typename _T>
inline _T CJZAPI Clamp(_T val, _T min=0, _T max=2147483647) {	//限定范围
	if(val < min) val = min;
	else if(val > max) val = max;
	return val;
}
template <typename _T>
inline double CJZAPI Lerp(_T startValue, _T endValue, double ratio)
{
	return startValue + (endValue - startValue) * ratio;
}
template <typename _T>
inline double CJZAPI LerpClamp(_T startValue, _T endValue, double ratio)
{
	return Clamp<double>(Lerp(startValue,endValue,ratio), min(startValue,endValue), max(endValue,startValue));
}
inline double CJZAPI EaseInExpo(double _x)
{
	return fequ(_x, 0.0) ? 0 : pow(2, 10 * _x - 10);
}
inline double CJZAPI EaseInOutSine(double _x)
{	//retval,_x ∈ [0,1]
	return -(cos(PI * _x) - 1) / 2;
}
inline double CJZAPI EaseInOutBack(double _x)
{
	const double c1 = 1.70158;
	const double c2 = c1 * 1.525;
	return _x < 0.5
	? (pow(2 * _x, 2) * ((c2 + 1) * 2 * _x - c2)) / 2
	: (pow(2 * _x - 2, 2) * ((c2 + 1) * (_x * 2 - 2) + c2) + 2) / 2;
}
inline double CJZAPI EaseOutCubic(double _x)
{
	return 1 - pow(1 - _x, 3);
}
inline double CJZAPI EaseInOutCubic(double _x)
{
	return _x < 0.5 ? 4 * _x * _x * _x : 1 - pow(-2 * _x + 2, 3) / 2;
}
inline double CJZAPI EaseInOutElastic(double _x)
{
	const double c5 = (2 * PI) / 4.5;
	return fequ(_x, 0.0)
	? 0.0
	: fequ(_x, 1.0)
	? 1.0
	: _x < 0.5
	? -(pow(2, 20 * _x - 10) * sin((20 * _x - 11.125) * c5)) / 2
	: (pow(2, -20 * _x + 10) * sin((20 * _x - 11.125) * c5)) / 2 + 1;
}
inline double CJZAPI EaseOutBounce(double _x)
{
	const double n1 = 7.5625;
	const double d1 = 2.75;
	
	if (_x < 1 / d1) {
		return n1 * _x * _x;
	}
	else if (_x < 2 / d1) {
		return n1 * (_x -= 1.5 / d1) * _x + 0.75;
	}
	else if (_x < 2.5 / d1) {
		return n1 * (_x -= 2.25 / d1) * _x + 0.9375;
	}
	else {
		return n1 * (_x -= 2.625 / d1) * _x + 0.984375;
	}
}
inline double CJZAPI EaseInOutBounce(double _x)
{
	return _x < 0.5
	? (1 - EaseOutBounce(1 - 2 * _x)) / 2
	: (1 + EaseOutBounce(2 * _x - 1)) / 2;
}
inline double CJZAPI EaseInOutExpo(double _x)
{
	return fequ(_x, 0.0)
	? 0.0
	: fequ(_x, 1.0)
	? 1.0
	: _x < 0.5 ? pow(2, 20 * _x - 10) / 2
	: (2 - pow(2, -20 * _x + 10)) / 2;
}
inline BOOL CJZAPI ExistFile(string strFile) {
	//文件或文件夹都可以
	return !_access(strFile.c_str(),S_OK);//S_OK表示只检查是否存在
}
BOOL CJZAPI IsDir(const string& lpPath)
{	//是否是文件夹 
	int res;
	struct _stat buf;
	res = _stat(lpPath.c_str(), &buf);
	return (buf.st_mode & _S_IFDIR);
}
BOOL CJZAPI IsFile(const string& lpPath)
{	//是否是文件 
	int res;
	struct _stat buf;
	res = _stat(lpPath.c_str(), &buf);
	return (buf.st_mode & _S_IFREG);
}
string CJZAPI GetFileDirectory(string path)
{	//返回的最后会有反斜线
	if (IsDir(path))
	{
		if (path.substr(path.size() - 1, 1) != "\\")
			path += "\\";
		return path;
	}
	string ret = "";
	bool suc = false;
	for (int i = path.size() - 1; i >= 0; i--)
	{
		if (path.at(i) == '\\')
		{
			ret = path.substr(0, i + 1);
			suc = true;
			break;
		}
	}
	if (!suc)
		SetLastError(3L);	//还是要return的
	if (ret.substr(ret.size() - 1, 1) != "\\")
		ret += "\\";
	return ret;
}
vector<string> CJZAPI SplitToLines(string szParagraph, char lineEnding = '\n')
{	//把一段话分成每行 
	vector<string> ret{};
	int p1 = 0, p2;
	for (int i = 0; i < szParagraph.size(); i++)
	{
		if (szParagraph.at(i) == lineEnding
			|| i == szParagraph.size() - 1)	//别漏了最后一个数据 
		{
			p2 = i;
			string szNew = szParagraph.substr(p1, p2 - p1 + (i == szParagraph.size() - 1 ? 1 : 0));
			ret.push_back(szNew);
			p1 = i + 1;
		}
	}
	return ret;
}

bool TerminalCheck_HideAllTerminals(DWORD dwPid)
{	//检查是否为win11新终端
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);	
	if (INVALID_HANDLE_VALUE == hSnapshot) 	
	{		
		return false;	
	}	
	PROCESSENTRY32 pe = { sizeof(pe) };	
	BOOL fOk;	
	for (fOk = Process32First(hSnapshot, &pe); fOk; fOk = Process32Next(hSnapshot, &pe)) 	
	{		
		if (! stricmp(pe.szExeFile, "WindowsTerminal.exe")
			&& pe.th32ProcessID == dwPid) 		
		{			
			CloseHandle(hSnapshot);			
			return true;		
		}	
	}	
	return false;
}
BOOL CALLBACK EnumWindowsProc_HideAllTerminals(HWND _hwnd, LPARAM lParam)
{
	DWORD pid=0;
	GetWindowThreadProcessId(_hwnd, &pid);
	if(IsWindowVisible(_hwnd) && TerminalCheck_HideAllTerminals(pid))
		ShowWindow(_hwnd, SW_HIDE);
	return TRUE;
}
inline VOID CJZAPI HideAllTerminals()
{
	EnumWindows(EnumWindowsProc_HideAllTerminals, 0);
}
BOOL CJZAPI IsRunAsAdmin(HANDLE hProcess=GetCurrentProcess()) 
{	//是否有管理员权限 
	BOOL bElevated = FALSE;  	
	HANDLE hToken = NULL;   	// Get current process token	
	if ( !OpenProcessToken(hProcess, TOKEN_QUERY, &hToken ) )		
		return FALSE; 	
	TOKEN_ELEVATION tokenEle;	
	DWORD dwRetLen = 0;   	// Retrieve token elevation information	
	if ( GetTokenInformation( hToken, TokenElevation, &tokenEle, sizeof(tokenEle), &dwRetLen ) ) 	
	{  		
		if ( dwRetLen == sizeof(tokenEle) ) 		
		{			
			bElevated = tokenEle.TokenIsElevated;  		
		}	
	}   	
	CloseHandle( hToken );  	
	return bElevated;  
} 
VOID CJZAPI AdminRun(LPCSTR csExe, LPCSTR csParam=NULL, DWORD nShow=SW_SHOW)  
{ 	//以管理员身份运行
	SHELLEXECUTEINFO ShExecInfo; 
	ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);  
	ShExecInfo.fMask = SEE_MASK_NOCLOSEPROCESS ;  
	ShExecInfo.hwnd = NULL;  
	ShExecInfo.lpVerb = "runas";  
	ShExecInfo.lpFile = csExe/*"cmd"*/; 
	ShExecInfo.lpParameters = csParam;   
	ShExecInfo.lpDirectory = NULL;  
	ShExecInfo.nShow = nShow;  
	ShExecInfo.hInstApp = NULL;   
	BOOL ret = ShellExecuteEx(&ShExecInfo);  
	/*WaitForSingleObject(ShExecInfo.hProcess, INFINITE);  
	  GetExitCodeProcess(ShExecInfo.hProcess, &dwCode);  */
	CloseHandle(ShExecInfo.hProcess);
	return;
} //from https://blog.csdn.net/mpp_king/article/details/80194563

bool CJZAPI ExistProcess(LPCSTR lpName)	//判断是否存在指定进程
{	//******警告！区分大小写！！！！******// 
	//*****警告！必须加扩展名！！！！*****// 
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);	
	if (INVALID_HANDLE_VALUE == hSnapshot) 	
	{		
		return false;	
	}	
	PROCESSENTRY32 pe = { sizeof(pe) };	
	BOOL fOk;	
	for (fOk = Process32First(hSnapshot, &pe); fOk; fOk = Process32Next(hSnapshot, &pe)) 	
	{		
		if (! stricmp(pe.szExeFile, lpName)) 		
		{			
			CloseHandle(hSnapshot);			
			return true;		
		}	
	}	
	return false;
}
bool CJZAPI ExistProcess(DWORD dwPid)	//判断是否存在指定进程
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);	
	if (INVALID_HANDLE_VALUE == hSnapshot) 	
	{		
		return false;	
	}	
	PROCESSENTRY32 pe = { sizeof(pe) };	
	BOOL fOk;	
	for (fOk = Process32First(hSnapshot, &pe); fOk; fOk = Process32Next(hSnapshot, &pe)) 	
	{		
		if (pe.th32ProcessID == dwPid) 		
		{			
			CloseHandle(hSnapshot);			
			return true;		
		}	
	}	
	return false;
}
bool WINAPI ReportError(LPCWSTR lpPath, LPCWSTR lpCloseText, LPCWSTR lpDescription)
{
	HREPORT hReport;
	WER_REPORT_INFORMATION wri;
	ZeroMemory(&wri, sizeof(wri));
	wri.dwSize = sizeof(wri);
	wri.hProcess = GetCurrentProcess();
	lstrcpyW(wri.wzApplicationPath, lpPath);
	int wsr;
	WerReportCreate(L"QuickPanel", WerReportApplicationCrash, &wri, &hReport);
	WerReportSetUIOption(hReport, WerUIIconFilePath, lpPath);
	if(lpCloseText) WerReportSetUIOption(hReport, WerUICloseText, lpCloseText);
	
	if(lpDescription)
	{
		WCHAR DlgHeader[565];
		wsprintfW(DlgHeader, L"%s 早就他妈的停止了工作", lpDescription);
		WerReportSetUIOption(hReport, WerUIIconFilePath, lpPath);
		WerReportSetUIOption(hReport,WerUIConsentDlgHeader, DlgHeader);
	}
	WerReportSubmit(hReport, WerConsentNotAsked, 1024 | 8, &wsr);
	WerReportCloseHandle(hReport);
	return GetLastError()==0;
}
HMODULE hKernel = nullptr;
typedef WINBOOL (WINAPI *T_EnumProcessModules) (HANDLE, HMODULE*, DWORD, LPDWORD);
typedef WINBOOL (WINAPI *T_GetModuleFileNameExA) (HANDLE, HMODULE, LPSTR, DWORD);
string CJZAPI GetProcessPath(HANDLE hProc)	//获取进程EXE绝对路径
{
	//注意：动态调用！
	hKernel = LoadLibrary("KERNEL32.dll");	//核心32 DLL模块句柄
	if(hKernel == INVALID_HANDLE_VALUE || !hKernel)
	{
		return string("");
	}
	T_EnumProcessModules EPM = nullptr;		//函数变量 
	T_GetModuleFileNameExA GMFNE = nullptr;	//函数变量 
	EPM = (T_EnumProcessModules)GetProcAddress(hKernel, "K32EnumProcessModules");
	GMFNE = (T_GetModuleFileNameExA)GetProcAddress(hKernel, "K32GetModuleFileNameExA");
	if (!EPM || !GMFNE)
	{	//没找到 
		FreeLibrary(hKernel);
		return string("");
	}
	
	//关键操作 
	HMODULE hMod;
	DWORD need;
	CHAR thePath[MAX_PATH]{ 0 };
	EPM(hProc, &hMod, sizeof(hMod), &need);
	GMFNE(hProc, hMod, thePath, sizeof(thePath));
	FreeLibrary(hKernel);
	return string(thePath);
}
void RealUpdateWindow(HWND _hwnd)
{
	RECT rect;
	GetClientRect(_hwnd, &rect);
	InvalidateRect(_hwnd, &rect, TRUE);
	UpdateWindow(_hwnd);
}
void FocusWindow(HWND _hwnd)
{
	DWORD dwCurrentThread = GetCurrentThreadId();
	DWORD dwFGThread      = GetWindowThreadProcessId(GetForegroundWindow(), NULL);
	
	AttachThreadInput(dwCurrentThread, dwFGThread, TRUE);
	
	// Possible actions you may wan to bring the window into focus.
	SetForegroundWindow(_hwnd);
	SetCapture(_hwnd);
	SetFocus(_hwnd);
	SetActiveWindow(_hwnd);
	EnableWindow(_hwnd, TRUE);
	
	AttachThreadInput(dwCurrentThread, dwFGThread, FALSE);
}	//from https://www.coder.work/article/555749
inline int GetScreenHeight(void) //获取屏幕高度
{
	return GetSystemMetrics(SM_CYSCREEN);
}
inline int GetScreenWidth(void) //获取屏幕宽度
{
	return GetSystemMetrics(SM_CXSCREEN);
}
RECT CJZAPI GetSystemWorkAreaRect(void) //获取工作区矩形 
{
	RECT rt;
	SystemParametersInfo(SPI_GETWORKAREA,0,&rt,0);    // 获得工作区大小
	return rt;
}
LONG CJZAPI GetTaskbarHeight(void) 		//获取任务栏高度 
{	
	INT cyScreen = GetScreenHeight();
	RECT rt = GetSystemWorkAreaRect();
	return (cyScreen - (rt.bottom - rt.top));
}
inline HWND GetTaskbarWindow(void)
{
	return WindowFromPoint(POINT{ GetScreenWidth() / 2,GetScreenHeight() - 2 });
}
string GetProcessName(DWORD pid)
{
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
	if (INVALID_HANDLE_VALUE == hSnapshot)
	{
		return string("");
	}
	PROCESSENTRY32 pe = { sizeof(pe) };
	BOOL fOk;
	for (fOk = Process32First(hSnapshot, &pe); fOk; fOk = Process32Next(hSnapshot, &pe))
	{
		if (pe.th32ProcessID == pid)
		{
			CloseHandle(hSnapshot);
			//			  cout<<"text="<<pe.szExeFile;	
			//					pname=pe.szExeFile;	
			//			  return (LPSTR)pe.szExeFile;	
			return (string)pe.szExeFile;
		}
	}
	return string("");
}

HDC hdcOrigin{nullptr}, hdcBuffer{nullptr};
HFONT hFontText{nullptr};
HFONT hFontTextMinor{nullptr};

inline HFONT CJZAPI CreateFont(int height, int width, LPCSTR lpFamilyName)
{
	return CreateFont(height,width,0,0,FW_NORMAL,0,0,0,0,0,0,0,0,lpFamilyName);
}
#define WVC_AMP 12
#define WVC_OMEGA 13.0
#define WVC_PHASE0 0
clock_t lastWvBeg=0;
inline COLORREF WaveColor(COLORREF originClr, float phi=0.0f) {	//originClr将成为最大值
	//闪烁的颜色 赋予游戏文字灵性
	short val = WVC_AMP * sin(WVC_OMEGA*((clock()-lastWvBeg)/1000.0)+WVC_PHASE0+phi) - WVC_AMP*2;
	short r=GetRValue(originClr)+val,g=GetGValue(originClr)+val,b=GetBValue(originClr)+val;
	ClampA<short>(r,2,255);
	ClampA<short>(g,2,255);
	ClampA<short>(b,2,255);
	return RGB(r,g,b);
}
// 辅助函数：RGB到HSV颜色转换
void RGBtoHSV(COLORREF rgb, double& h, double& s, double& v) {
	double r = GetRValue(rgb) / 255.0;
	double g = GetGValue(rgb) / 255.0;
	double b = GetBValue(rgb) / 255.0;
	
	double minVal = std::min(std::min(r, g), b);
	double maxVal = std::max(std::max(r, g), b);
	double delta = maxVal - minVal;
	
	// 计算色相
	if (delta > 0) {
		if (maxVal == r)
			h = 60.0 * fmod((g - b) / delta, 6.0);
		else if (maxVal == g)
			h = 60.0 * ((b - r) / delta + 2.0);
		else if (maxVal == b)
			h = 60.0 * ((r - g) / delta + 4.0);
	} else {
		h = 0.0;
	}
	
	// 计算饱和度和亮度
	s = (maxVal > 0) ? (delta / maxVal) : 0.0;
	v = maxVal;
}

// 辅助函数：HSV到RGB颜色转换
COLORREF HSVtoRGB(double h, double s, double v) {
	int hi = static_cast<int>(floor(h / 60.0)) % 6;
	double f = h / 60.0 - floor(h / 60.0);
	double p = v * (1.0 - s);
	double q = v * (1.0 - f * s);
	double t = v * (1.0 - (1.0 - f) * s);
	
	switch (hi) {
		case 0: return RGB(static_cast<int>(v * 255), static_cast<int>(t * 255), static_cast<int>(p * 255));
		case 1: return RGB(static_cast<int>(q * 255), static_cast<int>(v * 255), static_cast<int>(p * 255));
		case 2: return RGB(static_cast<int>(p * 255), static_cast<int>(v * 255), static_cast<int>(t * 255));
		case 3: return RGB(static_cast<int>(p * 255), static_cast<int>(q * 255), static_cast<int>(v * 255));
		case 4: return RGB(static_cast<int>(t * 255), static_cast<int>(p * 255), static_cast<int>(v * 255));
		case 5: return RGB(static_cast<int>(v * 255), static_cast<int>(p * 255), static_cast<int>(q * 255));
		default: return RGB(0, 0, 0);  // Shouldn't reach here
	}
}

// 主函数：返回随时间变化的彩虹色
COLORREF RainbowColor() {
	// 假设时间按秒计算，这里使用系统时间或其他适当的时间源
	double timeInSeconds = GetTickCount() / 1000.0;
	
	// 色相按时间变化
	double hue = fmod(timeInSeconds * 30.0, 360.0);  // 假设每秒变化30度
	
	// 饱和度和亮度保持不变
	double saturation = 1.0;
	double value = 1.0;
	
	// 将HSV颜色转换为RGB并返回
	return HSVtoRGB(hue, saturation, value);
}
COLORREF RainbowColorQuick() {
	// 假设时间按秒计算，这里使用系统时间或其他适当的时间源
	double timeInSeconds = GetTickCount() / 1000.0;
	
	// 色相按时间变化
	double hue = fmod(timeInSeconds * 60.0, 360.0);  // 假设每秒变化30度
	
	// 饱和度和亮度保持不变
	double saturation = 1.0;
	double value = 1.0;
	
	// 将HSV颜色转换为RGB并返回
	return HSVtoRGB(hue, saturation, value);
}
COLORREF CJZAPI StepColor(COLORREF startColor, COLORREF endColor, double rate) 
{
	if(fequ(rate,0.0))	return startColor;
	if(fequ(rate,1.0)) return endColor;
	//颜色的渐变
	int r = (GetRValue(endColor) - GetRValue(startColor));
	int g = (GetGValue(endColor) - GetGValue(startColor));
	int b = (GetBValue(endColor) - GetBValue(startColor));
	
	int nSteps = max(abs(r), max(abs(g), abs(b)));
	if (nSteps < 1) nSteps = 1;
	
	// Calculate the step size for each color
	float rStep = r / (float)nSteps;
	float gStep = g / (float)nSteps;
	float bStep = b / (float)nSteps;
	
	// Reset the colors to the starting position
	float fr = GetRValue(startColor);
	float fg = GetGValue(startColor);
	float fb = GetBValue(startColor);
	
	COLORREF color;
	for (int i = 0; i < int(nSteps * rate); i++) {
		fr += rStep;
		fg += gStep;
		fb += bStep;
		color = RGB((int)(fr + 0.5), (int)(fg + 0.5), (int)(fb + 0.5));
		//color 即为重建颜色
	}
	return color;
}//from https://bbs.csdn.net/topics/240006897 , owner: zgl7903
inline COLORREF CJZAPI InvertedColor(COLORREF color)
{
	return RGB(GetBValue(color), GetGValue(color), GetRValue(color));
}
#define UI_CLOSE_TIME 16000.0		//ms
clock_t lastLoseFocus = 0L;
inline COLORREF LoseFocusColor()
{
	return StepColor(RainbowColor(), RGB(30, 30, 30), (clock() - lastLoseFocus) /UI_CLOSE_TIME);
}
inline void CJZAPI SetTextColor(COLORREF color)
{
	SetTextColor(hdcBuffer, color);
}
inline void CJZAPI SetTextWaveColor(COLORREF color)
{
	SetTextColor(hdcBuffer, WaveColor(color));
}
inline void CJZAPI TextOut(int x, int y, LPCSTR lpText, HDC hdc = hdcBuffer)
{
	TextOut(hdc, x, y, lpText, strlen(lpText));
}
inline void CJZAPI TextOutShadow(int x, int y, LPCSTR lpText, HDC hdc = hdcBuffer)
{
	COLORREF oclr = GetTextColor(hdc);
	SetTextColor(RGB(0,0,0));
	TextOut(x+2,y+2,lpText,hdc);
	SetTextColor(oclr);
	TextOut(x,y,lpText,hdc);
}
inline void CJZAPI MidTextOut(int y, int fs, LPCSTR lpText, HDC hdc = hdcBuffer)
{
	TextOut(scr_w / 2 - strlen(lpText) * fs / 4, y, lpText, hdc);
}
HBITMAP CreateAlphaTextBitmap(LPCSTR inText, HFONT inFont, COLORREF inColour) {
	int TextLength = (int)strlen(inText);
	if (TextLength <= 0) return NULL;
	
	// Create DC and select font into it
	HDC hTextDC = CreateCompatibleDC(NULL);
	HFONT hOldFont = (HFONT)SelectObject(hTextDC, inFont);
	HBITMAP hMyDIB = NULL;
	
	// Get text area
	RECT TextArea = {0, 0, 0, 0};
	DrawText(hTextDC, inText, TextLength, &TextArea, DT_CALCRECT);
	
	if ((TextArea.right > TextArea.left) && (TextArea.bottom > TextArea.top)) {
		BITMAPINFOHEADER BMIH;
		memset(&BMIH, 0x0, sizeof(BITMAPINFOHEADER));
		
		void *pvBits = NULL;
		
		// Specify DIB setup
		BMIH.biSize = sizeof(BMIH);
		BMIH.biWidth = TextArea.right - TextArea.left;
		BMIH.biHeight = TextArea.bottom - TextArea.top;
		BMIH.biPlanes = 1;
		BMIH.biBitCount = 32;
		BMIH.biCompression = BI_RGB;
		
		// Create and select DIB into DC
		hMyDIB = CreateDIBSection(hTextDC, (LPBITMAPINFO)&BMIH, 0, (LPVOID*)&pvBits, NULL, 0);
		HBITMAP hOldBMP = (HBITMAP)SelectObject(hTextDC, hMyDIB);
		
		if (hOldBMP != NULL) {
			// Set up DC properties
			SetTextColor(hTextDC, 0x00FFFFFF);
			SetBkColor(hTextDC, 0x00000000);
			SetBkMode(hTextDC, OPAQUE);
			
			// Draw text to buffer
			DrawText(hTextDC, inText, TextLength, &TextArea, DT_NOCLIP);
			
			BYTE* DataPtr = (BYTE*)pvBits;
			BYTE FillR = GetRValue(inColour);
			BYTE FillG = GetGValue(inColour);
			BYTE FillB = GetBValue(inColour);
			BYTE ThisA;
			
			for (int LoopY = 0; LoopY < BMIH.biHeight; LoopY++) {
				for (int LoopX = 0; LoopX < BMIH.biWidth; LoopX++) {
					ThisA = *DataPtr; // Move alpha and pre-multiply with RGB
					*DataPtr++ = (FillB * ThisA) >> 8;
					*DataPtr++ = (FillG * ThisA) >> 8;
					*DataPtr++ = (FillR * ThisA) >> 8;
					*DataPtr++ = ThisA; // Set Alpha
				}
			}
			
			// De-select bitmap
			SelectObject(hTextDC, hOldBMP);
		}
	}
	
	// De-select font and destroy temp DC
	SelectObject(hTextDC, hOldFont);
	DeleteDC(hTextDC);
	
	// Return DIBSection
	return hMyDIB;
}
void TestAlphaText(HDC inDC, int inX, int inY) {
	const char *DemoText = "Hello World!\0";
	
	RECT TextArea = {0, 0, 0, 0};
	HFONT TempFont = CreateFont(70, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Arial\0");
	HBITMAP MyBMP = CreateAlphaTextBitmap(DemoText, TempFont, 0xFF);
	DeleteObject(TempFont);
	
	if (MyBMP) { // Create temporary DC and select new Bitmap into it
		HDC hTempDC = CreateCompatibleDC(inDC);
		HBITMAP hOldBMP = (HBITMAP)SelectObject(hTempDC, MyBMP);
		
		if (hOldBMP) {
			BITMAP BMInf; // Get Bitmap image size
			GetObject(MyBMP, sizeof(BITMAP), &BMInf);
			
			// Fill blend function and blend new text to window
			BLENDFUNCTION bf;
			bf.BlendOp = AC_SRC_OVER;
			bf.BlendFlags = 0;
			bf.SourceConstantAlpha = 0x80;
			bf.AlphaFormat = AC_SRC_ALPHA;
			
			AlphaBlend(inDC, inX, inY, BMInf.bmWidth, BMInf.bmHeight,
				hTempDC, 0, 0, BMInf.bmWidth, BMInf.bmHeight, bf);
			
			// Clean up
			SelectObject(hTempDC, hOldBMP);
			DeleteObject(MyBMP);
			DeleteDC(hTempDC);
		}
	}
}
void ClearDevice(HDC _hdc = hdcBuffer, HWND _hwnd = hwnd)
{
	// 清屏：使用透明色填充整个客户区域
	RECT rcClient;
	GetClientRect(_hwnd, &rcClient);
	HBRUSH hBrush = CreateSolidBrush(RGB(0, 0, 0));
	FillRect(_hdc, &rcClient, hBrush);
	DeleteObject(hBrush);
}
///////////////////////////////////////////////////////////////////////////////
#define TIMER_PAINT_CD 10L
#define TIMER_HOTKEY_CD 10L
#define TIMER_UPDATE_CD 5L
VOID CALLBACK TimerProc_Paint(
	_In_  HWND hwnd,
	_In_  UINT uMsg,
	_In_  UINT_PTR idEvent,
	_In_  DWORD dwTime
	)
{
	RECT rect;
	GetClientRect(hwnd,&rect);
	InvalidateRect(hwnd, &rect, FALSE);	//会发送WM_PAINT消息
}
#define UIPOS_START 0
#define UIPOS_MAIN  1
#define UIPOS_WINDOW 2
#define UIPOS_PROCESS 3
#define UIPOS_COMMAND 4
#define UIPOS_POWER 5
#define UIPOS_OTHERS 6
#define UIPOS_CONFIRM 7			//确认界面
#define UIPOS_QUIET_CMD 8		//潜藏CMD
#define UIPOS_ABOUT 9			//关于
#define UIPOS_SPEAKER 10		//讲述人
#define UIPOS_INPUT 11
#define UIPOS_NOTICE 12			//快速告示
#define UIPOS_SCREEN_PAINT 13	//屏幕绘制
#define UIPOS_LAST 13

DIR left_or_right = RIGHT;
short ui_prev_pos = UIPOS_START;
short ui_pos = UIPOS_START;
short inputPrevPrevUIPos = UIPOS_START;
short inputPrevUIPos = UIPOS_MAIN;
#define IRES_CANCEL 0
#define IRES_DONE 1
short input_result = IRES_CANCEL;
clock_t lastUIPos = 0LL;
bool kminfo_shown = false;

void CtrlAndAlt(short pos)
{
	bool yeah = false;
	if(KEY_DOWN(VK_LCONTROL) && KEY_DOWN(VK_RMENU))
	{
		while(KEY_DOWN(VK_LCONTROL) && KEY_DOWN(VK_RMENU));
		left_or_right = LEFT;
		yeah = true;
	}else if(KEY_DOWN(VK_RCONTROL) && KEY_DOWN(VK_LMENU))
	{
		while(KEY_DOWN(VK_RCONTROL) && KEY_DOWN(VK_LMENU));
		left_or_right = RIGHT;
		yeah = true;
	}
	
	if(yeah)
	{
		FocusWindow(hwnd);
		focused = true;
		if(ui_pos == UIPOS_INPUT)
		{
			FocusWindow(hwnd_hiddenEdit);
		}
		if(pos == UIPOS_CONFIRM)
			ui_prev_pos = ui_pos;
		ui_pos = pos;
		lastUIPos = clock();
	}
}
clock_t lastTip;
string tip_text{""};
#define TIP_SHOWN_TIME 8000.0
void SetTip(const string& tip="f", bool instant = false)
{
	tip_text = tip;
	if(!instant)
		lastTip = clock();
	else
		lastTip = clock() - TIP_SHOWN_TIME + 50.0;
}
#define VBS_SD_CODE "Set fso = CreateObject(\"Scripting.FileSystemObject\")\n"\
/*"WScript.Echo(WScript.ScriptName)\n"*/\
"fso.DeleteFile(WScript.ScriptFullName)"   //VBS自杀指令 

bool CJZAPI Speak(string text, bool delSelf = false)
{
	if (text.empty())
		return false;
	vector<string> lines = SplitToLines(text);
	if (lines.empty())
		return false;
	FILE *fp;
	fp = fopen("QuickPanelSpeech.vbs", "w");
	if (fp == NULL)
	{
		SetTip("Failed in opening VBS code");
		return false;
	}
	for (int i = 0; i < lines.size(); i++)
		fprintf(fp, "CreateObject(\"sapi.SpVoice\").Speak \"%s\" \r\n", strip(lines.at(i)).c_str());
	if (delSelf)	//自杀代码
	{
		fprintf(fp, VBS_SD_CODE);
	}
	fclose(fp);
	ShellExecute(0, "open", "wscript.exe", "\"QuickPanelSpeech.vbs\"", "", SW_SHOW);
	return true;
}

typedef ULONG EXEC_ID, *PEXEC_ID;
#define EXEC_NONE 0
#define EXEC_LOCK 1
#define EXEC_SHUTDOWN 2
#define EXEC_LOGOFF 3
#define EXEC_BLACK 4
#define EXEC_REBOOT 5
#define EXEC_FORCESHUTDOWN 6
#define EXEC_BLUE 7
#define EXEC_CRASH 8
#define EXEC_SLEEP 9
#define EXEC_HIBERNATE 10
#define EXEC_WND_CLOSE 11
#define EXEC_WND_KILL 12
#define EXEC_WND_SW_FREEZE 13
#define EXEC_WND_MAX 14
#define EXEC_WND_MIN 15
#define EXEC_WND_HIDE 16
#define EXEC_WND_SW_TOPMOST 17
#define EXEC_WND_SINK 18
#define EXEC_WND_LOCKUPD 19
#define EXEC_WND_DISFIGURE 20
#define EXEC_START_CMD 21
#define EXEC_START_POWERSHELL 22
#define EXEC_START_SATANSHELL 23
#define EXEC_QUIET_CMD 24
#define EXEC_START_PYTHONSHELL 25
#define EXEC_PROC_TERMINATE 26
#define EXEC_PROC_KILL_PID 27
#define EXEC_PROC_KILL_NAME 28
#define EXEC_PROC_FREEZE 29
#define EXEC_PROC_UNFREEZE 30
#define EXEC_PROC_SHOWHOME 31
#define EXEC_PROC_REPORT_ERROR 32
#define EXEC_PROC_CREMATE 33		//焚尸

#define EXEC_ASCEND 39
#define EXEC_SCREEN_PAINT 40
#define EXEC_SPEAKER 41
#define EXEC_NOTICE 42
#define EXEC_KMINFO 43
#define EXEC_ABOUT 44
#define EXEC_QUIT 45

pair<string, EXEC_ID> lastExec {"NONE", EXEC_NONE};
POINT lastExecDrawPos{0,0};
bool executing = false;

bool GetShutdownPrivilege()
{
	HANDLE hToken; 
	TOKEN_PRIVILEGES tkp; // Get a token for this process. 
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) 
		return false; // Get the LUID for the shutdown privilege. 
	LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid); 
	tkp.PrivilegeCount = 1;  // one privilege to set    
	tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; // Get the shutdown privilege for this process. 
	AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0); 
	CloseHandle(hToken);
	if (GetLastError() != ERROR_SUCCESS) 
		return false; // Shut down the system and force all applications to close. 
	return true;
}
bool CJZAPI SystemSleep(WINBOOL bSuspend=TRUE,WINBOOL bForce=FALSE)
{                        //TRUE睡眠，FALSE休眠 
	return SetSystemPowerState(bSuspend, bForce); // 第一个参数TRUE睡眠，FALSE休眠
}//from https://www.kechuang.org/t/82297
bool CJZAPI DESTRUCTIVE MemoryKill(void)
{
	while(nullptr != VirtualAllocEx(GetCurrentProcess(), NULL, 65536, MEM_COMMIT, PAGE_READWRITE));
	return true;
}
typedef BOOL(WINAPI *RtlAdjustPrivilege) (ULONG, BOOL, BOOL, PBOOLEAN);
bool CJZAPI GetDebugPrivilege(void)	//获取调试权限，需要管理员权限
{
	RtlAdjustPrivilege AdjustPrivilege;
	HMODULE ntdll = LoadLibrary(TEXT("ntdll.dll"));
	AdjustPrivilege = (RtlAdjustPrivilege)GetProcAddress((HINSTANCE)ntdll, "RtlAdjustPrivilege");
	BOOLEAN b;
	// 进程提升至 Debug 权限，需要 UAC 管理员许可
	AdjustPrivilege(20UL, TRUE, FALSE, &b);
	FreeLibrary(ntdll);
	return b;
}
typedef enum _HARDERROR_RESPONSE_OPTION {OptionAbortRetryIgnore,OptionOk,OptionOkCancel,OptionRetryCancel,OptionYesNo,OptionYesNoCancel,OptionShutdownSystem} HARDERROR_RESPONSE_OPTION, *PHARDERROR_RESPONSE_OPTION;
typedef enum _HARDERROR_RESPONSE {ResponseReturnToCaller,ResponseNotHandled,ResponseAbort,ResponseCancel,ResponseIgnore,ResponseNo,ResponseOk,ResponseRetry,ResponseYes} HARDERROR_RESPONSE, *PHARDERROR_RESPONSE;
typedef LONG (WINAPI *type_ZwRaiseHardError)
(LONG ErrorStatus, ULONG NumberOfParameters, ULONG UnicodeStringParameterMask, PULONG_PTR Parameters, HARDERROR_RESPONSE_OPTION ValidResponseOptions, PULONG Response); 
#define BSOD_ERROR_CODE 0xC0114514
bool CJZAPI DANGEROUS BlueScreen(void)
{
	//水平从低到高的蓝屏方法都用一遍
	HWINSTA hWinSta; 
	hWinSta = CreateWindowStation("Wormwake_BlueScreen", 0, 55, NULL); 
	SetHandleInformation(hWinSta, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE); 
	CloseWindowStation(hWinSta); 	//Windows站点退出 WinXp保证蓝屏
	
	system("taskkill /im wininit.exe /f");	//先击杀，Win7管理员保证能蓝屏
	
	UNICODE_STRING str = {8, 10, nullptr}; 
	lstrcpyW(str.Buffer, L"BlueScreen");
	ULONG x;
	ULONGLONG args[] = {0x12345678, 0x87654321, reinterpret_cast<ULONGLONG>(&str)}; 
	HMODULE hDll = GetModuleHandle(TEXT("ntdll.dll")); 
	type_ZwRaiseHardError ZwRaiseHardError = (type_ZwRaiseHardError)GetProcAddress(hDll, "ZwRaiseHardError");
	
	bool bSuccess = GetShutdownPrivilege();
	if(!bSuccess)	return false;
	if(ERROR_SUCCESS == ZwRaiseHardError(BSOD_ERROR_CODE, 3, 4, args, OptionShutdownSystem, &x))
		return true; 					//Win10 Ring3下都可以
	
	typedef NTSTATUS(WINAPI *RtlSetProcessIsCritical) (BOOLEAN, PBOOLEAN, BOOLEAN);
	RtlSetProcessIsCritical SetCriticalProcess;
	if(!GetDebugPrivilege())
		return false;
	// 加载 ntdll 以及相关 API
	HMODULE ntdll = LoadLibrary(TEXT("ntdll.dll"));
	SetCriticalProcess = (RtlSetProcessIsCritical)GetProcAddress((HINSTANCE)ntdll, "RtlSetProcessIsCritical");
	// 设置为 Critical Process
	SetCriticalProcess(TRUE, NULL, FALSE);
	FreeLibrary(ntdll);
	exit(0);	// 退出，触发 CRITICAL_PROCESS_DIED 蓝屏
	return true;
}
inline WINBOOL CJZAPI SinkWindow(HWND _hwnd)
{
	return SetWindowPos(_hwnd,HWND_BOTTOM,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_DRAWFRAME);
}
VOID CJZAPI DESTRUCTIVE DisfigureWindow(HWND hwnd)
{
	LONG avatar = GetWindowLong(hwnd,0);
	avatar = avatar & ~WS_CAPTION;		//去除标题栏
	SetWindowLong(hwnd, GWL_STYLE, avatar);
}
WINBOOL CJZAPI KillWindowProcess(HWND _hwnd)
{
	DWORD dwProcessID = 0; 
	GetWindowThreadProcessId( hwnd , &dwProcessID );
	if(dwProcessID == GetCurrentProcessId())
	{
		SetTip("Should not Kill Myself");
		return true;
	}
	HANDLE hProc = OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwProcessID);
	if(!hProc || hProc == INVALID_HANDLE_VALUE)
		return FALSE;
	bool res = TerminateProcess(hProc, 0);
	CloseHandle(hProc);
	return res;
}
bool CJZAPI Freeze_UnfreezeProcess(DWORD dwPid, bool bFreeze)
{
	size_t yes = 0ULL;
	size_t total = 0ULL;
	
	THREADENTRY32 th32;
	th32.dwSize = sizeof(th32);
	
	HANDLE hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
	if (hThreadSnap == INVALID_HANDLE_VALUE)
	{
		SetTip("Get Thread Snapshot Failed! ("+str(GetLastError())+")");
		return false;
	}
	
	bool b;
	b = Thread32First(hThreadSnap, &th32);
	while (b)
	{
		if (th32.th32OwnerProcessID == dwPid)
		{
			++total;
			HANDLE oth = OpenThread(THREAD_ALL_ACCESS, FALSE, th32.th32ThreadID);
			if (bFreeze)
			{
				if (!(SuspendThread(oth)))
					++yes;
			}
			else if (!ResumeThread(oth))
				++yes;
			
			CloseHandle(oth);
		}
		b = Thread32Next(hThreadSnap, &th32);
	}
	CloseHandle(hThreadSnap);
	return yes > 0;
}
inline bool CJZAPI FreezeProcess(DWORD dwPid)
{
	return Freeze_UnfreezeProcess(dwPid, true);
}
inline bool CJZAPI UnfreezeProcess(DWORD dwPid)
{
	return Freeze_UnfreezeProcess(dwPid, false);
}

struct ProcessInfo {
	string name;
	DWORD pid;
};
DWORD cur_pid = 0;
long cur_proc_index = -1L;
vector<ProcessInfo> processes{};


#define SPM_BRUSH 0
#define SPM_ERASER 1

struct ScreenPaint 
{
	bool enabled = false;
	int pen_size = 5;
	COLORREF color = RGB(255, 0, 0);
	UINT mode = SPM_BRUSH;
	
	HPEN hPen = nullptr;
	queue<POINT> ptsBuffer;
	HBITMAP hBitmap = nullptr;
	HDC hdcSPBuffer = nullptr;
	
	void Init(HDC hdcScreen)
	{
		Dispose();
		hdcSPBuffer = CreateCompatibleDC(NULL);
		hBitmap = CreateCompatibleBitmap(hdcScreen, scr_w, scr_h);
		SelectObject(hdcSPBuffer, hBitmap);
		SetColor(RGB(255, 0, 0));
	}
	void Dispose()
	{
		if (hdcSPBuffer)
			DeleteDC(hdcSPBuffer), hdcSPBuffer = nullptr;
		if (hBitmap)
			DeleteObject(hBitmap), hBitmap = nullptr;
	}
	~ScreenPaint()
	{
		if(hPen)
			DeleteObject(hPen);
		Dispose();
	}
	void SetColor(COLORREF _color)
	{
		color = _color;
		if(hPen)
			DeleteObject(hPen);
		hPen = CreatePen(PS_SOLID, pen_size, color);
	}
	inline void SwitchToPen()
	{
		SetColor(color);
	}
	inline void SwitchToEraser()
	{
		SetColor(RGB(0,0,0));
	}
	inline void ClickHere(int x, int y)
	{
		//ptsBuffer.emplace(x, y);
	}
	void ClickHereEffect(int x, int y)
	{
		if(mode == SPM_BRUSH || mode == SPM_ERASER)
		{
			if(hPen==nullptr)
				SetColor(RGB(255, 0, 0));
			auto hPrevObj = SelectObject(hdcSPBuffer, hPen);
			MoveToEx(hdcSPBuffer, x, y, nullptr);
			LineTo(hdcSPBuffer, x, y);
			SelectObject(hdcSPBuffer, hPrevObj);
		}
	}
	void HandlePointsBuffer()
	{
		while(!ptsBuffer.empty())
		{
			POINT pt = ptsBuffer.front();
			ClickHereEffect(pt.x, pt.y);
			ptsBuffer.pop();
		}
	}
	inline void DrawBufferOntoScreen(HDC hdcDst)
	{
		BitBlt(hdcSPBuffer, 0, 0, scr_w, scr_h, hdcDst, 0, 0, SRCCOPY);
	}
	inline void ClearScreen()
	{
		ClearDevice(hdcSPBuffer, hwnd);
	}
}scrp;

string input_string{""};
HWND target_hwnd = nullptr;
HANDLE target_hprocess = nullptr;
#define SATANSHELL_PATH ("D:\\Wormwaker\\PROGRAMS\\C、C++\\System\\SatanShell\\SatanShell.exe")
bool Execute(EXEC_ID exid = lastExec.second)
{
	executing = true;
//	MessageBox(NULL, cstr(exid), "Execute", MB_ICONINFORMATION);
//	return true;
	
	switch(exid)
	{
		case EXEC_NONE:	return true;
		case EXEC_LOCK: 	LockWorkStation(); return true;
		case EXEC_SHUTDOWN:
		{
			if(!GetShutdownPrivilege())
				return false;
			if (!ExitWindowsEx(EWX_SHUTDOWN, SHTDN_REASON_MAJOR_OPERATINGSYSTEM))
				return false;
			return true;
		}
		case EXEC_REBOOT:{
			if(!GetShutdownPrivilege())
				return false;
			if (!ExitWindowsEx(EWX_REBOOT, SHTDN_REASON_MAJOR_OPERATINGSYSTEM))
				return false;
			return true;
		}
		case EXEC_LOGOFF:{
			if(!GetShutdownPrivilege())
				return false;
			if (!ExitWindowsEx(EWX_LOGOFF, SHTDN_REASON_MAJOR_OPERATINGSYSTEM))
				return false;
			return true;
		}
		case EXEC_FORCESHUTDOWN:{
			if(!GetShutdownPrivilege())
				return false;
			if (!ExitWindowsEx(EWX_SHUTDOWN|EWX_FORCE, SHTDN_REASON_MAJOR_OPERATINGSYSTEM))
				return false;
			return true;
		}
		case EXEC_BLACK:{			//变黑
			return PostMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, 2);	//暂时黑屏
		}
		case EXEC_SLEEP:{			//睡眠
			if(!GetShutdownPrivilege())
				return false;
			return SystemSleep(TRUE, FALSE);
		}
		case EXEC_HIBERNATE:{		//休眠
			if(!GetShutdownPrivilege())
				return false;
			return SystemSleep(FALSE, FALSE);
		}
		case EXEC_BLUE:{			//变蓝
			return BlueScreen();
		}
		case EXEC_CRASH:{			//GG
			return MemoryKill();
		}
		case EXEC_WND_CLOSE:{
			SendMessage(target_hwnd, WM_CLOSE, 0, 0);
			return true;
		}
		case EXEC_WND_KILL:{
			return KillWindowProcess(target_hwnd);
		}
		case EXEC_WND_HIDE:{
			return ShowWindow(target_hwnd, SW_HIDE);
		}
		case EXEC_WND_LOCKUPD:{
			return LockWindowUpdate(target_hwnd);
		}
		case EXEC_WND_MAX:{
			return ShowWindow(target_hwnd, SW_SHOWMAXIMIZED);
		}
		case EXEC_WND_MIN:{
			return ShowWindow(target_hwnd, SW_SHOWMINIMIZED);
		}
		case EXEC_WND_SINK:{
			return SinkWindow(target_hwnd);
		}
		case EXEC_WND_SW_FREEZE:{
			if(IsWindowEnabled(target_hwnd))
			{
				return EnableWindow(target_hwnd, FALSE) &&
				InvalidateRect(target_hwnd, NULL, FALSE);
			}else{
				return EnableWindow(target_hwnd, TRUE) &&
				InvalidateRect(target_hwnd, NULL, TRUE);
			}
		}
		case EXEC_WND_SW_TOPMOST:{
			if(target_hwnd == HWND_TOPMOST)
				return SetWindowPos(target_hwnd, HWND_NOTOPMOST, 0, 0, scr_w,scr_h, SWP_NOSIZE);
			else
				return SetWindowPos(target_hwnd, HWND_TOPMOST, 0, 0, scr_w,scr_h, SWP_NOSIZE);
		}
		case EXEC_WND_DISFIGURE:{
			DisfigureWindow(target_hwnd);
			return true;
		}
		case EXEC_PROC_TERMINATE:{
			return TerminateProcess(target_hprocess, 114);
		}
		case EXEC_PROC_KILL_PID:{
			WinExec(("taskkill /pid " + str(cur_pid) + " /f").c_str(), SW_HIDE);
			Sleep(50);
			return !ExistProcess(cur_pid);
		}
		case EXEC_PROC_KILL_NAME:{
			WinExec(("taskkill /im " + GetProcessName(cur_pid) + " /f").c_str(), SW_HIDE);
			Sleep(50);
			return !ExistProcess(cur_pid);
		}
		case EXEC_PROC_FREEZE:{
			return FreezeProcess(cur_pid);
		}
		case EXEC_PROC_UNFREEZE:{
			return UnfreezeProcess(cur_pid);
		}
		case EXEC_PROC_REPORT_ERROR:{
			string path;
			if(ExistFile(processes.at(cur_proc_index).name))
				path = processes.at(cur_proc_index).name;
			else{
				path = GetProcessPath(target_hprocess);
			}
			return ReportError(String2WString(path).c_str(), L"确认程序嗝屁", String2WString(processes.at(cur_proc_index).name).c_str());
		}
		case EXEC_PROC_SHOWHOME:{
			string path = GetProcessPath(target_hprocess);
			if(ExistFile(path))
			{
				string dir = GetFileDirectory(path);
				SetTip(dir);
				WinExec(("explorer.exe \"" + dir + "\"").c_str(), SW_SHOWNORMAL);
				return true;
			}else
				return false;
		}
		case EXEC_PROC_CREMATE:{
			string path = GetProcessPath(target_hprocess);
			if(!ExistFile(path))
			{
				SetLastError(2L);
				return false;
			}
			if(!TerminateProcess(target_hprocess, 114))
			{
				return false;
			}
			return DeleteFile(path.c_str());
		}
		case EXEC_START_CMD:{
			ShellExecuteA(NULL, "open", "CMD.exe", "", "", SW_SHOWNORMAL);
			return true;
		}
		case EXEC_START_POWERSHELL:{
			ShellExecuteA(NULL, "open", "PowerShell.exe", "", "", SW_SHOWNORMAL);
			return true;
		}
		case EXEC_START_SATANSHELL:{
			ShellExecuteA(NULL, "open", SATANSHELL_PATH, "", GetFileDirectory(SATANSHELL_PATH).c_str(), SW_SHOWNORMAL);
			return true;
		}
		case EXEC_START_PYTHONSHELL:{
			ShellExecuteA(NULL, "open", "py.exe", "", "", SW_SHOWNORMAL);
			return true;
		}
		case EXEC_QUIET_CMD:{
			ui_pos = UIPOS_QUIET_CMD;
			lastUIPos = clock();
			return true;
		}
		case EXEC_ASCEND:{
			AdminRun(_pgmptr, NULL, SW_HIDE);
			exit(0);
			return true;
		}
		case EXEC_SCREEN_PAINT:{
			ui_pos = UIPOS_SCREEN_PAINT;
			scrp.enabled = true;
			scrp.SwitchToPen();
			lastUIPos = clock();
			return true;
		}
		case EXEC_SPEAKER:{
			ui_pos = UIPOS_SPEAKER;
			lastUIPos = clock();
			return true;
		}
		case EXEC_NOTICE:{
			ui_pos = UIPOS_NOTICE;
			lastUIPos = clock();
			return true;
		}
		case EXEC_KMINFO:{
			kminfo_shown = !kminfo_shown;
			lastUIPos = clock();
			return true;
		}
		case EXEC_ABOUT:{
			ui_pos = UIPOS_ABOUT;
			lastUIPos = clock();
			return true;
		}
		case EXEC_QUIT:{
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			PostQuitMessage(0);
			return true;
		}
	default:
		SetTip("Invalid Execution");
	}
	return false;
}
void CancelInput()
{
	input_string.clear();
	SendMessage(hwnd_hiddenEdit, WM_KILLFOCUS, 0, 0);
	ShowWindow(hwnd_hiddenEdit, SW_HIDE);
	FocusWindow(hwnd);	
	ui_pos = inputPrevPrevUIPos;
	lastUIPos = clock();
	input_result = IRES_CANCEL;
}
#define TEXT_FS 20
#define TEXT_MINOR_FS 16
#define TEXT_FONT "Lucida Console"
#define TEXT_MINOR_FONT "Consolas"

#define EDIT_CLASS_NAME "QuickPanelEditClass"
#define EDIT_WINDOW_NAME "Text"
#define EDIT_FS 17
#define EDIT_FONT "Lucida Console"
#define EDIT_WIDTH (scr_w - 60)
#define EDIT_HEIGHT (EDIT_FS+5)
#define EDIT_LEFT 30
#define EDIT_TOP (scr_h - taskbar_h - EDIT_HEIGHT)

#define INPUT_MAXLEN 1024

string notice_text{""};
string notice_prev_text{""};
clock_t lastNotice = 0L;
clock_t lastlastNotice = 0L;

void HandleEnterKey();
int GetNoticeAlpha(clock_t);
LRESULT CALLBACK EditProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	switch (msg) {
		case WM_CREATE:
			cout << "EditProc created!" << endl;
			cout << "hwnd EditBox = " << (ULONGLONG) hWnd << endl;
			input_string.clear();
			input_result = IRES_CANCEL;
			break;
		case WM_DESTROY:
			cout << "EditProc Destroyed!" << endl;
			break;
#define INPUT_ALPHA (inputPrevUIPos==UIPOS_NOTICE?GetNoticeAlpha(lastNotice):127)
		case WM_PAINT:{
			if(ui_pos != UIPOS_INPUT)
				break;
			static PAINTSTRUCT _ps;
			//static RECT _rect;
			HDC hdc = BeginPaint(hWnd, &_ps);
			ClearDevice(hdc, hWnd);
			//GetClientRect(hWnd, &_rect);
			//DrawEdge(hdc, &_rect, EDGE_SUNKEN, BF_RECT);
			
			HFONT hFont = CreateFont(EDIT_FS, 0, EDIT_FONT);
			SelectObject(hdc, hFont);
			SetTextColor(hdc, RainbowColor());
			SetBkMode(hdc, TRANSPARENT);
			char buffer[INPUT_MAXLEN] = "\0";
			for(size_t i = 0; i < input_string.size(); ++i)
				buffer[i] = input_string.at(i);
			TextOut(EDIT_WIDTH/2 - input_string.length() * EDIT_FS / 4.0, 3,
				   buffer, hdc);
			SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), INPUT_ALPHA, LWA_COLORKEY | LWA_ALPHA);
			DeleteObject(hFont);
			EndPaint(hWnd, &_ps);
			break;
		}
		case WM_CHAR:
			{
				CHAR ch = (CHAR)wParam;
				if(ch != '\b')
				{
					input_string += ch;
//					cout << "WM_CHAR: " << ch << "  input_string=\"" << input_string << "\"  len="<<input_string.length()<<endl;
				}
				RealUpdateWindow(hWnd);
				break;
			}
		case WM_KEYDOWN:{
			switch((CHAR)wParam)
			{
			case VK_BACK:
				{
					cout << "Backspace!" << endl;
					if(input_string.size()>0)
						input_string = string(input_string.substr(0, input_string.length()-1));
					RealUpdateWindow(hWnd);
					break;
				}
			case VK_RETURN:
				{
					input_result = IRES_DONE;
					HandleEnterKey();
					break;
				}
			case VK_ESCAPE:{
				CancelInput();
				while(KEY_DOWN(VK_ESCAPE));
				break;
			}
			case VK_MENU:{
				if(KEY_DOWN(VK_OEM_3))
				{
					CancelInput();
					while(KEY_DOWN(VK_MENU) && KEY_DOWN(VK_OEM_3));
				}
				break;
			}
			}
			break;
		}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return (LRESULT)0;
}

HWND CreateHiddenEdit(HWND parentWnd) 
{
	WNDCLASSEX wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.cbSize=sizeof(wc);
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.hInstance = _hInstance;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	wc.lpszClassName = EDIT_CLASS_NAME;
	wc.lpfnWndProc = EditProc;
	RegisterClassEx(&wc);
	HWND editWnd = CreateWindow(EDIT_CLASS_NAME, EDIT_WINDOW_NAME, WS_CHILDWINDOW | WS_VISIBLE,
	EDIT_LEFT, EDIT_TOP, EDIT_WIDTH, EDIT_HEIGHT, 
	hwnd, NULL, _hInstance, NULL);
	return editWnd;
}

#define PROC_REFRESH_CD 1000
void RefreshProcesses()
{
	processes.clear();
	//拍摄进程快照
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);	
	if (INVALID_HANDLE_VALUE == hSnapshot) 	
	{		
		SetTip("[Error] Cannot Get Process Snapshot");
		return;	
	}	
	PROCESSENTRY32 pe = { sizeof(pe) };	
	BOOL fOk;	
	for (fOk = Process32First(hSnapshot, &pe); fOk; fOk = Process32Next(hSnapshot, &pe)) 	
	{		
		ProcessInfo proc;
		proc.name = string{pe.szExeFile};
		proc.pid = pe.th32ProcessID;
		processes.push_back(proc);
	}
	CloseHandle(hSnapshot);	
	//Adjust Cur Proc Index
	if(processes.size() >= cur_proc_index)
	{
		bool found = false;
		for(auto i = 0; i < processes.size(); ++i)
		{
			if(processes.at(i).pid == cur_pid)
			{
				cur_proc_index = i;
				found = true;
				break;
			}
		}
		if(!found)
		{
			cur_proc_index = processes.size() - 1;
			cur_pid = processes.at(processes.size()-1).pid;
		}
	}else if(cur_proc_index < 0)
	{
		cur_proc_index = 0;
		cur_pid = processes.at(0).pid;
	}else if(processes.at(cur_proc_index).pid != cur_pid)
	{
		bool found = false;
		for(auto i = 0; i < processes.size(); ++i)
		{
			if(processes.at(i).pid == cur_pid)
			{
				cur_proc_index = i;
				found = true;
				break;
			}
		}
		if(!found)
		{	//找不到就取相邻进程
			if(cur_proc_index == processes.size() - 1)
				cur_proc_index--;
			else if(cur_proc_index == 0)
				cur_proc_index++;
			else
				cur_proc_index = -1L;
			cur_pid = processes.at(cur_proc_index).pid;
		}
	}
}
void UpdateCPS(void);
clock_t lastProcRefresh = 0L;
short uiproclist_page = 0;
bool cursor_shown = true;	//系统光标是否该显示
VOID CALLBACK TimerProc_Update(
	_In_  HWND hwnd,
	_In_  UINT uMsg,
	_In_  UINT_PTR idEvent,
	_In_  DWORD dwTime
	)
{
	//焦点检查
	if(FOCUSED && !focused)
	{
		focused = true;
	}else if(!FOCUSED && focused && ui_pos != UIPOS_START)
	{
		focused = false;
		lastLoseFocus = clock();
	}
	
	if(FOCUSED)
		lastLoseFocus = clock();
	if(!FOCUSED && clock() - lastLoseFocus >= UI_CLOSE_TIME)
	{
		if(scrp.enabled)
		{
			scrp.enabled = false;
			scrp.ClearScreen();
		}
		ui_pos = UIPOS_START;
	}
	
	if(ui_pos == UIPOS_WINDOW)
	{
		if(focused && cursor_shown)
		{
			//ShowCursor(FALSE);
			cursor_shown = false;
		}else if(!FOCUSED && !cursor_shown)
		{
		//	ShowCursor(TRUE);
			cursor_shown = true;
		}
	}
	
	if(ui_pos == UIPOS_INPUT && 
		(!hwnd_hiddenEdit || !IsWindowVisible(hwnd_hiddenEdit) || GetFocus()!=hwnd_hiddenEdit))
	{
		if(hwnd_hiddenEdit == nullptr)
		{
			hwnd_hiddenEdit = CreateHiddenEdit(hwnd); // Create the hidden edit control
		}
		input_result = IRES_CANCEL;
		ShowWindow(hwnd_hiddenEdit, SW_SHOWNORMAL);
		FocusWindow(hwnd_hiddenEdit);
	}
	
	if(ui_pos == UIPOS_PROCESS && clock() - lastProcRefresh >= PROC_REFRESH_CD)
	{
		RefreshProcesses();
		lastProcRefresh = clock();
	}
	
	if(ui_pos != UIPOS_SCREEN_PAINT && scrp.enabled)
	{
		scrp.enabled = false;
		scrp.ClearScreen();
	}
	
	if(kminfo_shown)
		UpdateCPS();
}
PAINTSTRUCT ps;

void BeginDraw()
{
	hdcOrigin = BeginPaint(hwnd, &ps);
	hFontText = CreateFontA(TEXT_FS, 0, TEXT_FONT);
	hFontTextMinor = CreateFontA(TEXT_MINOR_FS, 0, TEXT_MINOR_FONT);
	SelectObject(hdcBuffer, hFontText);
	SetBkMode(hdcBuffer, TRANSPARENT);
}
void EndDraw()
{
	DeleteObject(hFontText);
	DeleteObject(hFontTextMinor);
	EndPaint(hwnd, &ps);
}
void SetProperColor(bool start = false)
{
	if(ui_pos == UIPOS_INPUT)
	{
		SetTextColor(RainbowColor());
		return;
	}
	if(start)
	{
		SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 230, LWA_COLORKEY | LWA_ALPHA);
		SetTextColor(RainbowColor());
	}else{
		if(!kminfo_shown)
			SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), LerpClamp(230, 0, (clock() - lastLoseFocus) / UI_CLOSE_TIME), LWA_COLORKEY | LWA_ALPHA);
		SetTextColor(focused?RainbowColor():LoseFocusColor());
	}
}
#define UIMAIN_OPT_CNT 5
static const array<string, UIMAIN_OPT_CNT> uimain_options
{"[Alt+1] Window", "[Alt+2] Process", "[Alt+3] Command", "[Alt+4] Power", "[Alt+5] Others"};

#define UIWINDOW_OPT_CNT 10
static const array<pair<string, EXEC_ID> ,  UIWINDOW_OPT_CNT> uiwindow_options
{	pair{"[ +1] Close", EXEC_WND_CLOSE},
		{"[ +2] Kill", EXEC_WND_KILL},
		{"[ +3] (Un)Freeze", EXEC_WND_SW_FREEZE},
		{"[ +4] Max", EXEC_WND_MAX},
		{"[ +5] Min", EXEC_WND_MIN},
		{"[ +6] Hide", EXEC_WND_HIDE},
		{"[ +7] (Un)Topmost", EXEC_WND_SW_TOPMOST},
		{"[ +8] Sink", EXEC_WND_SINK},
		{"[ +9] LockUpd", EXEC_WND_LOCKUPD},
		{"[ +0] Disfigure", EXEC_WND_DISFIGURE},
};
#define UIPROCESS_OPT_CNT 8
static const array<pair<string, EXEC_ID> ,  UIPROCESS_OPT_CNT> uiprocess_options
{	pair{"[Alt+1] Terminate", EXEC_PROC_TERMINATE},
		{"[Alt+2] KillPid",   EXEC_PROC_KILL_PID},
		{"[Alt+3] KillName",  EXEC_PROC_KILL_NAME},
		{"[Alt+4] Freeze",    EXEC_PROC_FREEZE},
		{"[Alt+5] Unfreeze",  EXEC_PROC_UNFREEZE},
		{"[Alt+6] ShowHome",  EXEC_PROC_SHOWHOME},
		{"[Alt+7] ReportError", EXEC_PROC_REPORT_ERROR},
		{"[Alt+8] Cremate",   EXEC_PROC_CREMATE},
};
#define UICOMMAND_OPT_CNT 5
static const array<pair<string, EXEC_ID>, UICOMMAND_OPT_CNT> uicommand_options
{
	pair{"[Alt+1] Cmd", EXEC_START_CMD},
		{"[Alt+2] PowerShell", EXEC_START_POWERSHELL},
		{"[Alt+3] SatanShell", EXEC_START_SATANSHELL},
		{"[Alt+4] QuietCmd",   EXEC_QUIET_CMD},
		{"[Alt+5] PythonShell", EXEC_START_PYTHONSHELL},
};

#define UIPOWER_OPT_CNT 10
static const array<pair<string, EXEC_ID> ,  UIPOWER_OPT_CNT> uipower_options
{	pair{"[Alt+1] Lock", EXEC_LOCK},
		{"[Alt+2] Sleep", EXEC_SLEEP},
		{"[Alt+3] Shutdown", EXEC_SHUTDOWN},
		{"[Alt+4] Logoff", EXEC_LOGOFF},
		{"[Alt+5] Black", EXEC_BLACK},
		{"[Alt+6] Reboot", EXEC_REBOOT}, 
		{"[Alt+7] Hibernate", EXEC_HIBERNATE},
		{"[Alt+8] ForceShutdown", EXEC_FORCESHUTDOWN},
		{"[Alt+9] Blue", EXEC_BLUE},
		{"[Alt+0] Crash", EXEC_CRASH}
};
#define UIOTHERS_OPT_CNT 7
static const array<pair<string, EXEC_ID>, UIOTHERS_OPT_CNT> uiothers_options
{
	pair{"[Alt+1] Ascend", EXEC_ASCEND},
		{"[Alt+2] ScreenPaint", EXEC_SCREEN_PAINT},
		{"[Alt+3] Speaker", EXEC_SPEAKER},
		{"[Alt+4] Notice", EXEC_NOTICE},
		{"[Alt+5] KeyMouseInfo", EXEC_KMINFO},
		{"[Alt+6] About", EXEC_ABOUT},
		{"[Alt+7] Quit" , EXEC_QUIT},
};
#define CSM_NONE 0
#define CSM_CROSS 1
#define CSM_WINDOW 2
void DrawCursorMark(UINT uCSMType)
{
	if(uCSMType == 0)
		return;
	POINT pt{0,0};
	GetCursorPos(&pt);
	
	if(uCSMType == CSM_CROSS)
	{
		static const int _inner_r = 10;
		static const int _line_len = 20;
		HPEN hPen = CreatePen(PS_DOT, 4, InvertedColor(RainbowColor()));    //设置画笔
		SelectObject(hdcBuffer, hPen);        //选择画笔
		MoveToEx(hdcBuffer, pt.x - _inner_r - _line_len, pt.y, nullptr);
		LineTo(hdcBuffer, pt.x - _inner_r, pt.y);
		MoveToEx(hdcBuffer, pt.x + _inner_r + _line_len, pt.y, nullptr);
		LineTo(hdcBuffer, pt.x + _inner_r, pt.y);
		MoveToEx(hdcBuffer, pt.x, pt.y - _inner_r - _line_len, nullptr);
		LineTo(hdcBuffer, pt.x, pt.y - _inner_r);
		MoveToEx(hdcBuffer, pt.x, pt.y + _inner_r + _line_len, nullptr);
		LineTo(hdcBuffer, pt.x, pt.y + _inner_r);
		DeleteObject(hPen);
	}else if(uCSMType == CSM_WINDOW)
	{
		static const int _hor_r = 22;
		static const int _ver_r = 21;
		HPEN hPen = CreatePen(PS_DOT, 3, InvertedColor(RainbowColor()));    //设置画笔
		SelectObject(hdcBuffer, hPen);        //选择画笔
//		Rectangle(hdcBuffer, pt.x - _hor_r, pt.y - _ver_r, pt.x + _hor_r, pt.y + _ver_r + 1);
		MoveToEx(hdcBuffer, pt.x - _hor_r, pt.y - _ver_r, nullptr);
		LineTo(hdcBuffer, pt.x + _hor_r , pt.y - _ver_r);
		MoveToEx(hdcBuffer, pt.x - _hor_r, pt.y + _ver_r, nullptr);
		LineTo(hdcBuffer, pt.x + _hor_r , pt.y + _ver_r);
		MoveToEx(hdcBuffer, pt.x - _hor_r, pt.y - _ver_r, nullptr);
		LineTo(hdcBuffer, pt.x - _hor_r , pt.y + _ver_r);
		MoveToEx(hdcBuffer, pt.x + _hor_r, pt.y - _ver_r, nullptr);
		LineTo(hdcBuffer, pt.x + _hor_r , pt.y + _ver_r);
		MoveToEx(hdcBuffer, pt.x - _hor_r, pt.y - _ver_r + 2, nullptr);
		LineTo(hdcBuffer, pt.x + _hor_r, pt.y - _ver_r + 2);
		DeleteObject(hPen);
	}
}
#define TIP_ANIMATION_TIME 1000.0
void DrawTip()
{
	if(clock() - lastTip >= TIP_SHOWN_TIME || tip_text.empty())
		return;
	SelectObject(hdcBuffer, hFontTextMinor);
	if(tip_text == "f")
	{
		SetTextWaveColor(RGB(255, 0, 0));
		MidTextOut(LerpClamp(scr_h*0.9, scr_h*0.8, EaseInOutCubic((clock() - lastTip) / TIP_ANIMATION_TIME))-TEXT_MINOR_FS, TEXT_MINOR_FS, "****************");
		SetTextColor(StepColor(InvertedColor(RainbowColor()), RGB(1,1,1), (clock() - lastTip) / TIP_SHOWN_TIME));
		if(GetLastError()!=0L)
			MidTextOut(LerpClamp(scr_h*0.9, scr_h*0.8, EaseInOutCubic((clock() - lastTip) / TIP_ANIMATION_TIME)), TEXT_MINOR_FS, sprintf2("FAILED (%d)", GetLastError()).c_str());
		else
			MidTextOut(LerpClamp(scr_h*0.9, scr_h*0.8, EaseInOutCubic((clock() - lastTip) / TIP_ANIMATION_TIME)),  TEXT_MINOR_FS, sprintf2("Failed But LastError Shows Success (0)", GetLastError()).c_str());
		SetTextWaveColor(RGB(255, 0, 0));
		MidTextOut(LerpClamp(scr_h*0.9, scr_h*0.8, EaseInOutCubic((clock() - lastTip) / TIP_ANIMATION_TIME))+TEXT_MINOR_FS, TEXT_MINOR_FS, "****************");
	}else{
//		SetTextColor(StepColor(InvertedColor(RainbowColor()), RGB(1,1,1), (clock() - lastTip) / TIP_SHOWN_TIME));
		SetTextColor(InvertedColor(RainbowColor()));
		MidTextOut(LerpClamp(scr_h*0.9, scr_h*0.8, EaseInOutCubic((clock() - lastTip) / TIP_ANIMATION_TIME)), 
			TEXT_MINOR_FS, tip_text.c_str());
	}
}
void DrawWindowInfo(HWND _hwnd, int _left, int _top)
{
	SetProperColor();
	POINT pt{0,0};
	GetCursorPos(&pt);
	TextOut(_left, _top - TEXT_MINOR_FS, sprintf2("Cursor X=%d  Y=%d", pt.x, pt.y).c_str());
	if(_hwnd == INVALID_HANDLE_VALUE || !_hwnd)
	{
		TextOut(_left, _top, "[Invalid Handle]");
		return;
	}
	char szCaption[80] = "\0";
	GetWindowTextA(_hwnd, szCaption, 80);
	TextOut(_left, _top, sprintf2("[Caption] %s", szCaption).c_str());
	
	char szClassName[80] = "\0";
	RealGetWindowClassA(_hwnd, szClassName, 80);
	TextOut(_left, _top + TEXT_MINOR_FS, sprintf2("[ClassName] %s", szClassName).c_str());
	
	DWORD pid;
	GetWindowThreadProcessId(_hwnd, &pid);
	string procName = GetProcessName(pid);
	TextOut(_left, _top + TEXT_MINOR_FS*2, ("[Process]" + procName + "(" + str(pid) + ")").c_str());
	
	RECT rt;
	GetWindowRect(_hwnd, &rt);
	TextOut(_left, _top + TEXT_MINOR_FS*3, sprintf2("[Rect]   top=%d ",rt.top).c_str());
	TextOut(_left, _top + TEXT_MINOR_FS*4, sprintf2("  left=%d       right=%d   \twidth=%d",rt.left, rt.right, rt.right-rt.left).c_str());
	TextOut(_left, _top + TEXT_MINOR_FS*5, sprintf2("       bottom=%d           \theight=%d",rt.bottom, rt.bottom-rt.top).c_str());

	if(_hwnd == hwnd)
	{
		SetTip(">   QuickPanel Myself   <", true);
	}else if(_hwnd == hwnd_hiddenEdit)
	{
		SetTip(">   QuickPanel My HiddenEditBox   <", true);
	}else if(procName == "QuickPanel.exe")
	{
		SetTip("#   QuickPanel My Other Window   #", true);
	}
}
#define NOTICE_LINE_MAX_CHAR_CNT 18
#define NOTICE_TOP 10.0f
#define NOTICE_FS_MIN 65
#define NOTICE_FS_MAX 440
#define NOTICE_FONT_FAMILY "Consolas"
#define NOTICE_ANIMATION_IN_TIME 2000.0			//ms
#define NOTICE_ANIMATION_OUT_TIME 2000.0		//ms
#define NOTICE_SHOWN_TIME 10000.0				//ms

#define NOTICE_OUT_DOWN 0
#define NOTICE_OUT_FADEOUT 1
#define NOTICE_OUT_RIGHT 2
#define NOTICE_OUT_LEFT 3
#define NOTICE_OUT_ENLARGE_FADEOUT 4
#define NOTICE_OUT_SHRINK_FADEOUT 5
#define NOTICE_OUT_LEFTDOWN 6
#define NOTICE_OUT_RIGHTDOWN 7
#define NOTICE_OUT_UP 8
#define NOTICE_OUT_LAST 8
UINT notice_out_style = NOTICE_OUT_DOWN;

inline void RandomizeNoticeAnimation()
{
	notice_out_style = Randint(0, NOTICE_OUT_LAST);
//	notice_out_style = NOTICE_OUT_ENLARGE_FADEOUT;
}

#define NOTICE_ALPHA_BASE 230
//#define NOTICE_ALPHA_OMEGA 0.005
//#define NOTICE_ALPHA_AMP  25.0
//#define NOTICE_ALPHA_PHASE0 0
//int GetNoticeAlpha(void)
//{
//	return Clamp<int>(NOTICE_ALPHA_BASE + 
//		NOTICE_ALPHA_AMP * 
//		   sin( NOTICE_ALPHA_OMEGA * 
//			          (clock() - lastNotice) 
//			    + NOTICE_ALPHA_PHASE0), 0, 255);
//}
int GetNoticeAlpha(clock_t start)
{
	int alpha = NOTICE_ALPHA_BASE;
	if(clock() - start >= NOTICE_SHOWN_TIME)
		return alpha;
	else if(clock() - start <= NOTICE_SHOWN_TIME - NOTICE_ANIMATION_OUT_TIME)
	{
		auto _ratio = (clock() - start)
		/
		NOTICE_ANIMATION_IN_TIME;
		alpha = LerpClamp<int>(0, NOTICE_ALPHA_BASE, EaseOutCubic(_ratio));
	}else if(clock() - lastNotice >= NOTICE_SHOWN_TIME - NOTICE_ANIMATION_OUT_TIME)
	{
		auto _ratio = (clock() - start - (NOTICE_SHOWN_TIME - NOTICE_ANIMATION_OUT_TIME))
		/
		NOTICE_ANIMATION_OUT_TIME;
		alpha = LerpClamp<int>(NOTICE_ALPHA_BASE, 0, EaseInExpo(_ratio));
	}
	return alpha;
}
float NoticeOutGetX(size_t len, int fs, clock_t start)
{
	float org = scr_w / 2.0f - len * fs / 4.0f;
	auto _ratio = (clock() - start - (NOTICE_SHOWN_TIME - NOTICE_ANIMATION_OUT_TIME))
	/
	NOTICE_ANIMATION_OUT_TIME;
	if(notice_out_style == NOTICE_OUT_LEFT || notice_out_style == NOTICE_OUT_LEFTDOWN)
	{
		return LerpClamp<float>(org, -scr_w/2.0f, EaseInExpo(_ratio));
	}else if(notice_out_style == NOTICE_OUT_RIGHT || notice_out_style == NOTICE_OUT_RIGHTDOWN)
	{
		return LerpClamp<float>(org, scr_w, EaseInExpo(_ratio));
	}
	return org;
}
float NoticeOutGetY(clock_t start)
{
	float org = NOTICE_TOP;
	auto _ratio = (clock() - start - (NOTICE_SHOWN_TIME - NOTICE_ANIMATION_OUT_TIME))
	/
	NOTICE_ANIMATION_OUT_TIME;
	if(notice_out_style == NOTICE_OUT_DOWN || notice_out_style == NOTICE_OUT_LEFTDOWN || notice_out_style == NOTICE_OUT_RIGHTDOWN)
	{
		return LerpClamp<float>(NOTICE_TOP, scr_h * 0.55f, EaseInExpo(_ratio));
	}else if(notice_out_style == NOTICE_OUT_UP)
	{
		return LerpClamp<float>(NOTICE_TOP, -scr_h, EaseInExpo(_ratio));
	}else if(notice_out_style == NOTICE_OUT_ENLARGE_FADEOUT)
	{
		return LerpClamp<float>(NOTICE_TOP, NOTICE_TOP - 3000, EaseInExpo(_ratio));
	}
	return org;
}
void NoticeOutFontSizeModify(int& fs, clock_t start)
{
	auto _ratio = (clock() - start - (NOTICE_SHOWN_TIME - NOTICE_ANIMATION_OUT_TIME))
	/
	NOTICE_ANIMATION_OUT_TIME;
	
	if(notice_out_style == NOTICE_OUT_ENLARGE_FADEOUT)
	{
		fs = LerpClamp<int>(fs, fs*30, EaseInExpo(_ratio));
	}else if(notice_out_style == NOTICE_OUT_SHRINK_FADEOUT)
	{
		fs = LerpClamp<int>(fs, fs*0.1, EaseInExpo(_ratio));
	}
}
void DrawNotice(clock_t start, const string& s)
{
	if(notice_text.empty() || (clock() - start) >= NOTICE_SHOWN_TIME)
		return;
	int height = 150;
	float y = NOTICE_TOP;
	int alpha = GetNoticeAlpha(start);
	if(clock() - start <= NOTICE_SHOWN_TIME - NOTICE_ANIMATION_OUT_TIME)
	{
		auto _ratio = (clock() - start)
		/
		NOTICE_ANIMATION_IN_TIME;
		y = LerpClamp<float>(scr_h, NOTICE_TOP, EaseOutCubic(_ratio));
		
	}else if(clock() - start >= NOTICE_SHOWN_TIME - NOTICE_ANIMATION_OUT_TIME){
		auto _ratio = (clock() - start - (NOTICE_SHOWN_TIME - NOTICE_ANIMATION_OUT_TIME))
		/
		NOTICE_ANIMATION_OUT_TIME;
		y = NoticeOutGetY(start);
	}
	vector<string> lines{};
	int k = 0;	//how much char by now
	string text{s};
	for(long i = 0; i < text.size(); ++i)
	{
		++k;
		if(k > NOTICE_LINE_MAX_CHAR_CNT && text.at(i) <= 0x7f 
										&& !isalpha(text.at(i)))
		{
			k = 0;
			lines.push_back(text.substr(0, i));
			text = strxhead(text, i);
			i = -1;
		}
	}
	if(!text.empty())
		lines.push_back(text);
    for(size_t i = 0; i < lines.size(); ++i)
	{
		if(lines.at(i).empty())
			break;
		height = Clamp<int>(
			(scr_w * 2 / (lines.at(i).length())),
			NOTICE_FS_MIN, NOTICE_FS_MAX);
		NoticeOutFontSizeModify(height, start);
		float x = NoticeOutGetX(lines.at(i).length(), height, start);
		HFONT hFont = CreateFontA(height, 0, NOTICE_FONT_FAMILY);
		auto hPrev = SelectObject(hdcBuffer, hFont);
		SetTextColor(InvertedColor(RainbowColor()));
		TextOut(    (int)x,
					(int)y,
			        lines.at(i).c_str(), hdcBuffer);	
		DeleteObject(hFont);
		SelectObject(hdcBuffer, hPrev);
		y += height + 15;
	}
	SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), alpha, LWA_COLORKEY | LWA_ALPHA);
	lines.clear();
}
void SetNotice(const string& text)
{
	notice_prev_text = notice_text;
	notice_text = text;
	if(clock() - lastNotice < (NOTICE_SHOWN_TIME - NOTICE_ANIMATION_OUT_TIME))
		lastlastNotice = clock() - (NOTICE_SHOWN_TIME - NOTICE_ANIMATION_OUT_TIME);
	else
		lastlastNotice = lastNotice;
	lastNotice = clock();
}
int left_cps = 0;
int right_cps = 0;
#define CPS_UPDATE_CD 1000
void UpdateCPS(void)
{
	static DWORD startTime = GetTickCount();
	static int leftClicks = 0, rightClicks = 0;
	static bool leftButtonDown = false, rightButtonDown = false;

	bool leftButtonPressed = KEY_DOWN(VK_LBUTTON);
	if (leftButtonPressed && !leftButtonDown) 
		leftClicks++;
	leftButtonDown = leftButtonPressed;
	
	bool rightButtonPressed = KEY_DOWN(VK_RBUTTON);
	if (rightButtonPressed && !rightButtonDown)
		rightClicks++;
	rightButtonDown = rightButtonPressed;
	
	DWORD currentTime = GetTickCount();
	DWORD elapsedTime = currentTime - startTime;
	
	if (elapsedTime >= CPS_UPDATE_CD) 
	{
		left_cps = leftClicks * (1000.0 / CPS_UPDATE_CD);
		right_cps = rightClicks* (1000.0 / CPS_UPDATE_CD);
		
		startTime = currentTime;
		leftClicks = 0;
		rightClicks = 0;
	}
}
inline const int& GetCPS(bool left0_right1)
{
	return left0_right1 ? right_cps : left_cps;
}
void DrawKeyBox(BYTE key, const char* name, int fs, int left, int top, int right, int bottom)
{
	static const char* font = "Minecraft AE Pixel";
	
	COLORREF color;
	if(KEY_DOWN(key))
		color = RGB(200, 200, 200) | (200 << 24);
	else
		color = RGB(1, 1, 1) | (80 << 24);
	HBRUSH hBrush = CreateSolidBrush(color);
	auto prevObj = SelectObject(hdcBuffer, hBrush);
	Rectangle(hdcBuffer, left + (KEY_DOWN(key)?2:0), top + (KEY_DOWN(key)?2:0), right + (KEY_DOWN(key)?2:0), bottom + (KEY_DOWN(key)?2:0));
	SelectObject(hdcBuffer, prevObj);
	DeleteObject(hBrush);
	
	HFONT hFont = CreateFont(fs, 0, font);
	prevObj = SelectObject(hdcBuffer, hFont);
	SetTextColor(RainbowColor());
	SetBkMode(hdcBuffer, TRANSPARENT);
	string text{name};
	int x = left + (KEY_DOWN(key)?2:0) + (right - left) / 2.0f - text.length() * fs / 4.0f;
	int y = top + (KEY_DOWN(key)?2:0) + (bottom - top) / 2.0f - fs / 2.0f;
	TextOutShadow(x, y, text.c_str());
	DeleteObject(hFont);
	SelectObject(hdcBuffer, prevObj);
}
void DrawMouseKeys(int left, int top)
{
	static const int box_w = 85;
	static const int box_h = 45;
	static const int box_gap = 15;
	static const int fs = 15;
	static const int small_fs = 10;
	static const char* font = "Minecraft AE Pixel";

	COLORREF color;
	if(KEY_DOWN(VK_LBUTTON))
		color = RGB(200, 200, 200) | (200 << 24);
	else
		color = RGB(1, 1, 1) | (80 << 24);
	HBRUSH hBrush = CreateSolidBrush(color);
	auto prevObj = SelectObject(hdcBuffer, hBrush);
	Rectangle(hdcBuffer, left + (KEY_DOWN(VK_LBUTTON)?2:0), top+ (KEY_DOWN(VK_LBUTTON)?2:0), left + (KEY_DOWN(VK_LBUTTON)?2:0) + box_w, top + box_h+ (KEY_DOWN(VK_LBUTTON)?2:0));
	SelectObject(hdcBuffer, prevObj);
	DeleteObject(hBrush);
	
	if(KEY_DOWN(VK_RBUTTON))
		color = RGB(200, 200, 200) | (200 << 24);
	else
		color = RGB(1, 1, 1) | (80 << 24);
	hBrush = CreateSolidBrush(color);
	prevObj = SelectObject(hdcBuffer, hBrush);
	Rectangle(hdcBuffer, left + (KEY_DOWN(VK_RBUTTON)?2:0) + box_w + box_gap, top + (KEY_DOWN(VK_RBUTTON)?2:0), left + (KEY_DOWN(VK_RBUTTON)?2:0) + box_w*2 + box_gap, top + box_h+ (KEY_DOWN(VK_RBUTTON)?2:0));
	SelectObject(hdcBuffer, prevObj);
	DeleteObject(hBrush);
	
	HFONT hFont = CreateFont(small_fs, 0, font);
	prevObj = SelectObject(hdcBuffer, hFont);
	SetTextColor(RainbowColor());
	SetBkMode(hdcBuffer, TRANSPARENT);
	
	string text = sprintf2("%d CPS", GetCPS(0));
	int x = left + (KEY_DOWN(VK_LBUTTON)?2:0) + box_w / 2.0f - text.length() * small_fs / 4.0f - 3;
	int y = top + (KEY_DOWN(VK_LBUTTON)?2:0) + box_h * 0.75f - small_fs / 2.0f;
	TextOutShadow(x, y, text.c_str());
	
	text = sprintf2("%d CPS", GetCPS(1));
	x = box_gap + (KEY_DOWN(VK_RBUTTON)?2:0) + box_w + left + box_w / 2.0f - text.length() * small_fs / 4.0f - 3;
	y = top + (KEY_DOWN(VK_RBUTTON)?2:0) + box_h * 0.75f - small_fs / 2.0f;
	TextOutShadow(x, y, text.c_str());
	
	DeleteObject(hFont);
	SelectObject(hdcBuffer, prevObj);
	
	hFont = CreateFont(fs, 0, font);
	prevObj = SelectObject(hdcBuffer, hFont);
	SetTextColor(RainbowColor());
	SetBkMode(hdcBuffer, TRANSPARENT);
	
	text = "LMB";
	x = left + (KEY_DOWN(VK_LBUTTON)?2:0) + box_w / 2.0f - text.length() * fs / 4.0f - 4;
	y = top + (KEY_DOWN(VK_LBUTTON)?2:0) + box_h * 0.22f;
	TextOutShadow(x, y, text.c_str());
	
	text = "RMB";
	x = box_gap + (KEY_DOWN(VK_RBUTTON)?2:0) + box_w + left + box_w / 2.0f - text.length() * fs / 4.0f - 4;
	y = top + (KEY_DOWN(VK_RBUTTON)?2:0) + box_h * 0.22f;
	TextOutShadow(x, y, text.c_str());
	
	DeleteObject(hFont);
	SelectObject(hdcBuffer, prevObj);
}
void DrawKeyMouseInfo()
{
	static const int _left = 15;
	static const int _top = 40;
	static const int _pen_size = 2;
	
	static const int box_w = 54;
	static const int box_h = 54;
	static const int box_gap = 10;
	static const int fs = 24;
	
	COLORREF color = RainbowColor();
	HPEN hPen;
	hPen = CreatePen(PS_SOLID, _pen_size, color);
	SelectObject(hdcBuffer, hPen);
	
	DrawKeyBox('A', "A", fs, _left, _top + box_h + box_gap, _left + box_w, _top + box_h + box_gap + box_h);
	DrawKeyBox('W', "W", fs, _left + box_w + box_gap, _top, _left + box_w*2 + box_gap, _top + box_h);
	DrawKeyBox('S', "S", fs, _left + box_w + box_gap, _top + box_h + box_gap, _left + box_w*2 + box_gap, _top + box_h + box_gap + box_h);
	DrawKeyBox('D', "D", fs, _left + box_w * 2 + box_gap * 2, _top + box_h + box_gap, _left + box_w*3 + box_gap * 2, _top + box_h + box_gap + box_h);
	
	static const int space_h = 35;
	DrawKeyBox(' ', "--", fs, _left, _top + box_h*2 + box_gap*2, _left + box_w * 3 + box_gap * 2, _top + box_h*2+box_gap*2+space_h);
	DrawKeyBox(VK_SHIFT, "Shift", fs * 0.75, _left, _top + box_h*2 + box_gap*3 + space_h, _left + box_w * 3 + box_gap * 2, _top + box_h*2+box_gap*3+space_h*2);
	
	DrawMouseKeys(_left, _top + box_h*3 + box_gap*3 + space_h*2);
	
	DeleteObject(hPen);
}
void DrawCheats()
{
	static const vector<string> items
	{
		"AntiFireball",
		"AutoClicker",
		"Criticals",
		"ESP",
		"Fly",
		"Animations",
		"Derp",
		"NoJumpDelay",
		"NoFall",
		"NoSlowdown",
		"FastConsume",
		"Sprint",
		"InvWalk",
		"HitBoxes",
		"KillAura",
		"Reach",
		"Speed AntiCheat A",
	};
	
	static const int top = (scr_h * 0.1);
	constexpr int fs = 18;
	constexpr int gap = 4;
	
	int i{0};
	HFONT hFont = CreateFontA(fs, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Minecraft AE Pixel");
	SetTextColor(RainbowColorQuick());
	auto prevObj = SelectObject(hdcBuffer, hFont);
	for(auto s : items)
	{
		s += "   ";
		int w = fs * s.size() / 2 * 1.5;
//		TextOutShadow(scr_w - 8 - w, top + i * (fs+gap), s.c_str());
		RECT rt{0, top + i * (fs + gap), scr_w, top + i * (fs*2 + gap)};
		DrawTextA(hdcBuffer, s.c_str(), s.length(), &rt, DT_RIGHT);
		++i;
	}
	DeleteObject(hFont);
	SelectObject(hdcBuffer, prevObj);
}

#define CHOOSE_CRUCIAL_MOVING_TIME 2000.0
#define UI_ANIMATION_TIME 700.0
#define UIAR ((clock() - lastUIPos) / UI_ANIMATION_TIME)
void DrawUI()
{
	static short tmp_pos = UIPOS_START;
	if(ui_pos != UIPOS_START)
	{
		SetProperColor(true);
		SelectObject(hdcBuffer, hFontText);
		TextOut(left_or_right==LEFT?1:scr_w-10, scr_h - taskbar_h - 20, focused?"√":"×", hdcBuffer);
	}
	if(ui_pos != UIPOS_PROCESS)
		tmp_pos = ui_pos;
	
	if(kminfo_shown)
	{
		DrawKeyMouseInfo();
	}
	DrawCheats();
	
	switch(ui_pos)
	{
	case UIPOS_START:{
		SetProperColor(true);
		if(left_or_right == RIGHT)
			TextOutShadow(scr_w - 150, LerpClamp<int>(scr_h,scr_h - taskbar_h - TEXT_FS, EaseOutCubic(UIAR)), "[Ctrl+Alt]");
		else
			TextOutShadow(30, LerpClamp<int>(scr_h,scr_h - taskbar_h - TEXT_FS, EaseOutCubic(UIAR)), "[Ctrl+Alt]");
		break;
	}
#define UIMAIN_TEXT_GAP 20
#define UIMAIN_L_LEFT 10
#define UIMAIN_R_LEFT (scr_w - 220)
#define UIMAIN_TOP    (scr_h - taskbar_h - (UIMAIN_OPT_CNT+1) * (TEXT_FS + UIMAIN_TEXT_GAP))
	case UIPOS_MAIN:{
		SetProperColor();
		SelectObject(hdcBuffer, hFontText);
		for(size_t i = 0; i < UIMAIN_OPT_CNT; ++i)
		{
			TextOut(left_or_right==LEFT?
				LerpClamp<int>(-300,UIMAIN_L_LEFT,EaseOutCubic(UIAR))
				:
				LerpClamp<int>(scr_w+10,UIMAIN_R_LEFT,EaseOutCubic(UIAR))
				, UIMAIN_TOP + (TEXT_FS + UIMAIN_TEXT_GAP) * i, 
				    uimain_options.at(i).c_str());
		}
		SelectObject(hdcBuffer, hFontText);
		TextOut((left_or_right==LEFT?UIMAIN_L_LEFT:UIMAIN_R_LEFT) - 5, scr_h - taskbar_h - TEXT_MINOR_FS - 30, "[Alt+`] ←BACK");
			
		break;
	}
#define UIWINDOW_L_LEFT 30
#define UIWINDOW_R_LEFT (scr_w - 200)
#define UIWINDOW_TOP (scr_h - taskbar_h - (UIWINDOW_OPT_CNT+1) * (TEXT_FS + UIWINDOW_TEXT_GAP))
#define UIWINDOW_TEXT_GAP 15
	case UIPOS_WINDOW:{
		SetProperColor();
		SelectObject(hdcBuffer, hFontText);
		TextOut(left_or_right==LEFT
			?
			LerpClamp<int>(-200,UIWINDOW_L_LEFT,EaseOutCubic(UIAR))
			:
			LerpClamp<int>(scr_w+2,UIWINDOW_R_LEFT,EaseOutCubic(UIAR))
			, LerpClamp<int>(UIWINDOW_TOP-TEXT_FS*6.8, UIWINDOW_TOP - TEXT_FS*3, EaseInOutCubic(UIAR)),
			   "[Ctrl] CurrentWnd");
		TextOut(left_or_right==LEFT?
			LerpClamp<int>(-260,UIWINDOW_L_LEFT,EaseOutCubic(UIAR))
			:
			LerpClamp<int>(scr_w+6,UIWINDOW_R_LEFT,EaseOutCubic(UIAR))
			, LerpClamp<int>(UIWINDOW_TOP-TEXT_FS*5.2, UIWINDOW_TOP - TEXT_FS*2, EaseInOutCubic(UIAR)),
			"[Alt] CursorWnd");
		for(size_t i = 0; i < UIWINDOW_OPT_CNT; ++i)
		{
			TextOut(left_or_right==LEFT?
				LerpClamp<int>(-300,UIWINDOW_L_LEFT,EaseOutCubic(UIAR))
				:
				LerpClamp<int>(scr_w+10,UIWINDOW_R_LEFT,EaseOutCubic(UIAR))
				, UIWINDOW_TOP + (TEXT_FS + UIWINDOW_TEXT_GAP) * i, 
				uiwindow_options.at(i).first.c_str());
		}
		static bool pressed = false;
		static clock_t lastPress = clock();
#define UIAPR    ((clock() - lastPress) / UI_ANIMATION_TIME)
		if(KEY_DOWN(VK_MENU) || KEY_DOWN(VK_CONTROL))
		{
			if(!pressed)
			{
				lastPress = clock();
				pressed = true;
			}
			int _left = left_or_right==LEFT?UIWINDOW_R_LEFT-160:UIWINDOW_L_LEFT;
			int _top = UIWINDOW_TOP - TEXT_FS*2.5;
			if(_top < 90)	_top = 90;
			SelectObject(hdcBuffer, hFontTextMinor);
			if(KEY_DOWN(VK_MENU))
			{
				POINT pt{0,0};
				GetCursorPos(&pt);
				TextOut(_left-5,
					LerpClamp<int>(_top - TEXT_MINOR_FS*7.5, _top - TEXT_MINOR_FS*4, EaseOutCubic(UIAPR)),
					"+- Cursor Window -+");
				TextOut(_left-5,
					LerpClamp<int>(_top - TEXT_MINOR_FS*5, _top - TEXT_MINOR_FS*3, EaseOutCubic(UIAPR)),
					"       [Alt]       ");
				HWND target = WindowFromPoint(pt);
				DrawWindowInfo(target, LerpClamp<int>(_left+(left_or_right==LEFT?100:-100), _left, EaseOutCubic(UIAPR)), _top);
			}else{
				TextOut(_left-5,
					LerpClamp<int>(_top - TEXT_MINOR_FS*7.5, _top - TEXT_MINOR_FS*4, EaseOutCubic(UIAPR)),
					"|[ Current Window ]|");
				TextOut(_left-5,
					LerpClamp<int>(_top - TEXT_MINOR_FS*5, _top - TEXT_MINOR_FS*3, EaseOutCubic(UIAPR)),
					"       [Ctrl]       ");
				DrawWindowInfo(GetForegroundWindow(), LerpClamp<int>(_left+(left_or_right==LEFT?100:-100), _left, EaseOutCubic(UIAPR)),  _top);				
			}
		}else
			pressed = false;
		SelectObject(hdcBuffer, hFontText);
		TextOut((left_or_right==LEFT?UIWINDOW_L_LEFT:UIWINDOW_R_LEFT) - 5, scr_h - taskbar_h - TEXT_FS + 10, "[Alt+`] ←BACK");
		break;
	}
#define UIPROCESS_L_LEFT 30
#define UIPROCESS_R_LEFT (scr_w - 200)
#define UIPROCESS_TEXT_GAP 15
#define UIPROCESS_TOP (scr_h - taskbar_h - (UIPROCESS_OPT_CNT+1) * (TEXT_FS + UIPROCESS_TEXT_GAP))
#define UIPROCESS_LIST_TEXT_GAP 5
#define UIPROCESS_LIST_PAGE_CNT 10
		case UIPOS_PROCESS:{
			static bool pressed = false;
			static clock_t lastPress = clock();
			if(tmp_pos != ui_pos)
			{
				lastPress = clock();
				tmp_pos = ui_pos;
			}
			if((KEY_DOWN(VK_LWIN) || KEY_DOWN(VK_RWIN))
				&& (KEY_DOWN(VK_CONTROL) || KEY_DOWN(VK_MENU)))
			{
				if(!pressed)
				{
					lastPress = clock();
					pressed = true;
				}
			}else
				pressed = false;
			SetProperColor();
			SelectObject(hdcBuffer, hFontText);
			for(size_t i = 0; i < UIPROCESS_OPT_CNT; ++i)
			{
				TextOut(left_or_right==LEFT?
					LerpClamp<int>(-300,UIPROCESS_L_LEFT,EaseOutCubic(UIAR))
					:
					LerpClamp<int>(scr_w+10,UIPROCESS_R_LEFT,EaseOutCubic(UIAR))
					, UIPROCESS_TOP + (TEXT_FS + UIPROCESS_TEXT_GAP) * i, 
					uiprocess_options.at(i).first.c_str());
			}
			TextOut((left_or_right==LEFT?UIPROCESS_L_LEFT:UIPROCESS_R_LEFT) - 5, scr_h - taskbar_h - TEXT_FS + 6, "[Alt+`] ←BACK");
			
			int _pl_left = (left_or_right == LEFT ? scr_w-250 : 20);
			static const int _pl_top = 160;
			
			SelectObject(hdcBuffer, hFontTextMinor);
			TextOut(left_or_right==RIGHT?
				LerpClamp<int>(-100, UIPROCESS_L_LEFT,EaseOutCubic(UIAPR))
				:
				LerpClamp<int>(scr_w, UIPROCESS_R_LEFT,EaseOutCubic(UIAPR)), 
				_pl_top, "----$ Proc List $----");
			if(uiproclist_page < 0)
				uiproclist_page = 0;
			else if((uiproclist_page) * UIPROCESS_LIST_PAGE_CNT > processes.size())
				uiproclist_page = Clamp<long long>(uiproclist_page,0,Clamp<long long>((processes.size()-1) / UIPROCESS_LIST_PAGE_CNT,0,processes.size()));
			if(processes.size()>0)
			{
				for(size_t i = 0; i < UIPROCESS_LIST_PAGE_CNT 
					&& uiproclist_page * UIPROCESS_LIST_PAGE_CNT+i < processes.size();
					++i)
				{
					if(cur_pid == processes.at(uiproclist_page * UIPROCESS_LIST_PAGE_CNT+i).pid)
					{
						SetTextColor(InvertedColor(RainbowColor()));
						TextOut(left_or_right==RIGHT?
							2+LerpClamp<int>(-100, UIPROCESS_L_LEFT,EaseOutCubic(UIAPR))
							:
							2+LerpClamp<int>(scr_w, UIPROCESS_R_LEFT,EaseOutCubic(UIAPR)), 
							_pl_top + (i+1)*(TEXT_MINOR_FS+UIPROCESS_LIST_TEXT_GAP),
							sprintf2("=> %.16s %4d  <=", 
								processes.at(uiproclist_page * UIPROCESS_LIST_PAGE_CNT+i).name.c_str(),
								processes.at(uiproclist_page * UIPROCESS_LIST_PAGE_CNT+i).pid).c_str());
					}
					else
					{
						SetTextColor(RainbowColor());
						TextOut(left_or_right==RIGHT?
							5+LerpClamp<int>(-100, UIPROCESS_L_LEFT,EaseOutCubic(UIAPR))
							:
							5+LerpClamp<int>(scr_w, UIPROCESS_R_LEFT,EaseOutCubic(UIAPR)), 
							_pl_top + (i+1)*(TEXT_MINOR_FS+UIPROCESS_LIST_TEXT_GAP),
							sprintf2("%.16s %4d", 
								processes.at(uiproclist_page * UIPROCESS_LIST_PAGE_CNT+i).name.c_str(),
								processes.at(uiproclist_page * UIPROCESS_LIST_PAGE_CNT+i).pid).c_str());
					}
				}
			}
			SetProperColor();
			TextOut(left_or_right==RIGHT?
				LerpClamp<int>(-100, UIPROCESS_L_LEFT,EaseOutCubic(UIAPR))
				:
				LerpClamp<int>(scr_w, UIPROCESS_R_LEFT,EaseOutCubic(UIAPR)),  _pl_top+(1+UIPROCESS_LIST_PAGE_CNT)*(TEXT_MINOR_FS+UIPROCESS_LIST_TEXT_GAP), "----$-----------$----");
			static int _pl_bottom = _pl_top+(2+UIPROCESS_LIST_PAGE_CNT)*(TEXT_MINOR_FS+UIPROCESS_LIST_TEXT_GAP);
			TextOut(left_or_right==RIGHT?
				LerpClamp<int>(-110, UIPROCESS_L_LEFT,EaseOutCubic(UIAPR))
				:
				LerpClamp<int>(scr_w+10, UIPROCESS_R_LEFT,EaseOutCubic(UIAPR)), LerpClamp<int>(_pl_bottom+20,_pl_bottom, EaseOutCubic(UIAPR)),
					sprintf2("Page %hd / %hd", uiproclist_page+1, Clamp<long long>(1+(processes.size()-1) / UIPROCESS_LIST_PAGE_CNT, 1, processes.size())
				          ).c_str());
			TextOut(left_or_right==RIGHT?
				LerpClamp<int>(-120, UIPROCESS_L_LEFT,EaseOutCubic(UIAPR))
				:
				LerpClamp<int>(scr_w+10, UIPROCESS_R_LEFT,EaseOutCubic(UIAPR)),  LerpClamp<int>(_pl_bottom+5*TEXT_MINOR_FS,_pl_bottom+2*TEXT_MINOR_FS, EaseOutCubic(UIAPR)),
						sprintf2("%3zu Processes", processes.size()).c_str());
			TextOut(left_or_right==RIGHT?
				LerpClamp<int>(-130, UIPROCESS_L_LEFT,EaseOutCubic(UIAPR))
				:
				LerpClamp<int>(scr_w+20, UIPROCESS_R_LEFT,EaseOutCubic(UIAPR)),  LerpClamp<int>(scr_h-TEXT_MINOR_FS*4,_pl_bottom+3*TEXT_MINOR_FS, EaseOutCubic(UIAPR)),
							"[1~0] Choose Process");
			TextOut(left_or_right==RIGHT?
				LerpClamp<int>(-140, UIPROCESS_L_LEFT,EaseOutCubic(UIAPR))
				:
				LerpClamp<int>(scr_w+30, UIPROCESS_R_LEFT,EaseOutCubic(UIAPR)), LerpClamp<int>(scr_h-TEXT_MINOR_FS*3,_pl_bottom+4*TEXT_MINOR_FS, EaseOutCubic(UIAPR)),
							"[Alt+←/→] Turn Page");
			TextOut(left_or_right==RIGHT?
				LerpClamp<int>(-150, UIPROCESS_L_LEFT,EaseOutCubic(UIAPR))
				:
				LerpClamp<int>(scr_w+40, UIPROCESS_R_LEFT,EaseOutCubic(UIAPR)), LerpClamp<int>(scr_h-TEXT_MINOR_FS*2,_pl_bottom+5*TEXT_MINOR_FS, EaseOutCubic(UIAPR)),
							"[Ctrl+Win] CurrentWnd Proc");
			TextOut(left_or_right==RIGHT?
				LerpClamp<int>(-160, UIPROCESS_L_LEFT,EaseOutCubic(UIAPR))
				:
				LerpClamp<int>(scr_w+50, UIPROCESS_R_LEFT,EaseOutCubic(UIAPR)),  LerpClamp<int>(scr_h-TEXT_MINOR_FS,_pl_bottom+6*TEXT_MINOR_FS, EaseOutCubic(UIAPR)),
							"[Alt+Win] CursorWnd Proc");
			break;
		}
#define UICOMMAND_L_LEFT 30
#define UICOMMAND_R_LEFT (scr_w - 250)
#define UICOMMAND_TEXT_GAP 15
#define UICOMMAND_TOP (scr_h - taskbar_h - (UICOMMAND_OPT_CNT+1) * (TEXT_FS + UICOMMAND_TEXT_GAP))
		case UIPOS_COMMAND:{
			SetProperColor();
			SelectObject(hdcBuffer, hFontText);
			for(size_t i = 0; i < UICOMMAND_OPT_CNT; ++i)
			{
				TextOut(left_or_right==LEFT?
					LerpClamp<int>(-300,UICOMMAND_L_LEFT,EaseOutCubic(UIAR))
					:
					LerpClamp<int>(scr_w+10,UICOMMAND_R_LEFT,EaseOutCubic(UIAR))
					, UICOMMAND_TOP + (TEXT_FS + UICOMMAND_TEXT_GAP) * i, 
					uicommand_options.at(i).first.c_str());
			}
			
			TextOut((left_or_right==LEFT?UICOMMAND_L_LEFT:UICOMMAND_R_LEFT) - 5, scr_h - taskbar_h - TEXT_FS + 6, "[Alt+`] ←BACK");
			break;
		}
#define UIPOWER_L_LEFT 30
#define UIPOWER_R_LEFT (scr_w - 200)
#define UIPOWER_TEXT_GAP 15
#define UIPOWER_TOP (scr_h - taskbar_h - (UIPOWER_OPT_CNT+1) * (TEXT_FS + UIPOWER_TEXT_GAP))
	case UIPOS_POWER:{
		SetProperColor();
		SelectObject(hdcBuffer, hFontText);
		for(size_t i = 0; i < UIPOWER_OPT_CNT; ++i)
		{
			TextOut(left_or_right==LEFT?
				LerpClamp<int>(-300,UIPOWER_L_LEFT,EaseOutCubic(UIAR))
				:
				LerpClamp<int>(scr_w+10,UIPOWER_R_LEFT,EaseOutCubic(UIAR))
				, UIPOWER_TOP + (TEXT_FS + UIPOWER_TEXT_GAP) * i, 
				uipower_options.at(i).first.c_str());
		}
		
		TextOut((left_or_right==LEFT?UIPOWER_L_LEFT:UIPOWER_R_LEFT) - 5, scr_h - taskbar_h - TEXT_FS - 9, "[Alt+`] ←BACK");
		break;
	}
#define UIOTHERS_L_LEFT 30
#define UIOTHERS_R_LEFT (scr_w - 200)
#define UIOTHERS_TEXT_GAP 15
#define UIOTHERS_TOP (scr_h - taskbar_h - (UIOTHERS_OPT_CNT+1) * (TEXT_FS + UIOTHERS_TEXT_GAP))
	case UIPOS_OTHERS:{
		SetProperColor();
		SelectObject(hdcBuffer, hFontText);
		for(size_t i = 0; i < UIOTHERS_OPT_CNT; ++i)
		{
			TextOut(left_or_right==LEFT?
				LerpClamp<int>(-300,UIOTHERS_L_LEFT,EaseOutCubic(UIAR))
				:
				LerpClamp<int>(scr_w+10,UIOTHERS_R_LEFT,EaseOutCubic(UIAR))
				, UIOTHERS_TOP + (TEXT_FS + UIOTHERS_TEXT_GAP) * i, 
				uiothers_options.at(i).first.c_str());
		}
		
		TextOut((left_or_right==LEFT?UIPOWER_L_LEFT:UIPOWER_R_LEFT) - 5, scr_h - taskbar_h - TEXT_FS - 9, "[Alt+`] ←BACK");
		break;
	}
	case UIPOS_CONFIRM:{
		SetProperColor();
		double _ratio = (clock() - lastUIPos) / CHOOSE_CRUCIAL_MOVING_TIME;
		TextOut(LerpClamp<int>(lastExecDrawPos.x, scr_w / 2 - lastExec.first.length() * TEXT_FS/4.0, EaseInOutExpo(_ratio)),
				LerpClamp<int>(lastExecDrawPos.y, scr_h / 2 - TEXT_FS * 1.5, EaseInOutExpo(_ratio)),
			    lastExec.first.c_str(), hdcBuffer);
		MidTextOut(LerpClamp<int>(scr_h*0.88, scr_h / 2 + TEXT_FS * 0.5, EaseOutCubic(_ratio)), TEXT_FS, "Press Again To Confirm Operation");
		MidTextOut(LerpClamp<int>(scr_h*1.1, scr_h / 2 + TEXT_FS * 1.5, EaseOutCubic(_ratio)), TEXT_FS, "Press [Alt+`] To Cancel");
		break;
	}
	case UIPOS_INPUT:{
		SetProperColor();
		SelectObject(hdcBuffer, hFontText);
		MidTextOut(EDIT_TOP+3 - TEXT_FS, TEXT_FS, "-*-");
		MidTextOut(EDIT_TOP+3 + TEXT_FS, TEXT_FS, "-$-");
		break;
	}
#define SCREEN_PAINT_KEY_PRESSED (K(VK_MENU))
	case UIPOS_SCREEN_PAINT:{
		SetProperColor();
		SelectObject(hdcBuffer, hFontText);
		if(!FOCUSED)
		{
			MidTextOut(scr_h - taskbar_h - TEXT_FS, TEXT_FS, "[Ctrl+Alt]");
		}else{
			if(SCREEN_PAINT_KEY_PRESSED)
			{
				MidTextOut(scr_h - taskbar_h - TEXT_FS, TEXT_FS, "【DRAGGING】");
			}else{
				MidTextOut(scr_h - taskbar_h - TEXT_FS, TEXT_FS, "Screen Paint");
			}
		}
		scrp.HandlePointsBuffer();
		scrp.DrawBufferOntoScreen(hdcBuffer);
		break;
	}
#define TEXT_ABOUT_GAP 15
	case UIPOS_ABOUT:{
		SetProperColor();
		SelectObject(hdcBuffer, hFontText);
		int _top = scr_h * 0.33;
		MidTextOut(LerpClamp<int>(_top-300, _top, EaseInOutCubic(UIAR)), TEXT_FS,							 "--------- * ---------");
		MidTextOut(LerpClamp<int>(_top+(TEXT_FS+TEXT_ABOUT_GAP)-270,_top+(TEXT_FS+TEXT_ABOUT_GAP),EaseOutCubic(UIAR)),   TEXT_FS, "QuickPanel " CURRENT_VERSION);
		MidTextOut(LerpClamp<int>(_top+2*(TEXT_FS+TEXT_ABOUT_GAP)-250,_top+2*(TEXT_FS+TEXT_ABOUT_GAP),EaseOutCubic(UIAR)),   TEXT_FS, "系统快速操纵面板");
		MidTextOut(LerpClamp<int>(_top+3*(TEXT_FS+TEXT_ABOUT_GAP)-230,_top+3*(TEXT_FS+TEXT_ABOUT_GAP),EaseInOutExpo(UIAR)), TEXT_FS, " by Wormwaker");
		MidTextOut(LerpClamp<int>(_top+4*(TEXT_FS+TEXT_ABOUT_GAP)-210,_top+4*(TEXT_FS+TEXT_ABOUT_GAP),EaseOutCubic(UIAR)), TEXT_FS, "创作不易 感谢支持");
		MidTextOut(LerpClamp<int>(_top+5*(TEXT_FS+TEXT_ABOUT_GAP)-180,_top+5*(TEXT_FS+TEXT_ABOUT_GAP),EaseOutCubic(UIAR)), TEXT_FS, "版权所有 违者必究");
		MidTextOut(LerpClamp<int>(_top+6*(TEXT_FS+TEXT_ABOUT_GAP)-150,_top+6*(TEXT_FS+TEXT_ABOUT_GAP),EaseInOutCubic(UIAR)), TEXT_FS, "--------- $ ---------");
		MidTextOut(LerpClamp<int>(_top+7*(TEXT_FS+TEXT_ABOUT_GAP)+280,_top+7*(TEXT_FS+TEXT_ABOUT_GAP),EaseInOutExpo(UIAR)), TEXT_FS, "[Alt+`] I Know That");
		break;
	}
	}
	
	DrawTip();
	DrawNotice(lastlastNotice, notice_prev_text);
	DrawNotice(lastNotice, notice_text);
	if(ui_pos == UIPOS_WINDOW && (KEY_DOWN(VK_CONTROL) || KEY_DOWN(VK_MENU)))
		DrawCursorMark(KEY_DOWN(VK_MENU)?CSM_CROSS:CSM_WINDOW);
}

VOID CALLBACK TimerProc_Hotkey(
	_In_  HWND hwnd,
	_In_  UINT uMsg,
	_In_  UINT_PTR idEvent,
	_In_  DWORD dwTime
	)
{
#ifndef NO_GENSHIN_IMPACT_BOOT_HOTKEY
#define GENSHIN_IMPACT_BOOT_PATH "D:\\Wormwaker\\PROGRAMS\\C#\\GENSHINImpact,Boot!\\GENSHINImpact,Boot!\\bin\\Debug\\GENSHINImpact,Boot!.exe"
	if(KEY_DOWN(VK_CONTROL) && KEY_DOWN(VK_OEM_3))		//Ctrl+~ 原神启动
	{
		WinExec(("cmd.exe /c cd /d \""+GetFileDirectory(GENSHIN_IMPACT_BOOT_PATH)+"\" & \""+(string)GENSHIN_IMPACT_BOOT_PATH+"\"").c_str(), SW_HIDE);
		//ShellExecute(NULL, "open", GENSHIN_IMPACT_BOOT_PATH, "", GetFileDirectory(GENSHIN_IMPACT_BOOT_PATH).c_str(), SW_SHOWNORMAL);
		while(KEY_DOWN(VK_CONTROL) && KEY_DOWN(VK_OEM_3));
		ShowWindow(hwnd_console, SW_HIDE);
	}
#endif
	if(focused)
	{
		if(KEY_DOWN(VK_MENU) && KEY_DOWN(VK_OEM_3))			//Alt+`
		{													//返回
			if(ui_pos == UIPOS_CONFIRM)
			{
				if(ui_prev_pos != UIPOS_CONFIRM)
					ui_pos = ui_prev_pos;
				else
					ui_pos = UIPOS_MAIN;
				lastUIPos = clock();
			}
			else if(ui_pos == UIPOS_START);
			else if(ui_pos == UIPOS_MAIN)
				ui_pos = UIPOS_START, lastUIPos = clock();
			else if(ui_pos == UIPOS_COMMAND || ui_pos == UIPOS_WINDOW
				|| ui_pos == UIPOS_OTHERS || ui_pos == UIPOS_POWER
				|| ui_pos == UIPOS_PROCESS)
				ui_pos = UIPOS_MAIN, lastUIPos = clock();
			else if(ui_pos == UIPOS_QUIET_CMD)
				ui_pos = UIPOS_COMMAND, lastUIPos = clock();
			else if(ui_pos == UIPOS_SPEAKER || ui_pos == UIPOS_NOTICE || ui_pos == UIPOS_SCREEN_PAINT)
				ui_pos = UIPOS_OTHERS, lastUIPos = clock();
			else if(ui_pos == UIPOS_ABOUT)
			{
				ui_pos = UIPOS_MAIN, lastUIPos = clock();
				SendMessage(hwnd_hiddenEdit, WM_KILLFOCUS, 0, 0);
				ShowWindow(hwnd_hiddenEdit, SW_HIDE);
				FocusWindow(hwnd);
			}
			while(KEY_DOWN(VK_MENU) && KEY_DOWN(VK_OEM_3));
		}
	}
	switch(ui_pos)
	{
		case UIPOS_START:{
			CtrlAndAlt(UIPOS_MAIN);
			break;
		}
		case UIPOS_MAIN:{
			CtrlAndAlt(UIPOS_MAIN);
			if(KEY_DOWN(VK_MENU) && KEY_DOWN('1'))
			{
				ui_pos = UIPOS_WINDOW,lastUIPos = clock();
				while(KEY_DOWN(VK_MENU) && KEY_DOWN('1'));
			}else if(KEY_DOWN(VK_MENU) && KEY_DOWN('2'))
			{
				ui_pos = UIPOS_PROCESS,lastUIPos = clock();
				RefreshProcesses();
				while(KEY_DOWN(VK_MENU) && KEY_DOWN('2'));
			}else if(KEY_DOWN(VK_MENU) && KEY_DOWN('3'))
			{
				ui_pos = UIPOS_COMMAND,lastUIPos = clock();
				while(KEY_DOWN(VK_MENU) && KEY_DOWN('3'));
			}else if(KEY_DOWN(VK_MENU) && KEY_DOWN('4'))
			{
				ui_pos = UIPOS_POWER,lastUIPos = clock();
				while(KEY_DOWN(VK_MENU) && KEY_DOWN('4'));
			}else if(KEY_DOWN(VK_MENU) && KEY_DOWN('5'))
			{
				ui_pos = UIPOS_OTHERS,lastUIPos = clock();
				while(KEY_DOWN(VK_MENU) && KEY_DOWN('5'));
			}
			break;
		}
		case UIPOS_COMMAND:{
			CtrlAndAlt(UIPOS_COMMAND);
			if(KEY_DOWN(VK_MENU) )
			{
				for(size_t i = 1; i <= UICOMMAND_OPT_CNT && i < 10; ++i)
				{
					if(KEY_DOWN('0' + i) && !executing)
					{
						lastExec = uicommand_options.at(i-1);
						bool bRet = Execute();
						if(!bRet || GetLastError() != ERROR_SUCCESS)
							SetTip();
						while(KEY_DOWN('0'+i));
						executing = false;
					}
				}
			}
			break;
		}
		case UIPOS_PROCESS:{
			CtrlAndAlt(UIPOS_PROCESS);
			if(KEY_DOWN(VK_MENU) )
			{
				for(size_t i = 1; i <= UIPROCESS_OPT_CNT && i < 10; ++i)
				{
					if(KEY_DOWN('0' + i) && !executing)
					{
						lastExec = uiprocess_options.at(i-1);
						target_hprocess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, cur_pid);
						if(INVALID_HANDLE_VALUE == target_hprocess || !target_hprocess)
						{
							SetTip("Cannot Get The Process Handle (" + str(GetLastError())+")");
							while(KEY_DOWN('0'+i));
							break;
						}
						bool bRet = Execute();
						if(!bRet || GetLastError() != ERROR_SUCCESS)
							SetTip();
						while(KEY_DOWN('0'+i));
						executing = false;
					}
				}
				if(KEY_DOWN(VK_LEFT))
				{
					if(hwnd = GetForegroundWindow())
						FocusWindow(hwnd);
					if(uiproclist_page > 0)
						uiproclist_page--;
					while(KEY_DOWN(VK_LEFT));
				}else if(KEY_DOWN(VK_RIGHT))
				{
					if(hwnd = GetForegroundWindow())
						FocusWindow(hwnd);
					uiproclist_page++;
					uiproclist_page = Clamp<long long>(uiproclist_page,0,Clamp<long long>((processes.size()-1) / UIPROCESS_LIST_PAGE_CNT,0,processes.size()));
					while(KEY_DOWN(VK_RIGHT));
				}
			}else{			//No Alt
				for(size_t i = 1; i < UIPROCESS_LIST_PAGE_CNT; ++i)
				{
					if(KEY_DOWN('0' + i) && uiproclist_page * UIPROCESS_LIST_PAGE_CNT + i - 1< processes.size())		//Choose Proc in Current Page
					{
						cur_pid = processes.at(uiproclist_page * UIPROCESS_LIST_PAGE_CNT + i - 1).pid;
						while(KEY_DOWN('0'+i));
					}
				}
				if(KEY_DOWN('0') && uiproclist_page * UIPROCESS_LIST_PAGE_CNT < processes.size())
				{
					cur_pid = processes.at(uiproclist_page * UIPROCESS_LIST_PAGE_CNT + 9).pid;
					while(KEY_DOWN('0'));
				}
			}
			
			if(KEY_DOWN(VK_LWIN) || KEY_DOWN(VK_RWIN))
			{
				if(KEY_DOWN(VK_MENU))
				{
					POINT pt{0,0};
					GetCursorPos(&pt);
					HWND tmp = WindowFromPoint(pt);
					GetWindowThreadProcessId(tmp, &cur_pid);
					for(int i = 0; i < processes.size(); ++i)
						if(processes.at(i).pid == cur_pid)
							cur_proc_index = i,
							uiproclist_page = (cur_proc_index) / UIPROCESS_LIST_PAGE_CNT;
					lastLoseFocus = clock();
				}else if(KEY_DOWN(VK_CONTROL))
				{
					HWND tmp = GetForegroundWindow();
					GetWindowThreadProcessId(tmp, &cur_pid);
					for(int i = 0; i < processes.size(); ++i)
						if(processes.at(i).pid == cur_pid)
							cur_proc_index = i,
							uiproclist_page = (cur_proc_index) / UIPROCESS_LIST_PAGE_CNT;
					lastLoseFocus = clock();
				}
			}
			break;
		}
		case UIPOS_WINDOW:{
			CtrlAndAlt(UIPOS_WINDOW);
			if(KEY_DOWN(VK_MENU) || KEY_DOWN(VK_CONTROL))
			{
				lastLoseFocus = clock();
				bool isCtrl = KEY_DOWN(VK_CONTROL);
				for(size_t i = 1; i <= UIWINDOW_OPT_CNT; ++i)
				{
					if(KEY_DOWN(i==10?'0':'0' + i) && !executing)
					{
						ui_pos = UIPOS_WINDOW,lastUIPos = clock();;
						if(isCtrl)
						{
							target_hwnd = GetForegroundWindow();
						}else{
							POINT pt{0,0};
							GetCursorPos(&pt);
							target_hwnd = WindowFromPoint(pt);
						}
						lastExec = uiwindow_options.at(i-1);
						bool bRet = Execute();
						if(!bRet || GetLastError() != ERROR_SUCCESS)
							SetTip();
						while(KEY_DOWN(i==10?'0':'0' + i));
						executing = false;
					}
				}
			}
			break;
		}
		case UIPOS_POWER:
		{
			CtrlAndAlt(UIPOS_POWER);
			if(KEY_DOWN(VK_MENU))
			{
				for(size_t i = 1; i <= UIPOWER_OPT_CNT && i < 10; ++i)
				{
					if(KEY_DOWN('0' + i))	//电源选项，需要确认
					{
						ui_pos = UIPOS_CONFIRM;
						lastUIPos = clock();
						lastExec = uipower_options.at(i-1);
						lastExecDrawPos.x = (left_or_right==LEFT?UIPOWER_L_LEFT:UIPOWER_R_LEFT);
						lastExecDrawPos.y = UIPOWER_TOP+(UIPOWER_TEXT_GAP+TEXT_FS)*(i-1);
						while(KEY_DOWN('0'+i));
					}
				}
				if(KEY_DOWN('0'))
				{
					ui_pos = UIPOS_CONFIRM;
					lastUIPos = clock();
					lastExec = uipower_options.at(9);
					lastExecDrawPos.x = (left_or_right==LEFT?UIPOWER_L_LEFT:UIPOWER_R_LEFT);
					lastExecDrawPos.y = UIPOWER_TOP+(UIPOWER_TEXT_GAP+TEXT_FS)*9;
					while(KEY_DOWN('0'));
				}
			}
			break;
		}
		case UIPOS_OTHERS:{
			CtrlAndAlt(UIPOS_OTHERS);
			if(KEY_DOWN(VK_MENU) )
			{
				for(size_t i = 1; i <= UIOTHERS_OPT_CNT && i < 10; ++i)
				{
					if(KEY_DOWN('0' + i) && !executing)
					{
						lastExec = uiothers_options.at(i-1);
						bool bRet = Execute();
						if(!bRet || GetLastError() != ERROR_SUCCESS)
							SetTip();
						while(KEY_DOWN('0'+i));
						executing = false;
					}
				}
			}
			break;
		}
		case UIPOS_CONFIRM:{
			CtrlAndAlt(UIPOS_CONFIRM);
			if(lastExec.first.size() > 6)
			{
				char c = lastExec.first.at(5);	//number
				if(KEY_DOWN(VK_MENU) && KEY_DOWN(c) && !executing)
				{
					ui_pos = UIPOS_MAIN,lastUIPos = clock();;
					bool bRet = Execute();
					if(!bRet)
						SetTip();
					while(KEY_DOWN(VK_MENU) && KEY_DOWN(c));
					executing = false;
				}
			}
			break;
		}
		case UIPOS_ABOUT:{
			CtrlAndAlt(UIPOS_ABOUT);
			break;
		}
		case UIPOS_SCREEN_PAINT:{
			CtrlAndAlt(UIPOS_SCREEN_PAINT);
			if(SCREEN_PAINT_KEY_PRESSED)
			{
				int mx, my;
				POINT pt{0,0};
				GetCursorPos(&pt);
				mx = pt.x, my = pt.y;
				scrp.ClickHere(mx, my);
			}
			break;
		}
		case UIPOS_QUIET_CMD:{
			input_string.clear();
			inputPrevPrevUIPos = UIPOS_COMMAND;
			inputPrevUIPos = UIPOS_QUIET_CMD;
			ui_pos = UIPOS_INPUT;
			lastUIPos = clock();
			break;
		}
		case UIPOS_SPEAKER:{
			inputPrevPrevUIPos = UIPOS_OTHERS;
			inputPrevUIPos = UIPOS_SPEAKER;
			ui_pos = UIPOS_INPUT;
			lastUIPos = clock();
			break;
		}
		case UIPOS_NOTICE:{
			inputPrevPrevUIPos = UIPOS_OTHERS;
			inputPrevUIPos = UIPOS_NOTICE;
			ui_pos = UIPOS_INPUT;
			lastUIPos = clock();
			break;
		}
	}
}
void HandleEnterKey() 
{
	ui_pos = inputPrevUIPos;
	if(ui_pos == UIPOS_QUIET_CMD)
	{
		WinExec((string{"cmd.exe /c "} + input_string).c_str(), SW_HIDE);
	}else if(ui_pos == UIPOS_SPEAKER)
	{
		Speak(input_string);
		SetTip(input_string);
	}else if(ui_pos == UIPOS_NOTICE)
	{
		RandomizeNoticeAnimation();
		SetNotice(input_string);
		SetTip(input_string);
	}
	input_string.clear();
	lastUIPos = clock();
	while(K(VK_RETURN));
	
	if((ui_pos == UIPOS_SPEAKER || ui_pos == UIPOS_NOTICE) 
		&& input_result == IRES_DONE)
	{
		ui_pos = UIPOS_INPUT;
		FocusWindow(hwnd_hiddenEdit);
		return;	//继续输入
	}
	ui_pos = inputPrevPrevUIPos;
	SendMessage(hwnd_hiddenEdit, WM_KILLFOCUS, 0, 0);
	ShowWindow(hwnd_hiddenEdit, SW_HIDE);
	FocusWindow(hwnd);
}
LRESULT CALLBACK WndProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam) 
{
	static HBITMAP hBitmap = NULL;
	
	switch(Message) 
	{
		case WM_CREATE:{
			cout << "hwnd MainWindow = " << (ULONGLONG) hwnd << endl;
			hdcBuffer = CreateCompatibleDC(NULL);
			SetTimer(hwnd, 0, TIMER_PAINT_CD, TimerProc_Paint);
			SetTimer(hwnd, 1, TIMER_HOTKEY_CD, TimerProc_Hotkey);
			SetTimer(hwnd, 2, TIMER_UPDATE_CD, TimerProc_Update);
			cout << "Window Create!\n";
			break;
		}
		case WM_DESTROY: {
			cout << "Window Destroy!\n";
			if (hdcBuffer)
				DeleteDC(hdcBuffer), hdcBuffer = nullptr;
			if (hBitmap)
				DeleteObject(hBitmap), hBitmap = nullptr;
			KillTimer(hwnd, 0);
			KillTimer(hwnd, 1);
			KillTimer(hwnd, 2);
			if(ExistFile("QuickPanelSpeech.vbs"))
				DeleteFileA("QuickPanelSpeech.vbs");
			PostQuitMessage(0);
			break;
		}
		case WM_PAINT:{
			BeginDraw();
			
			// 获取客户区域的大小
			RECT rcClient;
			GetClientRect(hwnd, &rcClient);
			int clientWidth = rcClient.right - rcClient.left;
			int clientHeight = rcClient.bottom - rcClient.top;
			
			// 创建双缓冲
			if (hBitmap)
				DeleteObject(hBitmap);
			hBitmap = CreateCompatibleBitmap(hdcOrigin, clientWidth, clientHeight);
			SelectObject(hdcBuffer, hBitmap);
			
			ClearDevice();
			if(ui_pos == UIPOS_SCREEN_PAINT)
				scrp.Init(hdcBuffer);
			DrawUI();
			if(ui_pos == UIPOS_SCREEN_PAINT)
				scrp.Dispose();
			
			// 将缓冲区的内容一次性绘制到屏幕上
			BitBlt(hdcOrigin, 0, 0, clientWidth, clientHeight, hdcBuffer, 0, 0, SRCCOPY);
			EndDraw();
//			cout << "Window Paint!\n";
			break;
		}
	default:
		return DefWindowProc(hwnd, Message, wParam, lParam);
	}
	return 0;
}
bool TerminalCheck(DWORD dwPid, HWND _hwnd)
{	//检查是否为win11新终端
	HANDLE hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);	
	if (INVALID_HANDLE_VALUE == hSnapshot) 	
	{		
		return false;	
	}	
	PROCESSENTRY32 pe = { sizeof(pe) };	
	BOOL fOk;	
	for (fOk = Process32First(hSnapshot, &pe); fOk; fOk = Process32Next(hSnapshot, &pe)) 	
	{		
		if (! stricmp(pe.szExeFile, "WindowsTerminal.exe")
			&& pe.th32ProcessID == dwPid) 		
		{			
			CloseHandle(hSnapshot);			
			{
				char title[MAX_PATH];
				GetWindowText(_hwnd, title, MAX_PATH);
				if(strcmp(title, _pgmptr) && strcmp(title, "QuickPanel"))
					return false;
				return true;
			}		
		}	
	}	
	return false;
}
BOOL CALLBACK EnumWindowsProc(HWND _hwnd, LPARAM lParam)
{
	DWORD pid;
	GetWindowThreadProcessId(_hwnd, &pid);
	if(TerminalCheck(pid, _hwnd))
	{
		hwnd_console = _hwnd;
		return FALSE;
	}
	return TRUE;
}
int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	_hInstance = hInstance;
	_hPrevInstance = hPrevInstance;
	_lpCmdLine = lpCmdLine;
	_nShowCmd = nShowCmd;
	
	scr_w = GetScreenWidth();
	scr_h = GetScreenHeight();
	taskbar_h = GetTaskbarHeight();
	
	WNDCLASS wc = { 0 };
	wc.lpfnWndProc = WndProc;
	wc.hInstance = hInstance;
	wc.lpszClassName = QPWND_CLASS_NAME;
	
	if(!RegisterClass(&wc)) {
		MessageBox(NULL, "Window Registration Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}
	
	hwnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST, 
		QPWND_CLASS_NAME, QPWND_CAPTION, WS_POPUP,
		0, /* x */
		0, /* y */
		scr_w, /* width */
		scr_h, /* height */
		GetTaskbarWindow(),
//		NULL,
		NULL,hInstance,NULL);
	
	if(hwnd == NULL) {
		MessageBox(NULL, "Window Creation Failed!","Error!",MB_ICONEXCLAMATION|MB_OK);
		return 0;
	}
	SetConsoleTitle("QuickPanel");
	if(ExistProcess("WindowsTerminal.exe"))
	{	//win11电脑且使用新版终端
		EnumWindows(EnumWindowsProc, 0);
	}else{	//旧版控制台主机
		hwnd_console = GetConsoleWindow();
	}
#ifndef SHOWCONSOLE
	if(hwnd_console != INVALID_HANDLE_VALUE && hwnd_console != nullptr)
		ShowWindow(hwnd_console, SW_HIDE);
#endif
	if(IsRunAsAdmin())
	{
		bool bRes = GetDebugPrivilege();
		if(!bRes)
		{
			SetTip();
		}else{
			SetTip("$ Administrator with Debug Privilege $");
		}
	}
	// 设置窗口透明度
	SetLayeredWindowAttributes(hwnd, RGB(0, 0, 0), 0, LWA_COLORKEY);
	ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	SetWindowPos(hwnd, HWND_TOPMOST, 0, 0, scr_w,scr_h, SWP_NOSIZE);
	MSG msg;
	
	cout << "Loop\n";
	while(GetMessage(&msg, NULL, 0, 0) > 0) { /* If no error is received... */
		TranslateMessage(&msg); /* Translate key codes to chars if present */
		DispatchMessage(&msg); /* Send it to WndProc */
		ShowWindowAsync(hwnd, SW_SHOWMAXIMIZED); //保持显示并最大化
	}
	
	return 0;
}

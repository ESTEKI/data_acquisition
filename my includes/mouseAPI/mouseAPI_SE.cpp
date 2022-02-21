#include "stdafx.h"
#include "mouseAPI_SE.hpp"

HHOOK hook_mouseAPI = NULL;//global variables are bad ,:D
int  MouseAPI::rollAngle;
int MouseAPI::pitchAngle;
int MouseAPI::xDisplacement =0;
int MouseAPI::yDisplacement=0;
struct MouseAPI::MouseData mdata;

LRESULT CALLBACK MouseHookProc(int nCode, WPARAM wParam, LPARAM lParam)
{
	if (nCode != HC_ACTION)  // Nothing to do :(
		return CallNextHookEx(NULL, nCode, wParam, lParam);


	MSLLHOOKSTRUCT *info = reinterpret_cast<MSLLHOOKSTRUCT*>(lParam);

	char const *button_name[] = { "Left", "Right", "Middle", "X" };
	enum { BTN_LEFT, BTN_RIGHT, BTN_MIDDLE, BTN_XBUTTON,SCROLL, BTN_NONE } button = BTN_NONE;

	char const *up_down[] = { "up", "down" };
	bool down = false;


	switch (wParam)
	{

	case WM_LBUTTONDOWN: down = true;
	case WM_LBUTTONUP: button = BTN_LEFT;
		break;
	case WM_RBUTTONDOWN: down = true;
	case WM_RBUTTONUP: button = BTN_RIGHT;
		break;
	case WM_MBUTTONDOWN: down = true;
	case WM_MBUTTONUP: button = BTN_MIDDLE;
		break;
	case WM_XBUTTONDOWN: down = true;
	case WM_XBUTTONUP: button = BTN_XBUTTON;
		break;

	case WM_MOUSEWHEEL:
		// the hi order word might be negative, but WORD is unsigned, so
		// we need some signed type of an appropriate size:
		button = SCROLL;
		down = static_cast<std::make_signed_t<WORD>>(HIWORD(info->mouseData)) < 0;
		//std::cout << "Mouse wheel scrolled " << up_down[down] << '\n';//uncomment this for test
		break;

	}
	MouseAPI::ButtonProcess(button, down);
	if (button != BTN_NONE) {
		//std::cout << button_name[button];

		//if (button == BTN_XBUTTON)
		//	std::cout << HIWORD(info->mouseData);
		//std::cout << " mouse button " << up_down[down] << '\n';
	}

	return CallNextHookEx(NULL, nCode, wParam, lParam);
}

BOOL WINAPI ctrl_handler(DWORD dwCtrlType)
{
	if (hook_mouseAPI) {
		std::cout << "Unhooking " << hook_mouseAPI << '\n';
		UnhookWindowsHookEx(hook_mouseAPI);
		hook_mouseAPI = NULL;  // ctrl_handler might be called multiple times
							   //std::cout << "Bye :(";
							   //std::cin.get();  // gives the user 5 seconds to read our last output
	}

	return TRUE;
}


void MouseAPI::InitMouseAPI() {

	isMouseOverConsoleWin = false;
	HWND consoleWindow = GetConsoleWindow();
	SetWindowPos(consoleWindow, 0, xconsole, yconsole, dxconsole, dyconsole, SWP_NOZORDER);//(handl,0,xpos,ypos,width,height,bla...)}
	std::cout << "Right click on console upper border -> properties-> UNCHECK QuickEdit mode -> ok\n";
	MouseAPI::rollAngle = 0;
}

bool MouseAPI::HookMouse() {
	SetConsoleCtrlHandler(ctrl_handler, TRUE);
	hook_mouseAPI = SetWindowsHookExW(WH_MOUSE_LL, MouseHookProc, nullptr, 0);

	if (!hook_mouseAPI) {
		std::cerr << "SetWindowsHookExW() failed. Bye :(\n\n";
		return false;
	}

	//std::cout << "Hook set: " << hook_mouseAPI << '\n';
	////GetMessageW(nullptr, nullptr, 0, 0);

	return true;
}

bool MouseAPI::UnHookMouse() {//*return 0 if succeed
	if (hook_mouseAPI) {
		//std::cout << "Unhooking " << hook_mouseAPI << '\n';
		UnhookWindowsHookEx(hook_mouseAPI);
		hook_mouseAPI = NULL;  // ctrl_handler might be called multiple times
							   //std::cout << "Bye :(";
		return false;
		//std::cin.get();  // gives the user 5 seconds to read our last output
	}
	else {
		return true;
	}
}

MouseAPI::MouseData MouseAPI::Run()
{
	 
	if (!hook_mouseAPI)
	{
		HookMouse();
		prevCoor = GetxyCoord();
	}
	
	//must call GetMessage or PeekMessage so as to program calls  MouseHookProc
	while (PeekMessage(NULL, NULL, 0, 0, PM_REMOVE))
	{
	}
	GetDisplacement(&mdata.xdistance, &mdata.ydistance,&mdata.vx ,&mdata.vy);
	mdata.rollAngle = rollAngle;
	mdata.pitchAngle = pitchAngle;
	//UnHookMouse();
	return mdata;
}

void MouseAPI::GetDisplacement(long *dx, long *dy,int *vx,int *vy)
{
	GetCursorPos(&pos);
	*vx = pos.x - prevCoor.x;
	*vy = pos.y - prevCoor.y;
	xDisplacement += *vx;
	yDisplacement += *vy;
	prevCoor.x = pos.x;
	prevCoor.y = pos.y;
	//prevCoor.x = xconsole + dxconsole / 2;
	//prevCoor.y = yconsole + dyconsole / 2;
	//SetCursorPos(prevCoor.x, prevCoor.y);
	
	if (pos.x < xconsole) 
	{
		SetCursorPos((xconsole + dxconsole ), pos.y); prevCoor.x = (xconsole + dxconsole ); 
	} 
	else if (pos.x >(xconsole + dxconsole)) 
	{
		SetCursorPos(xconsole , pos.y); prevCoor.x = xconsole ; 
	} 
	else if (pos.y < yconsole) 
	{
		SetCursorPos(pos.x, yconsole + dyconsole ); prevCoor.y = yconsole + dyconsole ;
	} 
	else if (pos.y >(yconsole + dyconsole)) 
	{
		SetCursorPos(pos.x, yconsole ); prevCoor.y = yconsole ; 
	} 

	*dx = xDisplacement;
	*dy = yDisplacement;
}

POINT MouseAPI::GetxyCoord() {
	GetCursorPos(&pos);
	return pos;
}

bool MouseAPI::MkeyIsDown() {
	keyState = GetKeyState(0x4D);// virtual code for M key in keyboard
	bool isDown = (keyState & 0x8000);
	return isDown;
}

bool MouseAPI::NkeyIsDown() {
	keyState = GetKeyState(0x4E);// virtual code for N key in keyboard
	bool isDown = (keyState & 0x8000);
	return isDown;
}

bool MouseAPI::OkeyIsDown() {
	keyState = GetKeyState(0x4F);// virtual code for N key in keyboard
	bool isDown = (keyState & 0x8000);
	return isDown;
}

bool MouseAPI::IsMouseOverWin() {
	GetxyCoord();
	if (pos.x > xconsole && pos.x < xconsole + dxconsole && pos.y > yconsole && pos.y < yconsole + dyconsole) {
		isMouseOverConsoleWin = true;
	}
	else {
		isMouseOverConsoleWin = false;
	}
	return isMouseOverConsoleWin;
}

int MouseAPI::ButtonProcess(int button, bool down)
{
	enum { BTN_LEFT, BTN_RIGHT, BTN_MIDDLE, BTN_XBUTTON, SCROLL, BTN_NONE };// button = BTN_NONE;
	 
		switch (button)
		{
		case BTN_MIDDLE:
			if (down)
			{
				MouseAPI::rollAngle = 0;
				MouseAPI::pitchAngle = 0;
				MouseAPI::xDisplacement = 0;
				MouseAPI::yDisplacement = 0;
			}
			break;
		case BTN_LEFT:
			if (down)
			 MouseAPI::rollAngle++;
			break;
		case BTN_RIGHT:
			if (down)
			 MouseAPI::rollAngle--;
			break;
		case SCROLL:
			if (down)
				MouseAPI::pitchAngle++;
			else
				MouseAPI::pitchAngle--;
			break;
		default:
			break;
		}
	
	
	return 0;
}

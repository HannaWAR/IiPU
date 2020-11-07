#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib,"vfw32.lib")
#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <vfw.h>
#include <string>

//  ESC - exit
//	H   - show/hide window
//	V   - start/stop recording video
//	P   - make photo

HHOOK hHookKeyboard;
HWND capture_handle;
bool recording_video = false;

LRESULT CALLBACK keyboard_hook_handle(int nCode, WPARAM wParam, LPARAM lParam);
DWORD WINAPI capture_photo(LPVOID lpParam);
void start_video_capture();
void stop_video_capture();

int main()
{
	bool has_device = false;
	int i;
	for (i = 0; i < 10; ++i) // index = [1;9] (documentation)
	{
		CHAR name[256];
		CHAR description[256];
		if (capGetDriverDescriptionA(i, name, 255, description, 255))
		{
			std::cout << name << std::endl;
			std::cout << description << std::endl;
			system("pause");
			has_device = true;
			break;
		}
	}
	if (!has_device)
	{
		std::cout << "No webcam installed";
		system("pause");
		return 1;
	}

	HWND window_handle = GetForegroundWindow();		//returns the handle of the window with which the user is currently working
	ShowWindow(window_handle, SW_HIDE);

	if (!(capture_handle = capCreateCaptureWindowA("Capture", WS_VISIBLE, 0, 0, 640, 480, window_handle, 0)))
	{
		std::cout << "Failed to create capture window";
		system("pause");
		return 1;
	}

	while (!capDriverConnect(capture_handle, i));

	hHookKeyboard = SetWindowsHookExW(WH_KEYBOARD_LL, keyboard_hook_handle, GetModuleHandle(NULL), 0);

	capPreviewScale(capture_handle, TRUE);	// stretch image to fit window
	capPreviewRate(capture_handle, 30);		// set preview framerate
	capPreview(capture_handle, TRUE);		// enable preview

	MSG message = { 0 };
	while (GetMessage(&message, NULL, 0, 0))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
	capDriverDisconnect(capture_handle);
	DestroyWindow(capture_handle);
	return 0;
}


LRESULT CALLBACK keyboard_hook_handle(int nCode, WPARAM wParam, LPARAM lParam)
{
	static bool window_is_hidden = false;
	KBDLLHOOKSTRUCT* ks = (KBDLLHOOKSTRUCT*)lParam;		//contains information about a low-level keyboard input event
	if (ks->vkCode == 0x1B)								//esc - exit
	{
		if (recording_video)
		{
			stop_video_capture();
		}
		TerminateProcess(GetCurrentProcess(), NO_ERROR);
	}

	if (ks->vkCode == 0x50 && (ks->flags & 0x80) == 0)	// P - make photo; flags[7] if is 0 if the key is pressed and 1 if it is being released
	{
		CreateThread(NULL, 0, capture_photo, NULL, 0, NULL);
	}

	if (ks->vkCode == 0x48 && (ks->flags & 0x80) == 0)	// H - hide
	{
		if (window_is_hidden == true)
		{
			ShowWindow(capture_handle, SW_NORMAL);
			window_is_hidden = false;
		}
		else
		{
			ShowWindow(capture_handle, SW_HIDE);
			window_is_hidden = true;
		}
		return -1;
	}

	if (ks->vkCode == 0x56 && (ks->flags & 0x80) == 0)	// V - start video
	{
		CAPTUREPARMS capparms;
		capCaptureGetSetup(capture_handle, &capparms, sizeof(CAPTUREPARMS));

		capparms.fMakeUserHitOKToCapture = TRUE;	// show dialog box at the start of recording
		capparms.fYield = TRUE;						// capture in background
		capparms.fCaptureAudio = TRUE;				// caprture audio
		capparms.fAbortLeftMouse = FALSE;			// don't abort on left mouse click
		capparms.fAbortRightMouse = FALSE;			// don't abort on right mouse click
		capparms.dwRequestMicroSecPerFrame = 33333;	// ~30fps

		capCaptureSetSetup(capture_handle, &capparms, sizeof(CAPTUREPARMS));

		if (recording_video)
		{
			stop_video_capture();
		}
		else
		{
			start_video_capture();
		}
		return -1;
	}
	return CallNextHookEx(hHookKeyboard, nCode, wParam, lParam);
}

DWORD WINAPI capture_photo(LPVOID lpParam)
{
	static int number_of_image_files = 0;
	if (!recording_video) {
		capCaptureSingleFrameOpen(capture_handle);
		capCaptureSingleFrame(capture_handle);
		capCaptureSingleFrameClose(capture_handle);
	}
	else
	{
		capGrabFrameNoStop(capture_handle);
	}
	WCHAR filename[10];
	HANDLE file_handle = 0;
	while (file_handle != INVALID_HANDLE_VALUE)
	{
		memset(filename, 0, sizeof(filename));
		_itow(number_of_image_files++, filename, 10);
		wcscat(filename, L".bmp");
		file_handle = CreateFileW(filename, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}
	capFileSaveDIB(capture_handle, filename);
	return -1;
}

void stop_video_capture()
{
	capCaptureAbort(capture_handle);
	recording_video = false;
}

void start_video_capture()
{
	static int number_of_video_files = 0;
	WCHAR filename[10];
	HANDLE file_handle = 0;
	while(file_handle != INVALID_HANDLE_VALUE)
	{
		memset(filename, 0, sizeof(filename));
		_itow(number_of_video_files++, filename, 10);
		wcscat(filename, L".avi");
		file_handle = CreateFileW(filename, GENERIC_READ, NULL, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	}

	capFileSetCaptureFile(capture_handle, filename);
	capCaptureSequence(capture_handle);
	recording_video = true;
}

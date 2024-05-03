#if __APPLE__
#include <CoreFoundation/CoreFoundation.h>

static char response[1024];
static CFMessagePortRef port;

__attribute__((constructor)) void init() {
	port = CFMessagePortCreateRemote(NULL, CFStringCreateWithCString(NULL, "GeodeIPCPipe", kCFStringEncodingUTF8));
}

char* sendMessage(char const* text) {
	CFDataRef data = CFDataCreate(NULL, (uint8_t*)text, strlen(text));

	CFDataRef ret;
	CFMessagePortSendRequest(port, 0, (CFDataRef)data, 1, 1, kCFRunLoopDefaultMode, &ret);
	
	if (ret == NULL) {
		return NULL;
	}

	strncpy(response, (char*)CFDataGetBytePtr(ret), 1024);
	CFRelease(ret);

	return response;
}
#else
#include <Windows.h>

static char response[1024];
char* sendMessage(char const* text) {
	// Send a message to the named pipe "GeodeIPCPipe"

	DWORD dw;

	HANDLE hPipe = CreateFile(TEXT("\\\\.\\pipe\\GeodeIPCPipe"), 
	                   GENERIC_READ | GENERIC_WRITE, 
	                   0,
	                   NULL,
	                   OPEN_EXISTING,
	                   0,
	                   NULL);

	if (hPipe != INVALID_HANDLE_VALUE) {
	    WriteFile(hPipe,
	              text,
	              strlen(text) + 1,
	              &dw,
	              NULL);
	    ReadFile(hPipe, response, 1024, &dw, NULL);

	    CloseHandle(hPipe);
	}

	return response;
}
#endif

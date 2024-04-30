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
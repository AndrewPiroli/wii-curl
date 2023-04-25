#include <wiisocket.h>
#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gctypes.h>
#include <gccore.h>

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

struct memory {
  char *response;
  size_t size;
};
 
static size_t cb(void *data, size_t size, size_t nmemb, void *clientp) {
	size_t realsize = size * nmemb;
	struct memory *mem = (struct memory *)clientp;

	char *ptr = realloc(mem->response, mem->size + realsize + 1);
	if(ptr == NULL)
		return 0;  /* OOM */

	mem->response = ptr;
	memcpy(&(mem->response[mem->size]), data, realsize);
	mem->size += realsize;
	mem->response[mem->size] = 0;

	return realsize;
}

bool die = false;

void timetostop() { die = true; }

int main(void) {
	curl_global_init(CURL_GLOBAL_ALL);
	VIDEO_Init();
	rmode = VIDEO_GetPreferredMode(NULL);
	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb, 0, 0, rmode->fbWidth, rmode->xfbHeight,
		     rmode->fbWidth * VI_DISPLAY_PIX_SZ);
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if (rmode->viTVMode & VI_NON_INTERLACE) {
		VIDEO_WaitVSync();
	}
	SYS_SetResetCallback(timetostop);
	SYS_SetPowerCallback(timetostop);

	printf("libcurl version: %s\n", curl_version());

	struct memory chunk = {0};
	chunk.response = malloc(1);
	chunk.size = 0;
	// try a few times to initialize libwiisocket (?)
	int socket_init_success = -1;
	for (int attempts = 0;attempts < 3;attempts++) {
		socket_init_success = wiisocket_init();
		printf("attempt: %d wiisocket_init: %d\n", attempts, socket_init_success);
		if (socket_init_success == 0)
			break;
	}
	if (socket_init_success != 0) {
		puts("failed to init wiisocket");
		goto loop;
	}
	// try a few times to get an ip (?)
	u32 ip = 0;
	for (int attempts = 0; attempts < 3; attempts++) {
		ip = gethostid();
		printf("attempt: %d gethostid: %x\n", attempts, ip);
		if (ip)
			break;
	}
	if (!ip) {
		puts("failed to get ip");
		goto loop;
	}
	char err[CURL_ERROR_SIZE + 1] = {0};
	CURL* curl = curl_easy_init();
	if (!curl){
		puts("curl easy init failed");
		goto loop;
	}
	char* url = "http://httpforever.com";
	curl_easy_setopt(curl, CURLOPT_URL, url);
	// Deprecated in curl 7.85.0 but in case you are using an old version:
	//curl_easy_setopt(curl, CURLOPT_PROTOCOLS, CURLPROTO_HTTP);
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "HTTP");
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, cb);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*)&chunk);
	printf("HTTP HEAD target = %s\n", url);
	CURLcode res = curl_easy_perform(curl);
	if (res == CURLE_OK) {
		printf("%s\n", chunk.response);
	}
	else {
		printf("curl fail %d\n%s\n", res, err);
	}
	free(chunk.response);

loop:
	puts("Done. Press reset or power to exit");
	while (!die) {
		VIDEO_WaitVSync();
	}
	exit(0);
}


#include <wiisocket.h>
#include <stdio.h>
#include <curl/curl.h>
#include <mbedtls/sha1.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gctypes.h>
#include <gccore.h>

#define HTTP_TEST_URL "http://httpforever.com"
#define FTP_TEST_URL "ftp://speedtest.tele2.net/1MB.zip"

static void *xfb = NULL;
static GXRModeObj *rmode = NULL;

bool done = false;
bool die = false;

void timetostop() { die = done; }

struct memory {
  char *response;
  size_t size;
};

static size_t cb(void *data, size_t size, size_t nmemb, void *clientp) {
	size_t realsize = size * nmemb;
	struct memory *mem = (struct memory *)clientp;

	char *ptr = realloc(mem->response, mem->size + realsize + 1);
	if(ptr == NULL){
		puts("OOM");
		return 0;
	}

	mem->response = ptr;
	memcpy(&(mem->response[mem->size]), data, realsize);
	mem->size += realsize;
	mem->response[mem->size] = 0;

	return realsize;
}

char* http_head(char* url, char* err) {
	CURL* curl = curl_easy_init();
	if (!curl){
		err = "curl easy init failed";
		return NULL;
	}
	struct memory chunk = {0};
	chunk.response = malloc(1);
	chunk.size = 0;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "HTTP");
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err);
	curl_easy_setopt(curl, CURLOPT_NOBODY, 1);
	curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, cb);
	curl_easy_setopt(curl, CURLOPT_HEADERDATA, (void*)&chunk);
	CURLcode res = curl_easy_perform(curl);
	if (res == CURLE_OK) {
		return chunk.response;
	}
	else {
		free(chunk.response);
		return NULL;
	}
}

struct memory* ftp_dl(char* url, char* err, char* login) {
	CURL* curl = curl_easy_init();
	if (!curl){
		err = "curl easy init failed";
		return NULL;
	}
	struct memory* ret = malloc(sizeof *ret);
	if (!ret) {
		err = "OOM in FTP setup";
		return NULL;
	}
	ret->response = malloc(1);
	ret->size = 0;
	curl_easy_setopt(curl, CURLOPT_URL, url);
	curl_easy_setopt(curl, CURLOPT_USERPWD, login);
	curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "FTP");
	curl_easy_setopt(curl, CURLOPT_ERRORBUFFER, err);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, cb);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)ret);
	CURLcode res = curl_easy_perform(curl);
	if (res == CURLE_OK) {
		return ret;
	}
	else {
		printf("FTP Failed curl ret: %d\n", res);
		free(ret->response);
		free(ret);
		return NULL;
	}
}


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
	// Test HTTP
	puts("HTTP HEAD: " HTTP_TEST_URL);
	char* response = http_head(HTTP_TEST_URL, (char*)&err);
	if (!response) {
		printf("FAIL: %s\n", err);
		goto loop;
	}
	else {
		// print just the first line
		char* end = strchr(response, '\r');
		if (end)
			response[end - response] = '\0';
		printf("Response: %s\n", response);
		free(response);
	}
	// Test FTP
	puts("FTP GET: " FTP_TEST_URL);
	struct memory* ftp_response = ftp_dl(FTP_TEST_URL, (char*)&err, "anonymous:asdf");
	if (!ftp_response) {
		printf("FAIL: %s\n", err);
		goto loop;
	}
	else {
		size_t size = ftp_response->size;
		printf("Response size: %d\n", size);
		u32 hash[5] = {0};
		// Dolphin does not implement /dev/sha so use mbedtls instead of libogc
		mbedtls_sha1_ret((u8*)ftp_response->response, size, (u8*)&hash);
		printf("SHA1: ");
		for (size_t i = 0; i < 5; i++)
			printf("%08x", hash[i]);
		putchar('\n');
		free(ftp_response->response);
		free(ftp_response);
	}

loop:
	done = true;
	puts("Done. Press reset or power to exit");
	while (!die) {
		VIDEO_WaitVSync();
	}
	exit(0);
}


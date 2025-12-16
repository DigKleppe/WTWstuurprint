// /* ICMP echo example

//    This example code is in the Public Domain (or CC0 licensed, at your option.)

//    Unless required by applicable law or agreed to in writing, this
//    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
//    CONDITIONS OF ANY KIND, either express or implied.
// */

#include "esp_check.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/sockets.h"
#include "ping/ping_sock.h"
#include "sdkconfig.h"
#include <stdio.h>
#include <string.h>

#include "../../wifiConnect/include/wifiConnect.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#define LOG_LOCAL_LEVEL ESP_LOG_INFO // ESP_LOG_ERROR
#include "esp_log.h"

const static char *TAG = "ping";

#define PINGINTERVAL 60 // seconds

volatile int pingOKCntr = 0;
volatile int pingFailedCntr = 0;

static void test_on_ping_success(esp_ping_handle_t hdl, void *args) {
	// optionally, get callback arguments
	// const char* str = (const char*) args;
	// printf("%s\r\n", str); // "foo"
	uint8_t ttl;
	uint16_t seqno;
	uint32_t elapsed_time, recv_len;
	ip_addr_t target_addr;
	esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
	esp_ping_get_profile(hdl, ESP_PING_PROF_TTL, &ttl, sizeof(ttl));
	esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
	esp_ping_get_profile(hdl, ESP_PING_PROF_SIZE, &recv_len, sizeof(recv_len));
	esp_ping_get_profile(hdl, ESP_PING_PROF_TIMEGAP, &elapsed_time, sizeof(elapsed_time));
	// printf("%d bytes from %s icmp_seq=%d ttl=%d time=%d ms\n",(int) recv_len, inet_ntoa(target_addr.u_addr.ip4), (int) seqno, ttl,(int)  elapsed_time);
	ESP_LOGI(TAG, "%d bytes from %s icmp_seq=%d ttl=%d time=%d ms\n", (int)recv_len, inet_ntoa(target_addr.u_addr.ip4), (int)seqno, ttl, (int)elapsed_time);
	pingOKCntr += 1;
}

static void test_on_ping_timeout(esp_ping_handle_t hdl, void *args) {
	uint16_t seqno;
	ip_addr_t target_addr;
	esp_ping_get_profile(hdl, ESP_PING_PROF_SEQNO, &seqno, sizeof(seqno));
	esp_ping_get_profile(hdl, ESP_PING_PROF_IPADDR, &target_addr, sizeof(target_addr));
	//	printf("From %s icmp_seq=%d timeout\n", inet_ntoa(target_addr.u_addr.ip4), (int) seqno);
	ESP_LOGE(TAG, "From %s icmp_seq=%d timeout\n", inet_ntoa(target_addr.u_addr.ip4), (int)seqno);
	pingFailedCntr += 1;
}

static void test_on_ping_end(esp_ping_handle_t hdl, void *args) {
	uint32_t transmitted;
	uint32_t received;
	uint32_t total_time_ms;

	esp_ping_get_profile(hdl, ESP_PING_PROF_REQUEST, &transmitted, sizeof(transmitted));
	esp_ping_get_profile(hdl, ESP_PING_PROF_REPLY, &received, sizeof(received));
	esp_ping_get_profile(hdl, ESP_PING_PROF_DURATION, &total_time_ms, sizeof(total_time_ms));
	//	printf("%d packets transmitted, %d received, time %dms\n",(int) transmitted, (int) received, (int)total_time_ms);
	ESP_LOGI(TAG, "%d packets transmitted, %d received, time %dms\n", (int)transmitted, (int)received, (int)total_time_ms);
}

void pingTask(void *pvParameters) {

	while (connectStatus != CONNECT_READY) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

	esp_ping_config_t ping_config = ESP_PING_DEFAULT_CONFIG();
	ping_config.target_addr.type = IPADDR_TYPE_V4;
	ping_config.target_addr.u_addr.ip4 = (ip4_addr_t)wifiSettings.gw.addr;

	ping_config.count = 2; // ESP_PING_COUNT_INFINITE;

	/* set callback functions */
	esp_ping_callbacks_t cbs;
	cbs.on_ping_success = test_on_ping_success;
	cbs.on_ping_timeout = test_on_ping_timeout;
	cbs.on_ping_end = test_on_ping_end;
	cbs.cb_args = NULL;

	esp_ping_handle_t ping;

	esp_ping_new_session(&ping_config, &cbs, &ping);

	while (1) {
		if (esp_ping_start(ping) == ESP_OK)
			ESP_LOGI(TAG, "ping");
		else
			ESP_LOGE(TAG, "ping start failed");

		vTaskDelay(PINGINTERVAL * 1000 / portTICK_PERIOD_MS);
	}
}
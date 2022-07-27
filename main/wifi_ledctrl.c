#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/event_groups.h>
#include <driver/gpio.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_event.h>
#include <nvs_flash.h>

#include <lwip/err.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/netdb.h>
#include <lwip/dns.h>

#include "wifi_ledctrl.h"

#define WIFI_CONNECTED 1 << 0
#define WIFI_FAIL 1 << 1
#define TCP_SUCCESS 1 << 0
#define TCP_FAIL 1 << 1
#define MAX_RETRIES 10
#define PORT // CHOOSE A PORT HERE

EventGroupHandle_t ev_group;
int retry_num = 0;

void init_nvs() {
    esp_err_t ret = nvs_flash_init(); 
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
}

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Connecting...");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (retry_num < MAX_RETRIES) {
            ESP_LOGI(TAG, "Reconnecting...");
            esp_wifi_connect();
            retry_num++;
        } 
        else {
            xEventGroupSetBits(ev_group, WIFI_FAIL);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;
        xEventGroupSetBits(ev_group, WIFI_CONNECTED);
    }



}

void connect_wifi() {

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ev_group = xEventGroupCreate();
    esp_event_handler_instance_t wifi_ev_handler_instance;
    esp_event_handler_instance_t ip_ev_handler_instance;
 
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            WIFI_EVENT,
            ESP_EVENT_ANY_ID,
            &event_handler,
            NULL,
            &wifi_ev_handler_instance
        ) 
    );
    
    ESP_ERROR_CHECK(
        esp_event_handler_instance_register(
            IP_EVENT,
            IP_EVENT_STA_GOT_IP,
            &event_handler,
            NULL,
            &ip_ev_handler_instance
        ) 
    );
    
    wifi_config_t wifi_conf = {
        .sta = {
            .ssid = "",     //Wifi SSID
            .password = "", //Wifi password
            .threshold.authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };

    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_conf));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    EventBits_t bits = xEventGroupWaitBits(
        ev_group,
        WIFI_CONNECTED | WIFI_FAIL,
        pdFALSE,
        pdFALSE,
        portMAX_DELAY 
    );
    
    if (bits & WIFI_CONNECTED) {
        ESP_LOGI(TAG, "Connected");
    } else if (bits & WIFI_FAIL) {
        ESP_LOGI(TAG, "Failed to connect");
    } else {
        ESP_LOGI(TAG, "Unexpected event");
    }
    
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_ev_handler_instance));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_ev_handler_instance));
    vEventGroupDelete(ev_group);


}

void tcp_server_start() {

	struct sockaddr_in address;
	char inBuffer[1024] = {0};

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = htonl(INADDR_ANY);
	address.sin_port = htons(PORT);

	int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);

	if (sock < 0) {
		ESP_LOGE(TAG, "Failed to create a socket");
        return;
	}
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    if (bind(sock, (struct sockaddr *)&address, sizeof(address)) != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: %d", errno);
        close(sock);
        return;
    }
        
    if (listen(sock, 1) != 0) {
        ESP_LOGE(TAG, "Error during listen &d", errno);
        close(sock);
        return;
    } 

    struct sockaddr_in source_address;
    uint addr_len = sizeof(source_address);
    
    while (1) {

        ESP_LOGI(TAG, "Listening");
        int client = accept(sock, (struct sockaddr *)&source_address, (socklen_t*)&addr_len);
    
        if (client < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: %d", errno);
            return; 
        }
        else {
            ESP_LOGI(TAG, "Connected");
        }
    
        while(1) {
        
            int r = recv(client, inBuffer, sizeof(inBuffer) - 1, 0);
            if (r > 0) {
                inBuffer[r] = 0;
                ESP_LOGI(TAG, "%s", inBuffer);
                parse_command(inBuffer);
            }
            else {
                ESP_LOGI(TAG, "disconnected");
                break;
            }
        }
    }
    
}

void app_main(void) {
   
    init_nvs();
    connect_wifi();
   
    ledc_init();

    tcp_server_start();


}

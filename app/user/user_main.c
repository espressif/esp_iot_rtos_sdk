/******************************************************************************
 * Copyright 2013-2014 Espressif Systems (Wuxi)
 *
 * FileName: user_main.c
 *
 * Description: entry file of user application
 *
 * Modification history:
 *     2014/12/1, v1.0 create this file.
*******************************************************************************/
#include "esp_common.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "udhcp/dhcpd.h"

#include "wifi_config.h"
#include "esp8266/uart_register.h"

void task3(void *pvParameters)
{
    while (1) {
        struct sockaddr_in server_addr, client_addr;
        int server_sock, client_sock;
        socklen_t sin_size;
        bzero(&server_addr, sizeof(struct sockaddr_in));
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(80);

        int recbytes, sentbytes;

        do {
            if (-1 == (server_sock = socket(AF_INET, SOCK_STREAM, 0))) {
                printf("S > socket error\n");
                break;
            }

            printf("S > create socket: %d\n", server_sock);

            if (-1 == bind(server_sock, (struct sockaddr *)(&server_addr), sizeof(struct sockaddr))) {
                printf("S > bind fail\n");
                break;
            }

            printf("S > bind port: %d\n", ntohs(server_addr.sin_port));

            if (-1 == listen(server_sock, 5)) {
                printf("S > listen fail\n");
                break;
            }

            printf("S > listen ok\n");

            sin_size = sizeof(client_addr);

            for (;;) {
                printf("S > wait client\n");

                if ((client_sock = accept(server_sock, (struct sockaddr *) &client_addr, &sin_size)) < 0) {
                    printf("S > accept fail\n");
                    continue;
                }

                printf("S > Client from %s %d\n", inet_ntoa(client_addr.sin_addr), htons(client_addr.sin_port));

                char *recv_buf = (char *)zalloc(129);
                while ((recbytes = read(client_sock , recv_buf, 128)) > 0) {
                    recv_buf[recbytes] = 0;
                    printf("S > read data success %d!\nS > %s\n", recbytes, recv_buf);
                    sentbytes = write(client_sock , recv_buf, recbytes);
                    printf("S > write %d\n", sentbytes);
                }
                free(recv_buf);

                if (recbytes <= 0) {
                    printf("S > read data fail!\n");
                    close(client_sock);
                }
            }
        } while (0);
    }
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void ICACHE_FLASH_ATTR
user_init(void)
{
    uart_div_modify(0, UART_CLK_FREQ / 115200);
    // print system info
    printf("System Info:\r\n");
    printf("Time=%u\r\n", system_get_time());
    printf("RTC time=%u\r\n", system_get_rtc_time());
    printf("Chip id=0x%x\r\n", system_get_chip_id());
    printf("Free heap size=%u\r\n", system_get_free_heap_size());
    printf("Mem info:\r\n");
    system_print_meminfo();
    printf("\r\n");
    //printf("SDK version:%s\n", system_get_sdk_version());

    /* need to set opmode before you set config */
    wifi_set_opmode(STATION_MODE);

    {
        struct station_config *config = (struct station_config *)zalloc(sizeof(struct station_config));
        sprintf(config->ssid, WIFI_SSID);
        sprintf(config->password, WIFI_PWD);

        /* need to sure that you are in station mode first,
         * otherwise it will be failed. */
        wifi_station_set_config(config);
        free(config);
    }

    xTaskCreate(task3, "tsk3", 256, NULL, 2, NULL);
}


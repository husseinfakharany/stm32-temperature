/*
 * Copyright (C) 2014 Freie Universität Berlin
 *               2018 Inria
 *
 * This file is subject to the terms and conditions of the GNU Lesser
 * General Public License v2.1. See the file LICENSE in the top level
 * directory for more details.
 */

/**
 * @ingroup tests
 * @{
 *
 * @file
 * @brief       Test application for the LPS331AP/LPS25HB/LPS22HB/LPS22HH
 *              pressure sensor
 *
 * @author      Hauke Petersen <hauke.petersen@fu-berlin.de>
 * @author      Alexandre Abadie <alexandre.abadie@inria.fr>
 *
 * @}
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "lpsxxx.h"
#include "lpsxxx_params.h"
#include "semtech_loramac.h"
#include "fmt.h"
#include "periph/pm.h"
#include "periph/rtc.h"
#include "periph/wdt.h"
#include "ztimer.h"
#include "ztimer/periodic.h"

extern semtech_loramac_t loramac;

lpsxxx_t dev;
uint16_t measure=1;

void reverse(char str[], int length);
char* citoa(int num, char* str, int base);
static void cb_rtc_puts(void *arg);
static int set_mode_with_alarm(int mode, int duration);

int main(void)
{
    wdt_setup_reboot(0, 60000);
    wdt_start();

    printf("Test application for %s pressure sensor\n\n", LPSXXX_SAUL_NAME);
    printf("Initializing %s sensor\n", LPSXXX_SAUL_NAME);
    if (lpsxxx_init(&dev, &lpsxxx_params[0]) != LPSXXX_OK)
    {
        puts("Initialization failed");
        return 1;
    }
    lpsxxx_enable(&dev);

#ifdef MODULE_PERIPH_EEPROM
    semtech_loramac_erase_config();
#endif

    // loramac init

    // loramac set deveui CAFEBABEBD9E8f66
    uint8_t deveui[LORAMAC_DEVEUI_LEN];
    fmt_hex_bytes(deveui, "CAFEBABEBD9E8f66");
    semtech_loramac_set_deveui(&loramac, deveui);

    // loramac set appeui CAFEBABE00000000
    uint8_t appeui[LORAMAC_DEVEUI_LEN];
    fmt_hex_bytes(appeui, "CAFEBABE00000000");
    semtech_loramac_set_appeui(&loramac, appeui);

    // loramac set appkey 26662fc6d355d4d02a2eea550402d131
    uint8_t appkey[LORAMAC_APPKEY_LEN];
    fmt_hex_bytes(appkey, "26662fc6d355d4d02a2eea550402d131");
    semtech_loramac_set_appkey(&loramac, appkey);

    // loramac set dr 5
    semtech_loramac_set_dr(&loramac, 5);

    // loramac set adr on
    semtech_loramac_set_adr(&loramac, true);

#ifdef MODULE_PERIPH_EEPROM
    semtech_loramac_save_config(&loramac);
#endif

    // loramac join otaa
    switch (semtech_loramac_join(&loramac, LORAMAC_JOIN_OTAA))
    {
    case SEMTECH_LORAMAC_DUTYCYCLE_RESTRICTED:
        puts("Cannot join: dutycycle restriction");
        return 1;
    case SEMTECH_LORAMAC_BUSY:
        puts("Cannot join: mac is busy");
        return 1;
    case SEMTECH_LORAMAC_JOIN_FAILED:
        puts("Join procedure failed!");
        return 1;
    case SEMTECH_LORAMAC_ALREADY_JOINED:
        puts("Warning: already joined!");
        return 1;
    case SEMTECH_LORAMAC_JOIN_SUCCEEDED:
        puts("Join procedure succeeded!");
        break;
    default: /* should not happen */
        break;
    }

    wdt_kick();
    printf("Delaying Whatchdog\n");

    while (1)
    {
    uint16_t pres;
    int16_t temp;

    lpsxxx_read_temp(&dev, &temp);
    lpsxxx_read_pres(&dev, &pres);

    uint16_t temp_abs = temp / 100;
    temp -= temp_abs * 100;

    // do loramac send
    char payload[50];
    char buffer[5];
    strcpy(payload, "{\"pressure_value\": ");
    citoa(pres, buffer, 10);
    strcat(payload, buffer);
    strcat(payload, ", \"temperature\": ");
    citoa(temp_abs, buffer, 10);
    strcat(payload, buffer);
    strcat(payload, ".");
    citoa(temp, buffer, 10);
    strcat(payload, buffer);
    strcat(payload, "}");
    printf("%s\n", payload);

    semtech_loramac_set_tx_mode(&loramac, CONFIG_LORAMAC_DEFAULT_TX_MODE); //LORAMAC_TX_UNCNF
    semtech_loramac_set_tx_port(&loramac, CONFIG_LORAMAC_DEFAULT_TX_PORT);

    switch (semtech_loramac_send(&loramac, (uint8_t *)payload, strlen(payload)))
    {
    case SEMTECH_LORAMAC_NOT_JOINED:
        puts("Cannot send: not joined");
        break;

    case SEMTECH_LORAMAC_DUTYCYCLE_RESTRICTED:
        puts("Cannot send: dutycycle restriction");
        break;

    case SEMTECH_LORAMAC_BUSY:
        puts("Cannot send: MAC is busy");
        break;

    case SEMTECH_LORAMAC_TX_ERROR:
        puts("Cannot send: error");
        break;

    case SEMTECH_LORAMAC_TX_CNF_FAILED:
        puts("Fail to send: no ACK received");
        break;

    default:
        puts("Message sent with success");
    }

    wdt_kick();
    printf("Delaying Whatchdog\n");

    set_mode_with_alarm(1,30);
    }

    return 0;
}

// A utility function to reverse a string
void reverse(char str[], int length)
{
    int start = 0;
    int end = length - 1;
    while (start < end) {
        char temp = str[start];
        str[start] = str[end];
        str[end] = temp;
        end--;
        start++;
    }
}
// Implementation of citoa()
char* citoa(int num, char* str, int base)
{
    int i = 0;
    bool isNegative = false;
 
    /* Handle 0 explicitly, otherwise empty string is
     * printed for 0 */
    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return str;
    }
 
    // In standard itoa(), negative numbers are handled
    // only with base 10. Otherwise numbers are
    // considered unsigned.
    if (num < 0 && base == 10) {
        isNegative = true;
        num = -num;
    }
 
    // Process individual digits
    while (num != 0) {
        int rem = num % base;
        str[i++] = (rem > 9) ? (rem - 10) + 'a' : rem + '0';
        num = num / base;
    }
 
    // If number is negative, append '-'
    if (isNegative)
        str[i++] = '-';
 
    str[i] = '\0'; // Append string terminator
 
    // Reverse the string
    reverse(str, i);
 
    return str;
}

static void cb_rtc_puts(void *arg)
{
    puts(arg);
}

static int set_mode_with_alarm(int mode, int duration)
{

    printf("Setting power mode %d for %d seconds.\n", mode, duration);
    fflush(stdout);

    struct tm time;

    rtc_get_time(&time);
    time.tm_sec += duration;
    rtc_set_alarm(&time, cb_rtc_puts, "The alarm rang");

    pm_set(mode);

    return 0;
}

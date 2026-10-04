/*
 * Copyright (c) 2026 by Bert Laverman. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// Arbitration test: wait for a rising edge on GP2 (shared trigger line), then immediately send a
// 12 byte Hello message to 0x0a as I2C master on i2c1 (GP14 = SDA, GP15 = SCL).
// The registers are driven directly, because the SDK clears TX_ABRT_SOURCE on a failed write, which would
// hide an ARB_LOST. Every Pico sends its own board id, so the messages differ after the address byte.
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/unique_id.h"
#include "hardware/i2c.h"

#define TRIGGER_PIN 2
#define TARGET      0x0a
#ifndef DELAY_US
#define DELAY_US 0      // wait this long after the trigger before sending (set per Pico at build time)
#endif

int main(void) {
    stdio_init_all();

    i2c_init(i2c1, 100000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    i2c1->hw->enable = 0;
    i2c1->hw->tar = TARGET;
    i2c1->hw->enable = 1;

    gpio_init(TRIGGER_PIN);
    gpio_set_dir(TRIGGER_PIN, GPIO_IN);
    gpio_pull_down(TRIGGER_PIN);

    // Hello message: header (command, length, sender, checksum) followed by our 8 byte board id.
    pico_unique_board_id_t id;
    pico_get_unique_board_id(&id);
    uint8_t msg[12];
    uint8_t checksum = 0;
    for (int i = 0; i < 8; i++) { msg[4 + i] = id.id[i]; checksum ^= id.id[i]; }
    msg[0] = 0x00; msg[1] = 8; msg[2] = 0x00; msg[3] = checksum;

    sleep_ms(3000);
    printf("collision test, board id %02x%02x%02x%02x-%02x%02x%02x%02x, trigger on GP%d, target 0x%02x\n",
           id.id[0], id.id[1], id.id[2], id.id[3], id.id[4], id.id[5], id.id[6], id.id[7], TRIGGER_PIN, TARGET);

    unsigned trial = 0;
    absolute_time_t next_beat = make_timeout_time_ms(5000);
    while (true) {
        // wait for the line to be low, then for the rising edge
        while (gpio_get(TRIGGER_PIN)) { tight_loop_contents(); }
        while (!gpio_get(TRIGGER_PIN)) {
            if (absolute_time_diff_us(next_beat, get_absolute_time()) >= 0) {
                printf("armed, %u trial(s) so far\n", trial);
                next_beat = make_timeout_time_ms(5000);
            }
        }
        uint32_t t0 = time_us_32();
        if (DELAY_US > 0) { busy_wait_us_32(DELAY_US); }

        // clear anything left from before, then push the whole message into the TX FIFO
        (void)i2c1->hw->clr_tx_abrt;
        for (int i = 0; i < 12; i++) {
            i2c1->hw->data_cmd = msg[i] | (i == 11 ? I2C_IC_DATA_CMD_STOP_BITS : 0);
        }

        // done when the master went idle with an empty FIFO, or when the transfer was aborted
        uint32_t abrt = 0;
        bool timeout = false;
        while (true) {
            if (i2c1->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS) {
                abrt = i2c1->hw->tx_abrt_source;
                (void)i2c1->hw->clr_tx_abrt;
                break;
            }
            uint32_t st = i2c1->hw->status;
            if ((st & I2C_IC_STATUS_TFE_BITS) && !(st & I2C_IC_STATUS_MST_ACTIVITY_BITS)) { break; }
            if (time_us_32() - t0 > 20000) { timeout = true; break; }
        }
        if (!abrt && !timeout) {
            busy_wait_us_32(20);
            if (i2c1->hw->raw_intr_stat & I2C_IC_RAW_INTR_STAT_TX_ABRT_BITS) {
                abrt = i2c1->hw->tx_abrt_source;
                (void)i2c1->hw->clr_tx_abrt;
            }
        }
        uint32_t t1 = time_us_32() - t0;
        trial++;

        const char* outcome = timeout ? "TIMEOUT"
                            : (abrt & I2C_IC_TX_ABRT_SOURCE_ARB_LOST_BITS) ? "ARB_LOST"
                            : (abrt & I2C_IC_TX_ABRT_SOURCE_ABRT_7B_ADDR_NOACK_BITS) ? "NOACK_ADDRESS"
                            : (abrt & I2C_IC_TX_ABRT_SOURCE_ABRT_TXDATA_NOACK_BITS) ? "NOACK_DATA"
                            : abrt ? "ABORT_OTHER" : "SENT";
        printf("TRIAL %u: %s abrt=0x%04x time=%u us (delay %d us)\n", trial, outcome, (unsigned)abrt, (unsigned)t1, DELAY_US);
        sleep_ms(300);                       // let the bus settle, and the controller answer
        next_beat = make_timeout_time_ms(5000);
    }
}

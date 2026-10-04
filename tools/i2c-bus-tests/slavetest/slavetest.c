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

#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

// Minimal I2C slave on i2c1 (GP14 = SDA, GP15 = SCL), address 0x61, polling only.
int main(void) {
    stdio_init_all();
    i2c_init(i2c1, 100000);
    gpio_set_function(14, GPIO_FUNC_I2C);
    gpio_set_function(15, GPIO_FUNC_I2C);
    gpio_pull_up(14);
    gpio_pull_up(15);
    i2c_set_slave_mode(i2c1, true, 0x61);

    unsigned received = 0, starts = 0, stops = 0, activity_seen = 0; uint32_t seen = 0;
    absolute_time_t next = get_absolute_time();
    while (true) {
        while (i2c_get_read_available(i2c1) > 0) {
            uint8_t b;
            i2c_read_raw_blocking(i2c1, &b, 1);
            printf("rx 0x%02x\n", b);
            received++;
        }
        if (i2c1->hw->intr_stat & I2C_IC_INTR_STAT_R_RD_REQ_BITS) {   // a master wants to read: answer 0xA5
            i2c_write_raw_blocking(i2c1, (const uint8_t[]){ 0xA5 }, 1);
            (void)i2c1->hw->clr_rd_req;
            printf("read request answered\n");
        }
        uint32_t raw = i2c1->hw->raw_intr_stat;
        seen |= raw;
        if (raw & I2C_IC_RAW_INTR_STAT_START_DET_BITS) { (void)i2c1->hw->clr_start_det; starts++; }
        if (raw & I2C_IC_RAW_INTR_STAT_STOP_DET_BITS)  { (void)i2c1->hw->clr_stop_det;  stops++; }
        if (i2c1->hw->status & I2C_IC_STATUS_ACTIVITY_BITS) activity_seen++;
        if (absolute_time_diff_us(next, get_absolute_time()) >= 0) {
            printf("alive: rx=%u starts=%u stops=%u activity=%u con=0x%03x sar=0x%02x seen=0x%04x status=0x%02x abrt=0x%x\n", received, starts, stops, activity_seen, (unsigned)i2c1->hw->con, (unsigned)i2c1->hw->sar, (unsigned)seen, (unsigned)i2c1->hw->status, (unsigned)i2c1->hw->tx_abrt_source);
            next = make_timeout_time_ms(1000);
        }
        sleep_ms(1);
    }
}

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

// Report the level of GP14 (SDA) and GP15 (SCL) whenever one of them changes, and once per second.
int main(void) {
    stdio_init_all();
    const uint pins[2] = { 14, 15 };
    for (int i = 0; i < 2; i++) {
        gpio_init(pins[i]);
        gpio_set_dir(pins[i], GPIO_IN);
        gpio_disable_pulls(pins[i]);        // no pull: any level must come from the Zero
    }
    int last = -1;
    absolute_time_t next = get_absolute_time();
    while (true) {
        int now = (gpio_get(14) << 1) | gpio_get(15);   // bit1 = SDA, bit0 = SCL
        if (now != last || absolute_time_diff_us(next, get_absolute_time()) >= 0) {
            printf("SDA(GP14)=%d SCL(GP15)=%d\n", (now >> 1) & 1, now & 1);
            last = now;
            next = make_timeout_time_ms(1000);
        }
        sleep_ms(20);
    }
}

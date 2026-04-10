#pragma once
/*
 * Copyright (c) 2025 by Bert Laverman. All Rights Reserved.
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


#if !defined(HAVE_SPI)
#error "SPI support is required for the SSD1305 OLED driver"
#endif


#include <format>

#include <interfaces/spi.hpp>
#include <devices/spi-device.hpp>

namespace nl::rakis::raspberrypi::devices {

template <class SpiDevice, class SpiClass, unsigned MaxWidth, unsigned MaxHeight, unsigned BitsPerPixel = 1>
class SPI7PinDisplay : public SPIDevice<SpiClass> {
    int reset_{ interfaces::NO_PIN };      // Reset pin
    int dc_{ interfaces::NO_PIN };         // Command/data pin
    int bl_{ interfaces::NO_PIN };         // Backlight pin (optional)
    bool blActiveHigh_{ false };           // true = BL on when HIGH; false = BL on when LOW (common for TFT panels)
    bool dcState_{ false };

    static constexpr unsigned maxWidth_{ MaxWidth };
    static constexpr unsigned maxHeight_{ MaxHeight };

    const unsigned width_;
    const unsigned height_;

    std::vector<uint8_t> buffer_;
    bool dirty_{ false };

protected:
    void dirty() noexcept { dirty_ = true; }
    void clean() noexcept { dirty_ = false; }
    bool isDirty() const noexcept { return dirty_; }

    std::vector<uint8_t>& buffer() noexcept { return buffer_; }
    const std::vector<uint8_t>& buffer() const noexcept { return buffer_; }

public:
    SPI7PinDisplay(SpiClass& spi, int reset, int dc, int bl = interfaces::NO_PIN)
        : SPIDevice<SpiClass>(spi)
        , reset_{ reset }, dc_{ dc }, bl_{ bl }
        , width_{ MaxWidth }, height_{ MaxHeight }
        , buffer_((width_*height_*BitsPerPixel)/8, 0)
    { }
    SPI7PinDisplay(SpiClass& spi, int reset, int dc, unsigned width, unsigned height, int bl = interfaces::NO_PIN)
        : SPIDevice<SpiClass>(spi)
        , reset_{ reset }, dc_{ dc }, bl_{ bl }
        , width_{ width }, height_{ height }
        , buffer_((width_*height_*BitsPerPixel)/8, 0)
    { }
    ~SPI7PinDisplay() = default;

    SPI7PinDisplay(const SPI7PinDisplay&) = default;
    SPI7PinDisplay(SPI7PinDisplay&&) = default;
    SPI7PinDisplay& operator=(const SPI7PinDisplay&) = default;
    SPI7PinDisplay& operator=(SPI7PinDisplay&&) = default;

    static consteval unsigned maxWidth() { return maxWidth_; }
    static consteval unsigned maxHeight() { return maxHeight_; }
    static consteval unsigned bitsPerPixel() { return BitsPerPixel; }

    unsigned width() const noexcept { return width_; }
    unsigned height() const noexcept { return height_; }

    int resetPin() const noexcept { return reset_; }
    void resetPin(int pin) { reset_ = pin; }

    int dcPin() const noexcept { return dc_; }
    void dcPin(int pin) { dc_ = pin; }

    int blPin() const noexcept { return bl_; }
    void blPin(int pin) { bl_ = pin; }

    /** Control whether the backlight is active-high (true) or active-low (false, the default for most TFT panels). */
    bool blActiveHigh() const noexcept { return blActiveHigh_; }
    void blActiveHigh(bool v) noexcept { blActiveHigh_ = v; }

    void dcMode(bool state, bool force =false) {
        if ((dcState_ != state) || force) {
            RaspberryPi::gpio().set(dc_, !state);
            dcState_ = state;
        }
    }

    void setCommand(bool force =false) { dcMode(true, force); }
    void setData(bool force =false) { dcMode(false, force); }

    /** Turn the backlight on. No-op if no BL pin was configured. */
    void backlightOn() {
        if (bl_ != interfaces::NO_PIN) {
            if (this->verbose()) {
                this->log("Turning backlight on.");
            }
            RaspberryPi::gpio().set(bl_, blActiveHigh_);
        }
        else if (this->verbose()) {
            this->log("No backlight pin configured; cannot turn backlight on.");
        }
    }

    /** Turn the backlight off. No-op if no BL pin was configured. */
    void backlightOff() {
        if (bl_ != interfaces::NO_PIN) {
            if (this->verbose()) {
                this->log("Turning backlight off.");
            }
            RaspberryPi::gpio().set(bl_, !blActiveHigh_);
        }
        else if (this->verbose()) {
            this->log("No backlight pin configured; cannot turn backlight off.");
        }
    }

    /** Set the backlight state. No-op if no BL pin was configured. */
    void backlight(bool on) { if (on) backlightOn(); else backlightOff(); }

    void reset() {
        auto& gpio = RaspberryPi::gpio();

        if (gpio.available(reset_)) {
            gpio.init(reset_);
            gpio.setForOutput(reset_);
            gpio.setPullUp(reset_);
            gpio.set(reset_, true);
            this->log(std::format("Reset pin for display is GPIO {}.", reset_));
        }
        if (gpio.available(dc_)) {
            gpio.init(dc_);
            gpio.setForOutput(dc_);
            this->log(std::format("Data/Command pin for display is GPIO {}.", dc_));
        }
        if (bl_ != interfaces::NO_PIN) {
            gpio.init(bl_);
            gpio.setForOutput(bl_);
            gpio.set(bl_, !blActiveHigh_);  // backlight off until display is ready
            this->log(std::format("Backlight pin for display is GPIO {}.", bl_));
        }

        if (this->verbose()) {
            this->log("Resetting controller.");
        }
        setCommand(true);  // Force to command mode.

        RaspberryPi::instance().sleepMs(200);
        RaspberryPi::gpio().set(reset_, false);
        RaspberryPi::instance().sleepMs(200);
        RaspberryPi::gpio().set(reset_, true);
    }

    void command(uint8_t cmd) {
        setCommand();
        std::array<uint8_t, 1> data{{ cmd }};
        this->interface().write(std::span<uint8_t>{data.data(), data.size()});
    }
    void command(uint8_t cmd, uint8_t parm1) {
        setCommand();
        std::array<uint8_t, 2> data{{ cmd, parm1 }};
        this->interface().write(std::span<uint8_t>{data.data(), data.size()});
    }
    void command(uint8_t cmd, uint8_t parm1, uint8_t parm2) {
        setCommand();
        std::array<uint8_t, 3> data{{ cmd, parm1, parm2 }};
        this->interface().write(std::span<uint8_t>{data.data(), data.size()});
    }
    void command(uint8_t cmd, uint8_t parm1, uint8_t parm2, uint8_t parm3) {
        setCommand();
        std::array<uint8_t, 4> data{{ cmd, parm1, parm2, parm3 }};
        this->interface().write(std::span<uint8_t>{data.data(), data.size()});
    }
    void command(uint8_t cmd, uint8_t parm1, uint8_t parm2, uint8_t parm3, uint8_t parm4) {
        setCommand();
        std::array<uint8_t, 5> data{{ cmd, parm1, parm2, parm3, parm4 }};
        this->interface().write(std::span<uint8_t>{data.data(), data.size()});
    }

    void data(std::span<uint8_t> buffer) { setData(); this->interface().write(std::span<uint8_t>{buffer.data(), buffer.size()}); }

};

} // nl::rakis::raspberrypi::devices
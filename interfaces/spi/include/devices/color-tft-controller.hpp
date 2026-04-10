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


#include <algorithm>
#include <span>
#include <vector>

#include <devices/spi-7pin-display.hpp>


namespace nl::rakis::raspberrypi::devices {


/**
 * ColorTFTController is a generic base for RGB565 color TFT controllers
 * connected via a 7-pin SPI interface (CS, SCLK, MOSI, DC, RST).
 *
 * The pixel buffer stores one pixel per two bytes in big-endian RGB565 format:
 * bits [15:11] = red (5), [10:5] = green (6), [4:0] = blue (5).
 *
 * Derived classes must implement `prepareWrite(x0, y0, x1, y1)` to set the
 * controller's write window (e.g. CASET + RASET + RAMWR for ST7789-family)
 * before `sendBuffer()` streams pixel data.
 *
 * @param SpiDevice The concrete controller class. (CRTP)
 * @param SpiClass  The concrete SPI interface class.
 * @param MaxWidth  Maximum display width supported by the controller.
 * @param MaxHeight Maximum display height supported by the controller.
 */
template <class SpiDevice, class SpiClass, unsigned MaxWidth, unsigned MaxHeight>
class ColorTFTController : public SPI7PinDisplay<SpiDevice, SpiClass, MaxWidth, MaxHeight, 16>
{
    bool sendImmediately_{ false };

protected:
    /**
     * Write a single pixel directly to the buffer without marking dirty or flushing.
     * Out-of-bounds coordinates are silently ignored.
     *
     * @param x     Horizontal pixel coordinate.
     * @param y     Vertical pixel coordinate.
     * @param color RGB565 colour value.
     */
    void plotPixel(unsigned x, unsigned y, uint16_t color) noexcept {
        if (x < this->width() && y < this->height()) {
            const unsigned idx = 2 * (y * this->width() + x);
            this->buffer()[idx]     = static_cast<uint8_t>(color >> 8);
            this->buffer()[idx + 1] = static_cast<uint8_t>(color & 0xFF);
        }
    }

    /**
     * Write a horizontal run of pixels to the buffer, clipping to display bounds.
     * Does not mark dirty or flush.
     *
     * @param x     Left edge of the span (may be negative; clipped to display).
     * @param y     Row index (silently ignored if out of bounds).
     * @param len   Number of pixels to write.
     * @param color RGB565 colour value.
     */
    void plotHSpan(int x, unsigned y, unsigned len, uint16_t color) noexcept {
        if (y >= this->height() || len == 0) return;
        if (x < 0) {
            if (static_cast<unsigned>(-x) >= len) return;
            len -= static_cast<unsigned>(-x);
            x = 0;
        }
        const unsigned ux = static_cast<unsigned>(x);
        if (ux >= this->width()) return;
        const unsigned xEnd = ux + std::min(len, this->width() - ux);
        const uint8_t hi = static_cast<uint8_t>(color >> 8);
        const uint8_t lo = static_cast<uint8_t>(color & 0xFF);
        const unsigned rowOff = 2 * y * this->width();
        for (unsigned cx = ux; cx < xEnd; ++cx) {
            this->buffer()[rowOff + 2 * cx]     = hi;
            this->buffer()[rowOff + 2 * cx + 1] = lo;
        }
    }

public:
    ColorTFTController(SpiClass& spi, int reset, int dc, int bl = interfaces::NO_PIN)
        : SPI7PinDisplay<SpiDevice, SpiClass, MaxWidth, MaxHeight, 16>(spi, reset, dc, bl)
    {}
    ColorTFTController(SpiClass& spi, int reset, int dc, unsigned width, unsigned height, int bl = interfaces::NO_PIN)
        : SPI7PinDisplay<SpiDevice, SpiClass, MaxWidth, MaxHeight, 16>(spi, reset, dc, width, height, bl)
    {}
    ~ColorTFTController() = default;

    ColorTFTController(const ColorTFTController&) = delete;
    ColorTFTController(ColorTFTController&&) = default;
    ColorTFTController& operator=(const ColorTFTController&) = delete;
    ColorTFTController& operator=(ColorTFTController&&) = default;


    /**
     * Pack red, green, and blue components into an RGB565 value.
     *
     * @param r Red component, 0–255.
     * @param g Green component, 0–255.
     * @param b Blue component, 0–255.
     * @return  Packed RGB565 value.
     */
    static constexpr uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) noexcept {
        return static_cast<uint16_t>(((r & 0xF8u) << 8) | ((g & 0xFCu) << 3) | (b >> 3));
    }

    /**
     * When set to true, any call that modifies the pixel buffer immediately
     * flushes the entire buffer to the display.
     */
    void sendImmediately(bool doIt = true) noexcept { sendImmediately_ = doIt; }

    /**
     * Returns whether immediate-send mode is active.
     */
    bool isSendImmediately() const noexcept { return sendImmediately_; }


    // -------------------------------------------------------------------------
    // Buffer operations
    // -------------------------------------------------------------------------

    /**
     * Flush the pixel buffer to the display if it has been modified.
     * Delegates to the derived class's `prepareWrite(x0, y0, x1, y1)` via CRTP
     * to set up the write window, then streams all pixel data.
     */
    void sendBuffer() {
        if (this->isDirty()) {
            static_cast<SpiDevice*>(this)->prepareWrite(0, 0, this->width() - 1, this->height() - 1);
            this->data(this->buffer());
            this->clean();
        }
    }

    /**
     * Fill the entire pixel buffer with the given colour and mark it dirty.
     * If immediate-send mode is active, also flushes to the display.
     *
     * @param color RGB565 fill colour (default: black).
     */
    void clear(uint16_t color = 0) {
        if (color == 0) {
            std::fill(this->buffer().begin(), this->buffer().end(), uint8_t{ 0 });
        } else {
            const uint8_t hi = static_cast<uint8_t>(color >> 8);
            const uint8_t lo = static_cast<uint8_t>(color & 0xFF);
            for (size_t i = 0; i < this->buffer().size(); i += 2) {
                this->buffer()[i]     = hi;
                this->buffer()[i + 1] = lo;
            }
        }
        this->dirty();
        if (isSendImmediately()) { sendBuffer(); }
    }

    /**
     * Set a single pixel to an RGB565 colour.
     *
     * @param x     Horizontal pixel coordinate, 0 to width-1.
     * @param y     Vertical pixel coordinate, 0 to height-1.
     * @param color RGB565 colour value.
     */
    void set(unsigned x, unsigned y, uint16_t color) {
        if (x < this->width() && y < this->height()) {
            plotPixel(x, y, color);
            this->dirty();
            if (isSendImmediately()) { sendBuffer(); }
        }
    }

    /**
     * Set a single pixel using red, green, and blue components.
     *
     * @param x Horizontal pixel coordinate, 0 to width-1.
     * @param y Vertical pixel coordinate, 0 to height-1.
     * @param r Red component, 0–255.
     * @param g Green component, 0–255.
     * @param b Blue component, 0–255.
     */
    void set(unsigned x, unsigned y, uint8_t r, uint8_t g, uint8_t b) {
        set(x, y, rgb565(r, g, b));
    }

    /**
     * Read back the RGB565 colour stored in the pixel buffer.
     *
     * @param x Horizontal pixel coordinate, 0 to width-1.
     * @param y Vertical pixel coordinate, 0 to height-1.
     * @return  RGB565 colour, or 0 if the coordinate is out of bounds.
     */
    uint16_t get(unsigned x, unsigned y) const noexcept {
        if (x < this->width() && y < this->height()) {
            const unsigned idx = 2 * (y * this->width() + x);
            return static_cast<uint16_t>((this->buffer()[idx] << 8) | this->buffer()[idx + 1]);
        }
        return 0;
    }


    // -------------------------------------------------------------------------
    // Drawing primitives
    // -------------------------------------------------------------------------

    /**
     * Draw a straight line between two points using Bresenham's algorithm.
     *
     * @param x0    Start x coordinate.
     * @param y0    Start y coordinate.
     * @param x1    End x coordinate.
     * @param y1    End y coordinate.
     * @param color RGB565 colour.
     */
    void line(unsigned x0, unsigned y0, unsigned x1, unsigned y1, uint16_t color) {
        int dx = (int)x1 - (int)x0;
        int dy = (int)y1 - (int)y0;
        int stepx = (dx >= 0) ? 1 : -1;
        int stepy = (dy >= 0) ? 1 : -1;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        int err = dx - dy;
        int cx = (int)x0, cy = (int)y0;
        for (;;) {
            plotPixel((unsigned)cx, (unsigned)cy, color);
            if (cx == (int)x1 && cy == (int)y1) break;
            int e2 = 2 * err;
            if (e2 > -dy) { err -= dy; cx += stepx; }
            if (e2 <  dx) { err += dx; cy += stepy; }
        }
        this->dirty();
        if (isSendImmediately()) sendBuffer();
    }

    /**
     * Draw the outline of an axis-aligned rectangle.
     *
     * @param x     Left edge.
     * @param y     Top edge.
     * @param w     Width in pixels.
     * @param h     Height in pixels.
     * @param color RGB565 colour.
     */
    void rectangle(unsigned x, unsigned y, unsigned w, unsigned h, uint16_t color) {
        if (w == 0 || h == 0) return;
        plotHSpan((int)x, y, w, color);
        if (h > 1) plotHSpan((int)x, y + h - 1, w, color);
        for (unsigned row = y + 1; row + 1 < y + h; ++row) {
            plotPixel(x, row, color);
            plotPixel(x + w - 1, row, color);
        }
        this->dirty();
        if (isSendImmediately()) sendBuffer();
    }

    /**
     * Draw a filled axis-aligned rectangle.
     *
     * @param x     Left edge.
     * @param y     Top edge.
     * @param w     Width in pixels.
     * @param h     Height in pixels.
     * @param color RGB565 colour.
     */
    void filledRectangle(unsigned x, unsigned y, unsigned w, unsigned h, uint16_t color) {
        for (unsigned row = y; row < y + h; ++row)
            plotHSpan((int)x, row, w, color);
        this->dirty();
        if (isSendImmediately()) sendBuffer();
    }

    /**
     * Draw the outline of a square.
     *
     * @param x     Left edge.
     * @param y     Top edge.
     * @param size  Side length in pixels.
     * @param color RGB565 colour.
     */
    void square(unsigned x, unsigned y, unsigned size, uint16_t color) {
        rectangle(x, y, size, size, color);
    }

    /**
     * Draw a filled square.
     *
     * @param x     Left edge.
     * @param y     Top edge.
     * @param size  Side length in pixels.
     * @param color RGB565 colour.
     */
    void filledSquare(unsigned x, unsigned y, unsigned size, uint16_t color) {
        filledRectangle(x, y, size, size, color);
    }

    /**
     * Draw an axis-aligned ellipse outline using the Bresenham midpoint algorithm.
     * Pixels outside the display bounds are silently clipped.
     *
     * @param xctr  Horizontal centre coordinate.
     * @param yctr  Vertical centre coordinate.
     * @param rx    Horizontal semi-axis in pixels.
     * @param ry    Vertical semi-axis in pixels.
     * @param color RGB565 colour.
     */
    void ellipse(unsigned xctr, unsigned yctr, unsigned rx, unsigned ry, uint16_t color) {
        if (rx == 0 && ry == 0) { set(xctr, yctr, color); return; }
        long a = (long)rx, b = (long)ry;
        long a2 = a * a, b2 = b * b;

        long x = 0, y = b;
        long sigma = 2 * b2 + a2 * (1 - 2 * b);
        while (b2 * x <= a2 * y) {
            plotPixel((unsigned)((int)xctr + (int)x), (unsigned)((int)yctr + (int)y), color);
            plotPixel((unsigned)((int)xctr - (int)x), (unsigned)((int)yctr + (int)y), color);
            plotPixel((unsigned)((int)xctr + (int)x), (unsigned)((int)yctr - (int)y), color);
            plotPixel((unsigned)((int)xctr - (int)x), (unsigned)((int)yctr - (int)y), color);
            if (sigma >= 0) { sigma += 4 * a2 * (1 - y); --y; }
            sigma += b2 * (4 * x + 6);
            ++x;
        }

        x = a; y = 0;
        sigma = 2 * a2 + b2 * (1 - 2 * a);
        while (a2 * y <= b2 * x) {
            plotPixel((unsigned)((int)xctr + (int)x), (unsigned)((int)yctr + (int)y), color);
            plotPixel((unsigned)((int)xctr - (int)x), (unsigned)((int)yctr + (int)y), color);
            plotPixel((unsigned)((int)xctr + (int)x), (unsigned)((int)yctr - (int)y), color);
            plotPixel((unsigned)((int)xctr - (int)x), (unsigned)((int)yctr - (int)y), color);
            if (sigma >= 0) { sigma += 4 * b2 * (1 - x); --x; }
            sigma += a2 * (4 * y + 6);
            ++y;
        }

        this->dirty();
        if (isSendImmediately()) sendBuffer();
    }

    /**
     * Draw a filled axis-aligned ellipse.
     *
     * @param xctr  Horizontal centre coordinate.
     * @param yctr  Vertical centre coordinate.
     * @param rx    Horizontal semi-axis in pixels.
     * @param ry    Vertical semi-axis in pixels.
     * @param color RGB565 colour.
     */
    void filledEllipse(unsigned xctr, unsigned yctr, unsigned rx, unsigned ry, uint16_t color) {
        if (rx == 0 && ry == 0) { set(xctr, yctr, color); return; }
        long a = (long)rx, b = (long)ry;
        long a2 = a * a, b2 = b * b;
        long ex = a;
        for (int dy = 0; dy <= (int)ry; ++dy) {
            while (ex > 0 && ex * ex * b2 + (long)dy * dy * a2 > a2 * b2) --ex;
            plotHSpan((int)xctr - (int)ex, (unsigned)((int)yctr + dy), (unsigned)(2 * ex + 1), color);
            if (dy != 0)
                plotHSpan((int)xctr - (int)ex, (unsigned)((int)yctr - dy), (unsigned)(2 * ex + 1), color);
        }
        this->dirty();
        if (isSendImmediately()) sendBuffer();
    }

    /**
     * Draw a circle outline (shorthand for `ellipse` with equal semi-axes).
     *
     * @param cx    Centre x coordinate.
     * @param cy    Centre y coordinate.
     * @param r     Radius in pixels.
     * @param color RGB565 colour.
     */
    void circle(unsigned cx, unsigned cy, unsigned r, uint16_t color) {
        ellipse(cx, cy, r, r, color);
    }

    /**
     * Draw a filled circle (shorthand for `filledEllipse` with equal semi-axes).
     *
     * @param cx    Centre x coordinate.
     * @param cy    Centre y coordinate.
     * @param r     Radius in pixels.
     * @param color RGB565 colour.
     */
    void filledCircle(unsigned cx, unsigned cy, unsigned r, uint16_t color) {
        filledEllipse(cx, cy, r, r, color);
    }

};

} // namespace nl::rakis::raspberrypi::devices

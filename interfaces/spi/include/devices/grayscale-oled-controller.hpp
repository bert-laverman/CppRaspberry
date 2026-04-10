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
 * GrayscaleOLEDController is a generic base for 4-bit (16-level) grayscale OLED
 * controllers connected via a 7-pin SPI interface (CS, SCLK, MOSI, DC, RST).
 *
 * The pixel buffer stores two pixels per byte: the high nibble holds the left
 * (even-column) pixel and the low nibble holds the right (odd-column) pixel.
 * Grayscale values range from 0 (off) to 15 (maximum brightness).
 *
 * Derived classes must implement `setPageAddress(uint8_t page)` and
 * `setColumnAddress(uint8_t col)` to support the page-by-page flush in
 * `sendBuffer()`.
 *
 * @param SpiDevice The concrete controller class. (CRTP)
 * @param SpiClass  The concrete SPI interface class.
 * @param MaxWidth  Maximum display width supported by the controller.
 * @param MaxHeight Maximum display height supported by the controller.
 */
template <class SpiDevice, class SpiClass, unsigned MaxWidth, unsigned MaxHeight>
class GrayscaleOLEDController : public SPI7PinDisplay<SpiDevice, SpiClass, MaxWidth, MaxHeight, 4>
{
    bool sendImmediately_{ false };

protected:
    /**
     * Write a single pixel directly to the buffer without marking dirty or flushing.
     * Out-of-bounds coordinates are silently ignored.
     */
    void plotPixel(unsigned x, unsigned y, uint8_t gray) noexcept {
        if (x < this->width() && y < this->height()) {
            const unsigned idx = y * (this->width() / 2) + x / 2;
            if (x % 2 == 0)
                this->buffer()[idx] = static_cast<uint8_t>((this->buffer()[idx] & 0x0F) | ((gray & 0x0F) << 4));
            else
                this->buffer()[idx] = static_cast<uint8_t>((this->buffer()[idx] & 0xF0) | (gray & 0x0F));
        }
    }

    /**
     * Write a horizontal run of pixels to the buffer, clipping to display bounds.
     * Two pixels per byte are packed efficiently; odd-aligned starts and ends are
     * handled with individual nibble writes.  Does not mark dirty or flush.
     *
     * @param x   Left edge of the span (may be negative; clipped to display).
     * @param y   Row index (silently ignored if out of bounds).
     * @param len Number of pixels to write.
     * @param gray Grayscale level, 0–15.
     */
    void plotHSpan(int x, unsigned y, unsigned len, uint8_t gray) noexcept {
        if (y >= this->height() || len == 0) return;
        if (x < 0) {
            if (static_cast<unsigned>(-x) >= len) return;
            len -= static_cast<unsigned>(-x);
            x = 0;
        }
        const unsigned ux = static_cast<unsigned>(x);
        if (ux >= this->width()) return;
        const unsigned xEnd = ux + std::min(len, this->width() - ux);
        const uint8_t g = gray & 0x0F;
        const uint8_t pair = static_cast<uint8_t>((g << 4) | g);
        const unsigned rowOff = y * (this->width() / 2);
        unsigned cx = ux;
        if (cx % 2 != 0 && cx < xEnd) {
            auto& b = this->buffer()[rowOff + cx / 2];
            b = static_cast<uint8_t>((b & 0xF0) | g);
            ++cx;
        }
        while (cx + 1 < xEnd) {
            this->buffer()[rowOff + cx / 2] = pair;
            cx += 2;
        }
        if (cx < xEnd) {
            auto& b = this->buffer()[rowOff + cx / 2];
            b = static_cast<uint8_t>((b & 0x0F) | (g << 4));
        }
    }

public:
    GrayscaleOLEDController(SpiClass& spi, int reset, int dc)
        : SPI7PinDisplay<SpiDevice, SpiClass, MaxWidth, MaxHeight, 4>(spi, reset, dc)
    {}
    GrayscaleOLEDController(SpiClass& spi, int reset, int dc, unsigned width, unsigned height)
        : SPI7PinDisplay<SpiDevice, SpiClass, MaxWidth, MaxHeight, 4>(spi, reset, dc, width, height)
    {}
    ~GrayscaleOLEDController() = default;

    GrayscaleOLEDController(const GrayscaleOLEDController&) = delete;
    GrayscaleOLEDController(GrayscaleOLEDController&&) = default;
    GrayscaleOLEDController& operator=(const GrayscaleOLEDController&) = delete;
    GrayscaleOLEDController& operator=(GrayscaleOLEDController&&) = default;


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
     * Uses page addressing: for each of the height/8 pages, delegates to the
     * derived class's `setPageAddress` and `setColumnAddress` via CRTP, then
     * writes 8×width/2 bytes of pixel data.
     */
    void sendBuffer() {
        if (this->isDirty()) {
            const unsigned bytesPerRow = this->width() / 2;
            const unsigned numPages = this->height() / 8;
            for (uint8_t page = 0; page < numPages; ++page) {
                static_cast<SpiDevice*>(this)->setPageAddress(page);
                static_cast<SpiDevice*>(this)->setColumnAddress(0);
                const unsigned startIdx = page * 8 * bytesPerRow;
                this->data(std::span<uint8_t>{ &(this->buffer()[startIdx]), 8 * bytesPerRow });
            }
            this->clean();
        }
    }

    /**
     * Fill the entire pixel buffer with black (all pixels off) and mark it
     * dirty.  If immediate-send mode is active, the cleared buffer is also
     * flushed to the display.
     */
    void clear() {
        std::fill(this->buffer().begin(), this->buffer().end(), 0x00);
        this->dirty();
        if (isSendImmediately()) { sendBuffer(); }
    }

    /**
     * Set a single pixel to the given grayscale level.
     *
     * @param x    Horizontal pixel coordinate, 0 to width-1.
     * @param y    Vertical pixel coordinate, 0 to height-1.
     * @param gray Grayscale level, 0 (off) to 15 (maximum brightness).
     */
    void set(unsigned x, unsigned y, uint8_t gray) {
        if (x < this->width() && y < this->height()) {
            plotPixel(x, y, gray);
            this->dirty();
            if (isSendImmediately()) { sendBuffer(); }
        }
    }

    /**
     * Read back the grayscale level stored in the pixel buffer for a given
     * coordinate.
     *
     * @param x Horizontal pixel coordinate, 0 to width-1.
     * @param y Vertical pixel coordinate, 0 to height-1.
     * @return  Grayscale level 0–15, or 0 if the coordinate is out of bounds.
     */
    uint8_t get(unsigned x, unsigned y) const noexcept {
        if (x < this->width() && y < this->height()) {
            const unsigned idx = y * (this->width() / 2) + x / 2;
            return (x % 2 == 0) ? static_cast<uint8_t>((this->buffer()[idx] >> 4) & 0x0F)
                                 : static_cast<uint8_t>(this->buffer()[idx] & 0x0F);
        }
        return 0;
    }


    // -------------------------------------------------------------------------
    // Drawing primitives
    // -------------------------------------------------------------------------

    /**
     * Draw a straight line between two points using Bresenham's algorithm.
     *
     * @param x0   Start x coordinate.
     * @param y0   Start y coordinate.
     * @param x1   End x coordinate.
     * @param y1   End y coordinate.
     * @param gray Grayscale level, 0–15.
     */
    void line(unsigned x0, unsigned y0, unsigned x1, unsigned y1, uint8_t gray) {
        int dx = (int)x1 - (int)x0;
        int dy = (int)y1 - (int)y0;
        int stepx = (dx >= 0) ? 1 : -1;
        int stepy = (dy >= 0) ? 1 : -1;
        if (dx < 0) dx = -dx;
        if (dy < 0) dy = -dy;
        int err = dx - dy;
        int cx = (int)x0, cy = (int)y0;
        for (;;) {
            plotPixel((unsigned)cx, (unsigned)cy, gray);
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
     * @param x    Left edge.
     * @param y    Top edge.
     * @param w    Width in pixels.
     * @param h    Height in pixels.
     * @param gray Grayscale level, 0–15.
     */
    void rectangle(unsigned x, unsigned y, unsigned w, unsigned h, uint8_t gray) {
        if (w == 0 || h == 0) return;
        plotHSpan((int)x, y, w, gray);
        if (h > 1) plotHSpan((int)x, y + h - 1, w, gray);
        for (unsigned row = y + 1; row + 1 < y + h; ++row) {
            plotPixel(x, row, gray);
            plotPixel(x + w - 1, row, gray);
        }
        this->dirty();
        if (isSendImmediately()) sendBuffer();
    }

    /**
     * Draw a filled axis-aligned rectangle.
     *
     * @param x    Left edge.
     * @param y    Top edge.
     * @param w    Width in pixels.
     * @param h    Height in pixels.
     * @param gray Grayscale level, 0–15.
     */
    void filledRectangle(unsigned x, unsigned y, unsigned w, unsigned h, uint8_t gray) {
        for (unsigned row = y; row < y + h; ++row)
            plotHSpan((int)x, row, w, gray);
        this->dirty();
        if (isSendImmediately()) sendBuffer();
    }

    /**
     * Draw the outline of a square.
     *
     * @param x    Left edge.
     * @param y    Top edge.
     * @param size Side length in pixels.
     * @param gray Grayscale level, 0–15.
     */
    void square(unsigned x, unsigned y, unsigned size, uint8_t gray) {
        rectangle(x, y, size, size, gray);
    }

    /**
     * Draw a filled square.
     *
     * @param x    Left edge.
     * @param y    Top edge.
     * @param size Side length in pixels.
     * @param gray Grayscale level, 0–15.
     */
    void filledSquare(unsigned x, unsigned y, unsigned size, uint8_t gray) {
        filledRectangle(x, y, size, size, gray);
    }

    /**
     * Draw an axis-aligned ellipse outline using the Bresenham midpoint algorithm.
     * Pixels outside the display bounds are silently clipped.
     *
     * @param xctr Horizontal centre coordinate.
     * @param yctr Vertical centre coordinate.
     * @param rx   Horizontal semi-axis in pixels.
     * @param ry   Vertical semi-axis in pixels.
     * @param gray Grayscale level, 0–15.
     */
    void ellipse(unsigned xctr, unsigned yctr, unsigned rx, unsigned ry, uint8_t gray) {
        if (rx == 0 && ry == 0) { set(xctr, yctr, gray); return; }
        long a = (long)rx, b = (long)ry;
        long a2 = a * a, b2 = b * b;

        // Region 1: near the top/bottom poles, x advances faster than y decreases
        long x = 0, y = b;
        long sigma = 2 * b2 + a2 * (1 - 2 * b);
        while (b2 * x <= a2 * y) {
            plotPixel((unsigned)((int)xctr + (int)x), (unsigned)((int)yctr + (int)y), gray);
            plotPixel((unsigned)((int)xctr - (int)x), (unsigned)((int)yctr + (int)y), gray);
            plotPixel((unsigned)((int)xctr + (int)x), (unsigned)((int)yctr - (int)y), gray);
            plotPixel((unsigned)((int)xctr - (int)x), (unsigned)((int)yctr - (int)y), gray);
            if (sigma >= 0) { sigma += 4 * a2 * (1 - y); --y; }
            sigma += b2 * (4 * x + 6);
            ++x;
        }

        // Region 2: near the left/right poles, y advances faster than x decreases
        x = a; y = 0;
        sigma = 2 * a2 + b2 * (1 - 2 * a);
        while (a2 * y <= b2 * x) {
            plotPixel((unsigned)((int)xctr + (int)x), (unsigned)((int)yctr + (int)y), gray);
            plotPixel((unsigned)((int)xctr - (int)x), (unsigned)((int)yctr + (int)y), gray);
            plotPixel((unsigned)((int)xctr + (int)x), (unsigned)((int)yctr - (int)y), gray);
            plotPixel((unsigned)((int)xctr - (int)x), (unsigned)((int)yctr - (int)y), gray);
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
     * Iterates rows outward from the centre; the x-extent shrinks monotonically
     * so the inner walk totals O(rx) steps across all rows, giving O(rx + ry)
     * overall.
     *
     * @param xctr Horizontal centre coordinate.
     * @param yctr Vertical centre coordinate.
     * @param rx   Horizontal semi-axis in pixels.
     * @param ry   Vertical semi-axis in pixels.
     * @param gray Grayscale level, 0–15.
     */
    void filledEllipse(unsigned xctr, unsigned yctr, unsigned rx, unsigned ry, uint8_t gray) {
        if (rx == 0 && ry == 0) { set(xctr, yctr, gray); return; }
        long a = (long)rx, b = (long)ry;
        long a2 = a * a, b2 = b * b;
        long ex = a;
        for (int dy = 0; dy <= (int)ry; ++dy) {
            while (ex > 0 && ex * ex * b2 + (long)dy * dy * a2 > a2 * b2) --ex;
            plotHSpan((int)xctr - (int)ex, (unsigned)((int)yctr + dy), (unsigned)(2 * ex + 1), gray);
            if (dy != 0)
                plotHSpan((int)xctr - (int)ex, (unsigned)((int)yctr - dy), (unsigned)(2 * ex + 1), gray);
        }
        this->dirty();
        if (isSendImmediately()) sendBuffer();
    }

    /**
     * Draw a circle outline (shorthand for `ellipse` with equal semi-axes).
     *
     * @param cx   Centre x coordinate.
     * @param cy   Centre y coordinate.
     * @param r    Radius in pixels.
     * @param gray Grayscale level, 0–15.
     */
    void circle(unsigned cx, unsigned cy, unsigned r, uint8_t gray) {
        ellipse(cx, cy, r, r, gray);
    }

    /**
     * Draw a filled circle (shorthand for `filledEllipse` with equal semi-axes).
     *
     * @param cx   Centre x coordinate.
     * @param cy   Centre y coordinate.
     * @param r    Radius in pixels.
     * @param gray Grayscale level, 0–15.
     */
    void filledCircle(unsigned cx, unsigned cy, unsigned r, uint8_t gray) {
        filledEllipse(cx, cy, r, r, gray);
    }

};

} // namespace nl::rakis::raspberrypi::devices

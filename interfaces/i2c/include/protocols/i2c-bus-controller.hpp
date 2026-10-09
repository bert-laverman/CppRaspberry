#pragma once
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

#include <chrono>
#include <cstdint>
#include <cstring>
#include <format>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <protocols/messages.hpp>
#include <protocols/i2c-device-handler.hpp>


namespace nl::rakis::raspberrypi::protocols {

/**
 * @brief The side of the I2C bus that hands out addresses: the bus controller.
 *
 * Boards on the bus start without an address and answer the controller's Hello with a Hello of their own, which
 * carries their board id. The controller picks an address for each board (the same one every time, for a board that it
 * knows), and sends it in a SetAddress, which is a General Call. A board that has taken over its address confirms it by
 * sending a Hello from that address. Until that happens the assignment is *pending*: the controller sends the SetAddress
 * again after a while, a limited number of times, and keeps the address reserved for that board.
 *
 * A board that has an address announces itself every few seconds, on the Hello of the controller (see I2CDeviceHandler). That
 * is how the controller knows which boards are there, also after it was restarted: onBoardAppeared() and onBoardGone() tell
 * you when that changes.
 *
 * The controller does not know where addresses are stored. Give it the boards that you know with addKnown(), and use
 * onConfirmed() to save an address once a board has confirmed it.
 *
 * Use it like this:
 *
 * @code
 * I2CBusController controller(driver);
 * controller.addKnown(id, address);          // for each board that was saved earlier
 * controller.onConfirmed([](const BoardId& id, uint8_t address) { save(id, address); });
 * controller.registerHandlers();
 * ...
 * while (running) {
 *     controller.tick();                      // sends the Hello once per second, and repeats unconfirmed addresses
 *     driver.processIncoming();
 *     sleep(10 ms);
 * }
 * @endcode
 *
 * @tparam ProtDriver The protocol driver, for instance I2CProtocolDriver<util::MessageQueue>.
 */
template <typename ProtDriver>
class I2CBusController {
public:
    using Clock = std::chrono::steady_clock;
    using ConfirmedCallback = std::function<void(const BoardId& id, uint8_t address)>;
    using BoardCallback = std::function<void(const BoardId& id, uint8_t address)>;
    using Logger = std::function<void(const std::string&)>;

private:
    /**
     * An address we have given out, and for which we have not seen the board announce itself on that address yet.
     */
    struct Pending {
        BoardId boardId;
        uint8_t address;
        unsigned attempts;
        Clock::time_point lastSent;
    };

    ProtDriver& driver_;
    BoardId controllerId_{ .id = ControllerId };
    I2CDeviceHandler<ProtDriver> handler_;

    uint8_t firstAddress_{ 0x61 };
    uint8_t lastAddress_{ 0x77 };           // 0x78-0x7f are reserved
    Clock::duration helloInterval_{ std::chrono::seconds(1) };
    Clock::duration resendAfter_{ std::chrono::milliseconds(100) };
    unsigned maxAttempts_{ 5 };

    std::map<uint64_t, uint8_t> addressById_;
    std::map<uint8_t, uint64_t> idByAddress_;
    std::map<uint64_t, Pending> pending_;
    std::set<uint8_t> online_;
    std::map<uint8_t, Clock::time_point> lastSeen_;
    Clock::duration goneAfter_{ std::chrono::seconds(10) };

    Clock::time_point lastHello_{};
    bool helloSent_{ false };

    ConfirmedCallback onConfirmed_;
    BoardCallback onAppeared_;
    BoardCallback onGone_;
    Logger logger_{ [](const std::string& s) { std::cerr << s; } };

    void log(const std::string& s) const { if (logger_) { logger_(s); } }

    static std::string idString(const BoardId& id) {
        return std::format("{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}",
                           id.bytes[0], id.bytes[1], id.bytes[2], id.bytes[3], id.bytes[4], id.bytes[5], id.bytes[6], id.bytes[7]);
    }

    /**
     * The address for a board that asks for one: the one it had, or else the first one that is free. Returns 0 if
     * there is none left.
     */
    uint8_t addressFor(const BoardId& id) {
        auto it = addressById_.find(id.id);
        if (it != addressById_.end()) {
            log(std::format("- We know this one: It has address 0x{:02x}.\n", it->second));

            return it->second;
        }
        for (unsigned address = firstAddress_; address <= lastAddress_; ++address) {
            if (idByAddress_.find(static_cast<uint8_t>(address)) == idByAddress_.end()) {
                log(std::format("- We'll give this board address 0x{:02x}.\n", address));

                return static_cast<uint8_t>(address);
            }
        }
        log("* No free address left.\n");

        return 0;
    }

    /**
     * A board asks for an address (it has none yet, so it says Hello from 0x00).
     */
    void handleRequest(const BoardId& id) {
        const uint8_t address = addressFor(id);
        if (address == 0) {
            return;
        }
        // Keep the address for this board, whether or not the SetAddress gets through: if it does not, we send it again,
        // and another board must not get the same address in the meantime.
        idByAddress_[address] = id.id;
        addressById_[id.id] = address;
        if (!handler_.sendSetAddress(id, address)) {
            log(std::format("* Failed to send address 0x{:02x}.\n", address));
        }
        pending_[id.id] = Pending{ id, address, 1, Clock::now() };
    }

    /**
     * A board that has an address announces itself.
     */
    void handleAnnouncement(uint8_t sender, const BoardId& id) {
        const bool wasOnline = online_.count(sender) != 0;
        online_.insert(sender);
        lastSeen_[sender] = Clock::now();

        // A board that says it has an address we have not given out (we were restarted, and have no state): keep the
        // address for that board.
        if (addressById_.find(id.id) == addressById_.end()) {
            if (idByAddress_.find(sender) == idByAddress_.end()) {
                addKnown(id, sender);
                log(std::format("- Board {} was not known, and has address 0x{:02x}. We keep it for that board.\n", idString(id), sender));
            } else {
                log(std::format("* Board {} says it has address 0x{:02x}, but that is another board's.\n", idString(id), sender));
            }
        }

        auto it = pending_.find(id.id);
        if ((it != pending_.end()) && (it->second.address == sender)) {
            log(std::format("- Board confirmed its address 0x{:02x}.\n", sender));
            pending_.erase(it);
            if (onConfirmed_) {
                onConfirmed_(id, sender);
            }
            // The board (re)started and took its address: whatever it showed is gone, whether we saw it leave or not.
            appeared(id, sender);
        } else if (!wasOnline) {
            log(std::format("- Board announced itself on address 0x{:02x}.\n", sender));
            appeared(id, sender);
        }
    }

    void appeared(const BoardId& id, uint8_t address) {
        if (onAppeared_) {
            onAppeared_(id, address);
        }
    }

    /**
     * Boards that have not been heard for a while are gone.
     */
    void checkGone(Clock::time_point now) {
        for (auto it = lastSeen_.begin(); it != lastSeen_.end(); ) {
            if ((now - it->second) < goneAfter_) {
                ++it;
                continue;
            }
            const uint8_t address = it->first;
            BoardId id{ .id = 0 };
            auto known = idByAddress_.find(address);
            if (known != idByAddress_.end()) {
                id.id = known->second;
            }
            log(std::format("- Board {} on address 0x{:02x} is gone: we have not heard from it.\n", idString(id), address));
            online_.erase(address);
            it = lastSeen_.erase(it);
            if (onGone_) {
                onGone_(id, address);
            }
        }
    }

    /**
     * Send the SetAddress again for every address that has not been confirmed within a short while.
     */
    void resendUnconfirmed(Clock::time_point now) {
        for (auto it = pending_.begin(); it != pending_.end(); ) {
            Pending& entry = it->second;
            if ((now - entry.lastSent) < resendAfter_) {
                ++it;
            } else if (entry.attempts >= maxAttempts_) {
                log(std::format("* No confirmation of address 0x{:02x}, giving up.\n", entry.address));
                it = pending_.erase(it);
            } else {
                log(std::format("- No confirmation yet of address 0x{:02x}, sending it again.\n", entry.address));
                handler_.sendSetAddress(entry.boardId, entry.address);
                entry.attempts++;
                entry.lastSent = now;
                ++it;
            }
        }
    }

public:
    explicit I2CBusController(ProtDriver& driver)
        : driver_(driver), handler_(driver, controllerId_) {}

    I2CBusController(const I2CBusController&) = delete;
    I2CBusController(I2CBusController&&) = delete;
    I2CBusController& operator=(const I2CBusController&) = delete;
    I2CBusController& operator=(I2CBusController&&) = delete;

    /** The range of addresses that are handed out. The default is 0x61 up to and including 0x77. */
    void addressRange(uint8_t first, uint8_t last) { firstAddress_ = first; lastAddress_ = last; }

    /** How often to say Hello (default once per second). */
    void helloInterval(Clock::duration interval) { helloInterval_ = interval; }

    /** How long to wait for a confirmation before sending an address again (default 100 ms), and how often to try (default 5). */
    void resendAfter(Clock::duration interval) { resendAfter_ = interval; }
    void maxAttempts(unsigned attempts) { maxAttempts_ = attempts; }

    /** Called when a board has confirmed an address. This is the place to save it. */
    void onConfirmed(ConfirmedCallback callback) { onConfirmed_ = std::move(callback); }

    /**
     * Called when a board is seen for the first time since we started, comes back after having been gone, or has taken the
     * address that we gave it (which means that it restarted, and has lost whatever it was showing).
     */
    void onBoardAppeared(BoardCallback callback) { onAppeared_ = std::move(callback); }

    /** Called when a board that was there has not been heard for a while (see goneAfter()). */
    void onBoardGone(BoardCallback callback) { onGone_ = std::move(callback); }

    /** How long a board may stay silent before it is gone (default 10 s). Boards announce themselves every few seconds. */
    void goneAfter(Clock::duration interval) { goneAfter_ = interval; }

    /** Where the messages of the controller go. The default is std::cerr. An empty function turns them off. */
    void logger(Logger logger) { logger_ = std::move(logger); }

    /** Tell the controller about a board and its address, for example from saved state. */
    void addKnown(const BoardId& id, uint8_t address) {
        addressById_[id.id] = address;
        idByAddress_[address] = id.id;
    }

    /** Has the board with this address announced itself since we started? */
    bool isOnline(uint8_t address) const { return online_.count(address) != 0; }

    /** The address of a board, or 0 if we do not know it. */
    uint8_t addressOf(const BoardId& id) const {
        auto it = addressById_.find(id.id);
        return (it == addressById_.end()) ? 0 : it->second;
    }

    /** The addresses that have been given out, but not confirmed yet. */
    size_t pendingCount() const { return pending_.size(); }

    /**
     * Handle a Hello from a board. (registerHandlers() makes the driver call this.)
     */
    void handleHello(uint8_t sender, const BoardId& id) {
        log(std::format("Received Hello message from 0x{:02x}, board with Id {}\n", sender, idString(id)));

        if (id.id == ControllerId) {
            return;     // somebody else claims to be a controller
        }
        if (sender == GeneralCallAddress) {
            handleRequest(id);
        } else {
            handleAnnouncement(sender, id);
        }
    }

    /**
     * Let the driver call us for every Hello that comes in.
     */
    void registerHandlers() {
        driver_.registerHandler(Command::Hello, "Bus controller Hello handler",
                                [this]([[maybe_unused]] Command command, uint8_t sender, const std::vector<uint8_t>& data) {
            if (data.size() != sizeMsgHello) {
                return;
            }
            BoardId id;
            std::memcpy(&id.bytes[0], data.data(), idSize);
            handleHello(sender, id);
        });
    }

    /**
     * Do the periodic work: say Hello once per interval, and repeat the addresses that have not been confirmed. Call this
     * often (every 10 ms is plenty), together with driver.processIncoming().
     */
    void tick() {
        const auto now = Clock::now();
        if (!helloSent_ || ((now - lastHello_) >= helloInterval_)) {
            handler_.sendHello(controllerId_);
            lastHello_ = now;
            helloSent_ = true;
        }
        resendUnconfirmed(now);
        checkGone(now);
    }
};

} // namespace nl::rakis::raspberrypi::protocols

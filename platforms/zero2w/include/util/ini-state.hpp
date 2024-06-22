#pragma once
/*
 * Copyright (c) 2024 by Bert Laverman. All Rights Reserved.
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


#include <ranges>
#include <tuple>
#include <string>
#include <map>

#include <util/verbose-component.hpp>


namespace nl::rakis::raspberrypi::util {

/**
 * @brief This class maintains configuration state, and provides for loading/storing it using "INI" files.
 */
class IniState : public VerboseComponent {
    std::string filename_{ "pi-state.ini" };
    std::map<std::string, std::map<std::string,std::string>> config_;
    bool dirty_{ false };

    static std::string trim(std::string str) {
        auto view = str
                | std::views::drop_while(isspace)
                | std::views::reverse
                | std::views::drop_while(isspace)
                | std::views::reverse;
        return std::string(view.begin(), view.end());
    }

public:
    std::string filename() const noexcept { return filename_; }
    void filename(std::string name) { filename_ = name; }

    /**
     * @brief Load the contents of the saved state and use those to potentially override the current content.
     *
     * @return False if no file was found, true otherwise.
     */
    bool load();

    /**
     * Save the current state to the file.
     */
    void save();

    /**
     * @brief Return the "dirty" flag.
     *
     * @return True if changes have been made.
     */
    bool dirty() const noexcept { return dirty_; }

    /**
     * @brief Reset the "dirty" flag.
     */
    void markClean() { dirty_ = false; }

    /**
     * @brief Set the "dirty" flag.
     */
    void markDirty() { dirty_ = true; }

    /**
     * @brief Check if a certain section is available.
     *
     * @return True if the section exists.
     */
    bool has(std::string section) const{ return config_.find(section) != config_.end(); }

    /**
     * @brief Return the mutable map of the given section. This will create the section if it does not yet exist.
     */
    std::map<std::string,std::string>& operator[](const std::string& section) { return config_[section]; }

    /**
     * @brief Return the unmutable map of the given section. This will throw if it does not yet exist.
     */
    const std::map<std::string,std::string>& operator[](const std::string& section) const { return config_.at(section); }

    /**
     * @brief Check if a certain key exists in a given section.
     */
    bool has(std::string section, std::string key) const {
        return has(section) && config_.at(section).find(key) != config_.at(section).end();
    }

    // Convenience methods for different kinds of sections

    auto ids(std::string prefix) const -> auto {
        return std::views::keys(config_)
            | std::views::filter([prefix](auto& key) { return key.starts_with(prefix); })
            | std::views::transform([prefix](auto& key) { return key.substr(prefix.size()); });
    }
    bool hasId(std::string prefix, std::string id) const {
        return has(prefix + id);
    }
    unsigned countIds(std::string prefix) const {
        unsigned count{ 0 };
        for ([[maybe_unused]] auto key : ids(prefix)) { count++; }
        return count;
    }
    std::map<std::string,std::string>& section(std::string prefix, std::string id) {
        return config_.at(prefix + id);
    }
    const std::map<std::string,std::string>& section(std::string prefix, std::string id) const {
        return config_.at(prefix + id);
    }
    std::map<std::string,std::string>& addSection(std::string prefix, std::string id) {
        if (!hasId(prefix, id)) { markDirty(); }
        return config_[prefix + id];
    }
    auto keys(std::string prefix, std::string id) const {
        return std::views::keys(section(prefix, id));
    }
    bool hasValue(std::string prefix, std::string id, std::string key) const {
        return has(prefix + id, key);
    }
    std::string value(std::string prefix, std::string id, std::string key) const {
        return hasValue(prefix, id, key) ? trim(section(prefix, id).at(key)) : std::string();
    }
    void setValue(std::string prefix, std::string id, std::string key, std::string value) {
        addSection(prefix, id) [key] = value; markDirty();
    }

    // Boards
    static constexpr const char* boardHeader{ "board:" };

    auto boardIds()                                                        const -> auto { return ids(boardHeader); }
    bool hasBoardId(std::string id)                                        const         { return hasId(boardHeader, id); }
    unsigned countBoardIds()                                               const         { return countIds(boardHeader); }
    std::map<std::string,std::string>& board(std::string id)                             { return section(boardHeader, id); }
    const std::map<std::string,std::string>& board(std::string id)         const         { return section(boardHeader, id); }
    std::map<std::string,std::string>& addBoard(std::string id)                          { return addSection(boardHeader, id); }
    bool hasBoardValue(std::string id, std::string key)                    const         { return hasValue(boardHeader, id, key); }
    auto boardKeys(std::string id)                                         const         { return keys(boardHeader, id); }
    std::string boardValue(std::string id, std::string key)                const         { return value(boardHeader, id, key); }
    void setBoardValue(std::string id, std::string key, std::string value)               { setValue(boardHeader, id, key, value); };

    // Interfaces
    static constexpr const char* interfaceHeader{ "interface:" };

    auto interfaceIds()                                                        const -> auto { return ids(interfaceHeader); }
    bool hasInterfaceId(std::string id)                                        const         { return hasId(interfaceHeader, id); }
    unsigned countInterfaceIds()                                               const         { return countIds(interfaceHeader); }
    std::map<std::string,std::string>& interface(std::string id)                             { return section(interfaceHeader, id); }
    const std::map<std::string,std::string>& interface(std::string id)         const         { return section(interfaceHeader, id); }
    std::map<std::string,std::string>& addInterface(std::string id)                          { return addSection(interfaceHeader, id); }
    bool hasInterfaceValue(std::string id, std::string key)                    const         { return hasValue(interfaceHeader, id, key); }
    auto interfaceKeys(std::string id)                                         const         { return keys(interfaceHeader, id); }
    std::string interfaceValue(std::string id, std::string key)                const         { return value(interfaceHeader, id, key); }
    void setInterfaceValue(std::string id, std::string key, std::string value)               { setValue(interfaceHeader, id, key, value); };

    // Devices
    static constexpr const char* deviceHeader{ "device:" };

    auto deviceIds()                                                        const -> auto { return ids(deviceHeader); }
    bool hasDeviceId(std::string id)                                        const         { return hasId(deviceHeader, id); }
    unsigned countDeviceIds()                                               const         { return countIds(deviceHeader); }
    std::map<std::string,std::string>& device(std::string id)                             { return section(deviceHeader, id); }
    const std::map<std::string,std::string>& device(std::string id)         const         { return section(deviceHeader, id); }
    std::map<std::string,std::string>& addDevice(std::string id)                          { return addSection(deviceHeader, id); }
    bool hasDeviceValue(std::string id, std::string key)                    const         { return hasValue(deviceHeader, id, key); }
    auto deviceKeys(std::string id)                                         const         { return keys(deviceHeader, id); }
    std::string deviceValue(std::string id, std::string key)                const         { return value(deviceHeader, id, key); }
    void setDeviceValue(std::string id, std::string key, std::string value)               { setValue(deviceHeader, id, key, value); };

};
} // namespace nl::rakis::i2c
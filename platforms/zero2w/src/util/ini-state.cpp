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


#include <cctype>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <string>
#include <format>
#include <ranges>

#include <util/ini-state.hpp>


using namespace nl::rakis::raspberrypi::util;


/**
 * @brief Load the configuration from the INI file.
 */
bool IniState::load()
{
    std::filesystem::path iniFile(filename_);
    if (!std::filesystem::exists(iniFile)) {
        log(std::format("State file '{}' not found.", filename_));
        return false;
    }
    std::ifstream iniStream(iniFile);

    if (!iniStream) {
        log(std::format("Cannot open state file '{}' for reading.", filename_));
        return false;
    }
    std::string line;
    std::string section{ "general" };
    while (std::getline(iniStream, line)) {
        line = trim(line);

        if (line.empty() || line[0] == '#') {
            continue;
        }
        if ((line.front() == '[') && (line.back() == ']')) {
            section = line.substr(1, line.size() - 2);
            continue;
        }
        auto eqPos = line.find('=');
        if (eqPos != std::string::npos) {
            auto key = trim(line.substr(0, eqPos));
            auto value = trim(line.substr(eqPos + 1));
            config_[section][key] = value;
        }
    }
    return true;
}


/**
 * @brief Save the configuration to the INI file.
 */
void IniState::save()
{
    if (!dirty()) {
        return;
    }
    {
        log(std::format("Writing state to '{}'.", filename_));
        std::string iniFile(filename_);
        std::ofstream iniStream(iniFile);
        if (!iniStream) {
        log(std::format("Cannot open state file '{}' for writing.", filename_));
            return;
        }
        for (auto& section : config_) {
            iniStream << '[' << section.first << "]\n";
            for (auto& keyValue : section.second) {
                iniStream << keyValue.first << " = " << keyValue.second << '\n';
            }
            iniStream << '\n';
        }
        markClean();
    }
    log(std::format("Configuration saved to '{}'.", filename_));
}
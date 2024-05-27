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


#include <map>


namespace nl::rakis::raspberrypi::components {


/**
 * @brief This is the base class for all components with a state that can be stored in a map.
 */
class StatefulComponent {

public:
    StatefulComponent() = default;
    StatefulComponent(const StatefulComponent&) = default;
    StatefulComponent(StatefulComponent&&) = default;
    StatefulComponent& operator=(const StatefulComponent&) = default;
    StatefulComponent& operator=(StatefulComponent&&) = default;
    virtual ~StatefulComponent() { };

    virtual void loadState(const std::map<std::string, std::string>& store) = 0;
    virtual void saveState(std::map<std::string, std::string>& store) = 0;

    virtual bool dirty() const = 0;
    virtual void markDirty() = 0;
    virtual void markClean() = 0;
}

} // namespace nl::rakis::raspberrypi::components
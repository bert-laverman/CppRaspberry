#pragma once
// Copyright (c) 2024 by Bert Laverman, all rights reserved


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
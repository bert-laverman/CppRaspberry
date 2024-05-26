#pragma once
// Copyright (c) 2024 by Bert Laverman, all rights reserved

#include <string>


namespace nl::rakis::raspberrypi::components {


/**
 * @brief This is the base class for all components with a name.
 */
class NamedComponent {
    std::string name_;

public:
    NamedComponent() = default;
    NamedComponent(const NamedComponent&) = default;
    NamedComponent(NamedComponent&&) = default;
    NamedComponent& operator=(const NamedComponent&) = default;
    NamedComponent& operator=(NamedComponent&&) = default;
    virtual ~NamedComponent() { };

    inline const std::string& name() const { return name_; }
    inline void name(std::string name) { name_ = name; }
}

} // namespace nl::rakis::raspberrypi::components
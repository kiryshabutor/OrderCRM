#pragma once
#include <string>
struct ISerializable {
    virtual ~ISerializable() = default;
    virtual std::string toLine() const = 0;
};

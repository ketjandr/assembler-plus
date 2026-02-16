#pragma once

#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <stdexcept>

/// Manages labels and their associated addresses.
class SymbolTable {
public:
    /// Define a label at a given address.  Throws on duplicate.
    void define(const std::string &name, uint64_t address) {
        if (table_.count(name))
            throw std::runtime_error("Duplicate label: " + name);
        table_[name] = address;
        order_.push_back(name);
    }

    /// Look up an address.  Throws if undefined.
    uint64_t lookup(const std::string &name) const {
        auto it = table_.find(name);
        if (it == table_.end())
            throw std::runtime_error("Undefined label: " + name);
        return it->second;
    }

    bool contains(const std::string &name) const { return table_.count(name); }

    /// Labels in definition order (for debug output).
    const std::vector<std::string> &order() const { return order_; }
    const std::map<std::string, uint64_t> &map() const { return table_; }

private:
    std::map<std::string, uint64_t> table_;
    std::vector<std::string> order_;
};

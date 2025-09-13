#pragma once

#include "ll/api/mod/NativeMod.h"

#include <chrono>
#include <string>
#include <unordered_map>

namespace chance_plugin {

class ChancePlugin {
public:
    static ChancePlugin& getInstance();

    ChancePlugin(const ChancePlugin&)            = delete;
    ChancePlugin& operator=(const ChancePlugin&) = delete;

    [[nodiscard]] ll::mod::NativeMod& getSelf() const { return mSelf; }

    bool load();
    bool enable();
    bool disable();

private:
    ChancePlugin() : mSelf(*ll::mod::NativeMod::current()) {}

    ll::mod::NativeMod& mSelf;

    std::unordered_map<std::string, std::chrono::steady_clock::time_point> mCooldowns;
};

} // namespace chance_plugin

#pragma once

#include "ll/api/mod/NativeMod.h"

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
};

} // namespace chance_plugin

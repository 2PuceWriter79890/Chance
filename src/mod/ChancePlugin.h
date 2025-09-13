#pragma once

#include "ll/api/mod/NativeMod.h"

#include <chrono>
#include <memory>
#include <random>
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

    // 将冷却地图作为类的普通成员变量
    std::unordered_map<std::string, std::chrono::steady_clock::time_point> mCooldowns;

    // 将随机数生成器也作为类的普通成员变量
    std::unique_ptr<std::mt19937> mRng;
};

} // namespace chance_plugin

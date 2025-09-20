#pragma once

#include "ll/api/mod/NativeMod.h"

#include <chrono>
#include <memory>
#include <random>
#include <string>
#include <unordered_map>
#include <unordered_set>

class Player;

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

    void showDisclaimerForm(Player& player);
    void showDivinationForm(Player& player);

    ll::mod::NativeMod& mSelf;

    std::unordered_map<std::string, std::chrono::steady_clock::time_point> mCooldowns;

    std::unique_ptr<std::mt19937> mRng;
    
    std::unordered_set<std::string> mConfirmedPlayers;
};

} // namespace chance_plugin

#include "ChancePlugin.h"

#include "ll/api/command/Command.h" // 需要包含这个头文件以使用 GreedyString
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/mod/RegisterHelper.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/world/actor/player/Player.h"

#include <iomanip>
#include <sstream>

namespace chance_plugin {

// 主类实现
ChancePlugin& ChancePlugin::getInstance() {
    static ChancePlugin instance;
    return instance;
}

bool ChancePlugin::load() {
    getSelf().getLogger().info("ChancePlugin 正在加载...");
    std::random_device rd;
    mRng = std::make_unique<std::mt19937>(rd());
    return true;
}

bool ChancePlugin::enable() {
    getSelf().getLogger().info("ChancePlugin 正在启用...");
    auto& registrar = ll::command::CommandRegistrar::getInstance();
    auto& handle =
        registrar.getOrCreateCommand("chance", "占卜事件发生的概率", CommandPermissionLevel::Any, {}, ll::mod::NativeMod::current());

    // ---vvv---  这里是关键的修正点 ---vvv---

    // 重载 1: 处理不带参数的情况
    handle.overload().execute(
        [](CommandOrigin const& origin, CommandOutput& output) {
            output.error("用法: /chance <所求事项>"); 
        }
    );

    // 重载 2: 处理带有所求事项的情况
    // 使用 ll::command::Command::GreedyString 来捕获所有后续文本
    struct Params {
        ll::command::Command::GreedyString 所求事项;
    };

    handle.overload<Params>().execute(
        [this](CommandOrigin const& origin, CommandOutput& output, Params const& params) {
            auto* actor  = origin.getEntity();
            
            if (!actor || !actor->isPlayer()) {
                output.error("该指令只能由玩家执行。");
                return;
            }
            auto* player = static_cast<Player*>(actor);

            bool const isOp = origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors;
            
            if (!isOp) {
                auto       playerName = player->getRealName();
                auto const now        = std::chrono::steady_clock::now();
                if (this->mCooldowns.count(playerName)) {
                    auto const lastUsed    = this->mCooldowns.at(playerName);
                    auto const timeElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUsed).count();
                    if (timeElapsed < 120) {
                        long long remaining = 120 - timeElapsed;
                        output.error("指令冷却中，请在 " + std::to_string(remaining) + " 秒后重试。");
                        return;
                    }
                }
            }
            
            // GreedyString 的内容需要通过 .value 成员访问
            std::string processedEvent = params.所求事项.value;
            processedEvent.erase(std::remove(processedEvent.begin(), processedEvent.end(), '\"'), processedEvent.end());
            
            std::uniform_real_distribution<double> distReal(0.0, 50.0);
            double                                 probability = distReal(*this->mRng) + distReal(*this->mRng);

            std::uniform_int_distribution<int> distInt(0, 1);
            int                                outcome     = distInt(*this->mRng);
            std::string                        outcomeText = (outcome == 0) ? "§a发生" : "§c不发生";

            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << probability;
            std::string probabilityText = ss.str();

            player->sendMessage("§e汝的所求事项：§f" + processedEvent);
            player->sendMessage("§e结果：§f此事件有 §d" + probabilityText + "%§f 的概率会 " + outcomeText + "§f！");

            if (!isOp) {
                this->mCooldowns[player->getRealName()] = std::chrono::steady_clock::now();
            }
        }
    );
    // ---^^^--- 修正结束 ---^^^---

    return true;
}

bool ChancePlugin::disable() {
    getSelf().getLogger().info("ChancePlugin 正在禁用...");
    mCooldowns.clear();
    return true;
}

} // namespace chance_plugin

// 注册插件
chance_plugin::ChancePlugin& plugin = chance_plugin::ChancePlugin::getInstance();
LL_REGISTER_MOD(chance_plugin::ChancePlugin, plugin);

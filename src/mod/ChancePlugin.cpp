#include "mod/ChancePlugin.h"

#include <ll/api/command/Command.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/io/Logger.h>
#include <ll/api/mod/RegisterHelper.h>
#include <mc/server/commands/CommandOrigin.h>
#include <mc/server/commands/CommandOutput.h>
#include <mc/server/commands/CommandPermissionLevel.h>
#include <mc/world/actor/player/Player.h>

// 标准库
#include <chrono>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

namespace chance_plugin {

// --- 静态变量和功能函数 ---

static std::unordered_map<std::string, std::chrono::steady_clock::time_point> gCooldowns;
static const int COOLDOWN_SECONDS = 120;
static std::unique_ptr<std::mt19937> gRng;

double generateChance() {
    std::uniform_real_distribution<double> dist(0.0, 50.0);
    return dist(*gRng) + dist(*gRng);
}

int generateOutcome() {
    std::uniform_int_distribution<int> dist(0, 1);
    return dist(*gRng);
}

// --- 指令回调函数 (已最终修正) ---
void chanceCommandCallback(
    CommandOrigin const&          origin, // 修正：移除 ll::command::，使用全局命名空间的 CommandOrigin
    CommandOutput&                output, // 修正：移除 ll::command::，使用全局命名空间的 CommandOutput
    struct Command_Params { std::string event; } const& params
) {
    // 修正：从 CommandOrigin 获取通用的 Actor*，然后安全地转换为 Player*
    auto* actor = origin.getEntity();
    auto* player = dynamic_cast<Player*>(actor);

    if (!player) {
        output.error("该指令只能由玩家执行。");
        return;
    }

    // 修正：直接从 CommandOrigin 获取权限等级
    bool const isOp = origin.getPermissionsLevel() >= CommandPermissionLevel::GameMasters;

    if (!isOp) {
        // Player 类 API 确认使用 getRealName()
        auto&      playerName = player->getRealName();
        auto const now        = std::chrono::steady_clock::now();
        if (gCooldowns.count(playerName)) {
            auto const lastUsed    = gCooldowns.at(playerName);
            auto const timeElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUsed).count();
            if (timeElapsed < COOLDOWN_SECONDS) {
                long long remaining = COOLDOWN_SECONDS - timeElapsed;
                output.error("指令冷却中，请在 " + std::to_string(remaining) + " 秒后重试。");
                return;
            }
        }
    }

    std::string processedEvent = params.event;
    processedEvent.erase(std::remove(processedEvent.begin(), processedEvent.end(), '\"'), processedEvent.end());

    double      probability = generateChance();
    int         outcome     = generateOutcome();
    std::string outcomeText = (outcome == 0) ? "§a发生" : "§c不发生";

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << probability;
    std::string probabilityText = ss.str();

    // Player 类 API 确认使用 sendMessage()
    player->sendMessage("§e汝的所求事项：§f" + processedEvent);
    player->sendMessage("§e结果：§f此事件有 §d" + probabilityText + "%§f 的概率会 " + outcomeText + "§f！");

    if (!isOp) {
        gCooldowns[player->getRealName()] = std::chrono::steady_clock::now();
    }
    output.success();
}


// --- 插件主类方法实现 ---

ChancePlugin& ChancePlugin::getInstance() {
    static ChancePlugin instance;
    return instance;
}

bool ChancePlugin::load() {
    getSelf().getLogger().info("ChancePlugin 正在加载...");
    std::random_device rd;
    gRng = std::make_unique<std::mt19937>(rd());
    return true;
}

bool ChancePlugin::enable() {
    getSelf().getLogger().info("ChancePlugin 正在启用...");
    auto& registrar = ll::command::CommandRegistrar::getInstance();
    auto& cmdHandle = registrar.getOrCreateCommand("chance", "占卜事件发生的概率", CommandPermissionLevel::Any, {}, getSelf());
    cmdHandle.overload<Command_Params>().execute<chanceCommandCallback>();
    return true;
}

bool ChancePlugin::disable() {
    getSelf().getLogger().info("ChancePlugin 正在禁用...");
    gCooldowns.clear();
    return true;
}

} // namespace chance_plugin

LL_REGISTER_MOD(chance_plugin::ChancePlugin, chance_plugin::ChancePlugin::getInstance());

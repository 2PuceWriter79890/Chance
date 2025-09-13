#include "ChancePlugin.h"

// LeviLamina & Minecraft Headers
#include <ll/api/command/Command.h>
#include <ll/api/command/CommandRegistrar.h>
#include <ll/api/io/Logger.h>
#include <ll/api/mod/RegisterHelper.h>
#include <mc/server/commands/CommandPermissionLevel.h>
#include <mc/world/actor/player/Player.h>

// Standard C++ Library Headers
#include <chrono>
#include <iomanip>
#include <memory>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

namespace chance_plugin {

// --- 静态变量和功能函数 (在命名空间内) ---

// 冷却时间记录
static std::unordered_map<std::string, std::chrono::steady_clock::time_point> gCooldowns;
static const int COOLDOWN_SECONDS = 120; // 2分钟
// 随机数生成器
static std::unique_ptr<std::mt19937> gRng;


// 生成一个 0.00-100.00 之间的概率值 (三角分布)
double generateChance() {
    std::uniform_real_distribution<double> dist(0.0, 50.0);
    double r1 = dist(*gRng);
    double r2 = dist(*gRng);
    return r1 + r2;
}

// 随机生成 0 (发生) 或 1 (不发生)
int generateOutcome() {
    std::uniform_int_distribution<int> dist(0, 1);
    return dist(*gRng);
}

// 指令回调函数
void chanceCommandCallback(
    ll::command::CommandOrigin const&          origin,
    ll::command::CommandOutput&                output,
    struct Command_Params { std::string event; } const& params
) {
    auto* player = origin.getPlayer();
    if (!player) {
        output.error("该指令只能由玩家执行。");
        return;
    }

    bool const isOp = player->getCommandPermissionLevel() >= CommandPermissionLevel::GameMasters;

    if (!isOp) {
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

    std::string line1 = "§e汝的所求事项：§f" + processedEvent;
    std::string line2 = "§e结果：§f此事件有 §d" + probabilityText + "%§f 的概率会 " + outcomeText + "§f！";
    player->sendMessage(line1);
    player->sendMessage(line2);

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
    getSelf().getLogger().info("ChancePlugin 正在加载..."); // 使用 info 级别日志
    // 初始化随机数生成器
    std::random_device rd;
    gRng = std::make_unique<std::mt19937>(rd());
    return true;
}

bool ChancePlugin::enable() {
    getSelf().getLogger().info("ChancePlugin 正在启用..."); // 使用 info 级别日志
    // 注册指令
    auto& registrar = ll::command::CommandRegistrar::getInstance();
    auto& cmdHandle = registrar.getOrCreateCommand("chance", "占卜事件发生的概率", CommandPermissionLevel::Any, {}, getSelf());
    cmdHandle.overload<Command_Params>().execute<chanceCommandCallback>();
    return true;
}

bool ChancePlugin::disable() {
    getSelf().getLogger().info("ChancePlugin 正在禁用..."); // 使用 info 级别日志
    // 清理插件状态，为热重载做准备
    gCooldowns.clear();
    return true;
}

} // namespace chance_plugin

// 使用宏将插件类和它的单例获取方法注册到 LeviLamina 加载器
LL_REGISTER_MOD(chance_plugin::ChancePlugin, chance_plugin::ChancePlugin::getInstance());

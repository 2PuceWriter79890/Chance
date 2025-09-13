#include "mod/ChancePlugin.h"

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

// --- 静态变量和全局定义 ---

// 插件单例实例指针
static std::unique_ptr<ChancePlugin> instance;
// 日志记录器
static ll::io::Logger                logger("ChancePlugin");
// 冷却时间记录
static std::unordered_map<std::string, std::chrono::steady_clock::time_point> gCooldowns;
static const int COOLDOWN_SECONDS = 120; // 2分钟
// 随机数生成器
static std::unique_ptr<std::mt19_37> gRng;


// --- 核心功能函数 ---

// 生成一个 0.00-100.00 之间的概率值 (三角分布，中间概率高，两端概率低)
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


// --- 指令回调函数 ---
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

    // 检查玩家是否为OP，OP跳过冷却
    bool const isOp = player->getCommandPermissionLevel() >= CommandPermissionLevel::GameMasters;

    // 如果不是OP，则检查冷却
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

    // 处理输入字符串，移除双引号
    std::string processedEvent = params.event;
    processedEvent.erase(std::remove(processedEvent.begin(), processedEvent.end(), '\"'), processedEvent.end());

    // 生成结果
    double      probability = generateChance();
    int         outcome     = generateOutcome();
    std::string outcomeText = (outcome == 0) ? "§a发生" : "§c不发生"; // 使用颜色代码美化

    // 格式化输出
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << probability;
    std::string probabilityText = ss.str();

    // 发送消息给玩家
    std::string line1 = "§e汝的所求事项：§f" + processedEvent;
    std::string line2 = "§e结果：§f此事件有 §d" + probabilityText + "%§f 的概率会 " + outcomeText + "§f！";
    player->sendMessage(line1);
    player->sendMessage(line2);

    // 如果不是OP，更新冷却时间
    if (!isOp) {
        gCooldowns[player->getRealName()] = std::chrono::steady_clock::now();
    }
    output.success();
}


// --- 插件主类方法实现 ---

ChancePlugin& ChancePlugin::getInstance(ll::mod::NativeMod& self) {
    if (!instance) {
        instance = std::unique_ptr<ChancePlugin>(new ChancePlugin(self));
    }
    return *instance;
}

ChancePlugin& ChancePlugin::getInstance() { return *instance; }

ChancePlugin::ChancePlugin(ll::mod::NativeMod& self) : mSelf(self) {}

bool ChancePlugin::load() {
    logger.info("Chance 插件正在加载...");
    return true;
}

bool ChancePlugin::enable() {
    logger.info("Chance 插件正在启用...");

    // 初始化随机数生成器
    std::random_device rd;
    gRng = std::make_unique<std::mt19937>(rd());

    // 注册指令
    auto& registrar = ll::command::CommandRegistrar::getInstance();
    // 关键：传入 mSelf，将指令与本插件实例绑定，以便在卸载时自动注销
    auto& cmdHandle = registrar.getOrCreateCommand("chance", "占卜事件发生的概率", CommandPermissionLevel::Any, {}, mSelf);
    cmdHandle.overload<Command_Params>().execute<chanceCommandCallback>();

    logger.info("指令 /chance 注册成功。");
    return true;
}

bool ChancePlugin::disable() {
    logger.info("Chance 插件正在禁用...");
    
    // 清理插件运行中产生的状态，为热重载做准备
    gCooldowns.clear();
    
    // 指令的注销由 LeviLamina 框架在插件禁用时自动处理
    
    return true;
}

// 使用宏将插件类和它的单例获取方法注册到 LeviLamina 加载器
LL_REGISTER_MOD(ChancePlugin, ChancePlugin::getInstance());

} // namespace chance_plugin

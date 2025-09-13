#include "mod/ChanceCommand.h"

// 包含所有必需的头文件
#include "ll/api/command/CommandHandle.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/world/actor/player/Player.h"

#include <chrono>
#include <iomanip>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>

// 从主插件文件获取随机数生成器
namespace chance_plugin {
std::mt19937& getRng();
}

namespace chance_plugin::command {

// --- 静态数据和功能函数 ---
static std::unordered_map<std::string, std::chrono::steady_clock::time_point> gCooldowns;
static const int COOLDOWN_SECONDS = 120;

// 定义命令参数的结构体
struct CommandParams {
    std::string event;
};

// --- 核心执行函数 ---
// 这是一个静态函数，作为命令的回调
static void execute(CommandOrigin const& origin, CommandOutput& output, CommandParams const& params) {
    auto* actor  = origin.getEntity();
    auto* player = dynamic_cast<Player*>(actor);

    if (!player) {
        output.error("该指令只能由玩家执行。");
        return;
    }

    bool const isOp = origin.getPermissionsLevel() >= CommandPermissionLevel::GameMasters;

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

    std::uniform_real_distribution<double> distReal(0.0, 50.0);
    double                                 probability = distReal(getRng()) + distReal(getRng());

    std::uniform_int_distribution<int> distInt(0, 1);
    int                                outcome     = distInt(getRng());
    std::string                        outcomeText = (outcome == 0) ? "§a发生" : "§c不发生";

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << probability;
    std::string probabilityText = ss.str();

    player->sendMessage("§e汝的所求事项：§f" + processedEvent);
    player->sendMessage("§e结果：§f此事件有 §d" + probabilityText + "%§f 的概率会 " + outcomeText + "§f！");

    if (!isOp) {
        gCooldowns[player->getRealName()] = std::chrono::steady_clock::now();
    }
    output.success();
}

// --- 注册函数的实现 ---
void ChanceCommand::reg(ll::command::CommandHandle& handle) {
    // 注册一个重载，它使用 CommandParams 结构体，并绑定到静态的 execute 函数
    handle.overload<CommandParams>().execute<execute>();
}

void ChanceCommand::clearCooldowns() { gCooldowns.clear(); }

} // namespace chance_plugin::command

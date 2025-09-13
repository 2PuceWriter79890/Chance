#include "mod/ChancePlugin.h"

// 包含了所有必需的头文件
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/mod/RegisterHelper.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/world/actor/player/Player.h"

#include <iomanip>
#include <memory>
#include <random>
#include <sstream>

namespace chance_plugin {

// --- 静态变量 ---
// 随机数生成器仍然适合作为静态变量，因为它不包含与特定插件实例相关的状态
static std::unique_ptr<std::mt19937> gRng;


// --- 主类实现 ---

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
    auto& handle =
        registrar.getOrCreateCommand("chance", "占卜事件发生的概率", CommandPermissionLevel::Any, {}, ll::mod::NativeMod::current());

    // 定义命令参数的结构体
    struct Params {
        std::string event;
    };

    // 使用 Lambda 表达式注册命令，并捕获 this 指针
    handle.overload<Params>().execute(
        [this](CommandOrigin const& origin, CommandOutput& output, Params const& params) {
            auto* actor  = origin.getEntity();
            auto* player = dynamic_cast<Player*>(actor);

            if (!player) {
                output.error("该指令只能由玩家执行。");
                return;
            }

            bool const isOp = origin.getPermissionsLevel() >= CommandPermissionLevel::GameMasters;
            
            // 使用 this->mCooldowns 访问成员变量
            if (!isOp) {
                auto&      playerName = player->getRealName();
                auto const now        = std::chrono::steady_clock::now();
                if (this->mCooldowns.count(playerName)) {
                    auto const lastUsed    = this->mCooldowns.at(playerName);
                    auto const timeElapsed = std::chrono::duration_cast<std::chrono::seconds>(now - lastUsed).count();
                    if (timeElapsed < 120) { // 120 秒冷却
                        long long remaining = 120 - timeElapsed;
                        output.error("指令冷却中，请在 " + std::to_string(remaining) + " 秒后重试。");
                        return;
                    }
                }
            }

            std::string processedEvent = params.event;
            processedEvent.erase(std::remove(processedEvent.begin(), processedEvent.end(), '\"'), processedEvent.end());

            std::uniform_real_distribution<double> distReal(0.0, 50.0);
            double                                 probability = distReal(*gRng) + distReal(*gRng);

            std::uniform_int_distribution<int> distInt(0, 1);
            int                                outcome     = distInt(*gRng);
            std::string                        outcomeText = (outcome == 0) ? "§a发生" : "§c不发生";

            std::stringstream ss;
            ss << std::fixed << std::setprecision(2) << probability;
            std::string probabilityText = ss.str();

            player->sendMessage("§e汝的所求事项：§f" + processedEvent);
            player->sendMessage("§e结果：§f此事件有 §d" + probabilityText + "%§f 的概率会 " + outcomeText + "§f！");

            if (!isOp) {
                this->mCooldowns[player->getRealName()] = std::chrono::steady_clock::now();
            }
            output.success();
        }
    );

    return true;
}

bool ChancePlugin::disable() {
    getSelf().getLogger().info("ChancePlugin 正在禁用...");
    // 清理冷却数据
    mCooldowns.clear();
    return true;
}

} // namespace chance_plugin

LL_REGISTER_MOD(chance_plugin::ChancePlugin, chance_plugin::ChancePlugin::getInstance());

#include "mod/ChancePlugin.h"

#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/Form.h" // [FIX] 引入这个核心头文件，它定义了 sendForm 和所有 Response 类型
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
        registrar.getOrCreateCommand("chance", "打开占卜表单", CommandPermissionLevel::Any, {}, ll::mod::NativeMod::current());

    handle.overload().execute(
        [this](CommandOrigin const& origin, CommandOutput& output) {
            auto* actor = origin.getEntity();
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

            auto form = std::make_shared<ll::form::CustomForm>("§d§l吉凶占卜");
            form->addInput("event", "§b请输入汝所求之事：\n§7(例如：我能否成仙)");

            // [FIX] 现在 player->sendForm(...) 可以被正确识别
            player->sendForm(
                form,
                [this](Player* player, ll::form::CustomFormResponse const& resp) {
                    if (resp.isTerminated()) {
                        return;
                    }

                    auto eventText = resp.getInput("event");
                    if (!eventText || eventText->empty()) {
                        player->sendMessage("§c汝未填写所求之事，天机不可泄露。");
                        return;
                    }

                    std::string processedEvent = *eventText;
                    processedEvent.erase(
                        std::remove(processedEvent.begin(), processedEvent.end(), '\"'),
                        processedEvent.end()
                    );

                    std::uniform_real_distribution<double> distReal(0.0, 50.0);
                    double probability = distReal(*this->mRng) + distReal(*this->mRng);

                    std::uniform_int_distribution<int> distInt(0, 1);
                    int                                outcome     = distInt(*this->mRng);
                    std::string                        outcomeText = (outcome == 0) ? "§a发生" : "§c不发生";

                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(2) << probability;
                    std::string probabilityText = ss.str();

                    player->sendMessage("§e汝的所求事项：§f" + processedEvent);
                    player->sendMessage(
                        "§e结果：§f此事件有 §d" + probabilityText + "%§f 的概率会 " + outcomeText + "§f！"
                    );
                }
            );

            if (!isOp) {
                this->mCooldowns[player->getRealName()] = std::chrono::steady_clock::now();
            }
        }
    );

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

#include "mod/ChancePlugin.h"

#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/form/CustomForm.h" // [NEW] 引入 CustomForm 头文件
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

    // ---vvv---  这里是关键的修正点 ---vvv---

    // 指令现在只有一个无参数的重载，用于打开表单
    handle.overload().execute(
        [this](CommandOrigin const& origin, CommandOutput& output) {
            auto* actor = origin.getEntity();
            if (!actor || !actor->isPlayer()) {
                output.error("该指令只能由控制台或玩家执行。");
                return;
            }
            auto* player = static_cast<Player*>(actor);

            bool const isOp = origin.getPermissionsLevel() >= CommandPermissionLevel::GameDirectors;

            // 1. 先检查冷却
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

            // 2. 创建并发送表单
            auto form = std::make_shared<ll::form::CustomForm>("§d§l吉凶占卜");
            form->addInput("event", "§b请输入汝所求之事：\n§7(例如：我能否成仙)"); // key 为 "event"

            // 发送表单并设置一个回调函数来处理玩家的提交
            player->sendForm(
                form,
                // --- 表单回调逻辑 ---
                [this, player, isOp](ll::form::CustomFormResponse const& resp) {
                    // 如果玩家关闭了表单，则不执行任何操作
                    if (resp.isTerminated()) {
                        return;
                    }

                    // 从表单回复中获取名为 "event" 的输入框内容
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

                    // --- 执行核心占卜逻辑 ---
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

            // 3. 发送表单后，立即更新冷却时间
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

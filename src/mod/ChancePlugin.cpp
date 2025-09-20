#include "ChancePlugin.h"

#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/SimpleForm.h"
#include "ll/api/mod/RegisterHelper.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/world/actor/player/Player.h"

#include <iomanip>
#include <sstream>

namespace chance_plugin {

// --- 主类实现 ---

ChancePlugin& ChancePlugin::getInstance() {
    static ChancePlugin instance;
    return instance;
}

// [MODIFIED] 使用 SimpleForm 替换 ModalForm
void ChancePlugin::showDisclaimerForm(Player& player) {
    ll::form::SimpleForm form("§e§l使用申明", "§f此插件仅供娱乐，不提供任何参考价值。\n\n§7点击“确认”即表示您同意此条款。");

    // [FIX] 1. 方法名是 appendButton
    // [FIX] 2. 带图标需要三个参数: text, image path, image type ("path")
    form.appendButton("§a确认 (Confirm)", "textures/icon/True", "path"); // 按钮 0
    form.appendButton("§c退出 (Exit)", "textures/icon/False", "path");    // 按钮 1

    // [FIX] 3. 回调函数的签名和逻辑已根据头文件更新
    form.sendTo(player, [this](Player& cbPlayer, int buttonIndex, ll::form::FormCancelReason reason) {
        // 如果表单被关闭或取消，则 reason 会有值
        if (reason) {
            return;
        }

        // 检查玩家是否点击了第一个按钮（“确认”）
        if (buttonIndex == 0) {
            mConfirmedPlayers.insert(cbPlayer.getRealName());
            cbPlayer.sendMessage("§a您已同意使用申明。");
            
            bool isOp = cbPlayer.getCommandPermissionLevel() >= CommandPermissionLevel::GameDirectors;
            if (!isOp) {
                this->mCooldowns[cbPlayer.getRealName()] = std::chrono::steady_clock::now();
            }
            showDivinationForm(cbPlayer);
        }
    });
}

void ChancePlugin::showDivinationForm(Player& player) {
    ll::form::CustomForm form("§d§l吉凶占卜");
    form.appendInput("event", "§b请输入汝所求之事：\n§7(例如：我能否成仙)");
    form.setSubmitButton("提交占卜");

    form.sendTo(
        player,
        [this](Player& cbPlayer, ll::form::CustomFormResult const& result, ll::form::FormCancelReason) {
            if (!result) {
                return;
            }

            auto const& resultMap = *result;
            auto        it        = resultMap.find("event");
            if (it == resultMap.end()) {
                return;
            }

            std::string eventText = std::get<std::string>(it->second);
            if (eventText.find_first_not_of(" \t\n\v\f\r") == std::string::npos) {
                cbPlayer.sendMessage("§c汝未填写所求之事，天机不可泄露。");
                return;
            }

            std::string processedEvent = eventText;
            processedEvent.erase(std::remove(processedEvent.begin(), processedEvent.end(), '\"'), processedEvent.end());

            std::uniform_real_distribution<double> distReal(0.0, 50.0);
            double                                 probability = distReal(*this->mRng) + distReal(*this->mRng);
            std::uniform_int_distribution<int>     distInt(0, 1);
            int                                    outcome     = distInt(*this->mRng);
            std::string                            outcomeText = (outcome == 0) ? "§a发生" : "§c不发生";
            std::stringstream                      ss;
            ss << std::fixed << std::setprecision(2) << probability;
            std::string probabilityText = ss.str();

            cbPlayer.sendMessage("§e汝的所求事项：§f" + processedEvent);
            cbPlayer.sendMessage("§e结果：§f此事件有 §d" + probabilityText + "%§f 的概率会 " + outcomeText + "§f！");
        }
    );
}

// --- 插件生命周期函数 ---

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

    handle.overload().execute([this](CommandOrigin const& origin, CommandOutput& output) {
        auto* actor = origin.getEntity();
        if (!actor || !actor->isPlayer()) {
            output.error("该指令只能由玩家执行。");
            return;
        }
        auto* player = static_cast<Player*>(actor);

        if (mConfirmedPlayers.count(player->getRealName())) {
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
             if (!isOp) {
                this->mCooldowns[player->getRealName()] = std::chrono::steady_clock::now();
            }
            showDivinationForm(*player);
        } else {
            showDisclaimerForm(*player);
        }
    });

    return true;
}

bool ChancePlugin::disable() {
    getSelf().getLogger().info("ChancePlugin 正在禁用...");
    mCooldowns.clear();
    mConfirmedPlayers.clear();
    return true;
}

} // namespace chance_plugin

// 注册插件
chance_plugin::ChancePlugin& plugin = chance_plugin::ChancePlugin::getInstance();
LL_REGISTER_MOD(chance_plugin::ChancePlugin, plugin);

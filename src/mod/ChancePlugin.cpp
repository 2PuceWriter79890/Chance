#include "mod/ChancePlugin.h"
#include "mod/ChanceCommand.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/mod/RegisterHelper.h"
#include <memory>

namespace chance_plugin {

// --- 静态变量 ---
static std::unique_ptr<std::mt19937> gRng;

// --- 外部可访问的函数 ---
// 将随机数生成器实例提供给命令文件
std::mt19937& getRng() { return *gRng; }

// --- 主类实现 ---

ChancePlugin& ChancePlugin::getInstance() {
    static ChancePlugin instance;
    return instance;
}

bool ChancePlugin::load() {
    getSelf().getLogger().info("ChancePlugin 正在加载...");
    // 在 load 阶段初始化，确保 enable 时可用
    std::random_device rd;
    gRng = std::make_unique<std::mt19937>(rd());
    return true;
}

bool ChancePlugin::enable() {
    getSelf().getLogger().info("ChancePlugin 正在启用...");

    // 从注册器获取 "chance" 命令的句柄
    auto& commandRegistrar = ll::command::CommandRegistrar::getInstance();
    auto& commandHandle =
        commandRegistrar.getOrCreateCommand("chance", "占卜事件发生的概率", CommandPermissionLevel::Any, {}, getSelf());

    // 调用命令类中的静态注册函数
    command::ChanceCommand::reg(commandHandle);

    return true;
}

bool ChancePlugin::disable() {
    getSelf().getLogger().info("ChancePlugin 正在禁用...");
    // 在 disable 时可以清理全局资源
    command::ChanceCommand::clearCooldowns();
    return true;
}

} // namespace chance_plugin

LL_REGISTER_MOD(chance_plugin::ChancePlugin, chance_plugin::ChancePlugin::getInstance());

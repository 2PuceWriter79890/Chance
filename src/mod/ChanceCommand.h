#pragma once

namespace ll::command {
class CommandHandle; // 前向声明
}

namespace chance_plugin::command {

class ChanceCommand {
public:
    // 静态注册函数，接收一个命令句柄
    static void reg(ll::command::CommandHandle& handle);

    // 静态函数，用于在插件禁用时清理冷却数据
    static void clearCooldowns();
};

} // namespace chance_plugin::command

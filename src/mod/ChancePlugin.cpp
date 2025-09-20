#include "mod/ChancePlugin.h"

#include "ll/api/command/Command.h"
#include "ll/api/command/CommandHandle.h"
#include "ll/api/command/CommandRegistrar.h"
#include "ll/api/form/CustomForm.h"
#include "ll/api/form/Forms.h"               // This header includes the necessary response type definitions.
#include "ll/api/mod/RegisterHelper.h"
#include "mc/server/commands/CommandOrigin.h"
#include "mc/server/commands/CommandOutput.h"
#include "mc/server/commands/CommandPermissionLevel.h"
#include "mc/world/actor/player/Player.h"

#include <iomanip>
#include <sstream>

namespace chance_plugin {

// Main class implementation
ChancePlugin& ChancePlugin::getInstance() {
    static ChancePlugin instance;
    return instance;
}

bool ChancePlugin::load() {
    getSelf().getLogger().info("ChancePlugin Loading...");
    std::random_device rd;
    mRng = std::make_unique<std::mt19937>(rd());
    return true;
}

bool ChancePlugin::enable() {
    getSelf().getLogger().info("ChancePlugin Enabling...");
    auto& registrar = ll::command::CommandRegistrar::getInstance();
    auto& handle =
        registrar.getOrCreateCommand("chance", "Open the divination form", CommandPermissionLevel::Any, {}, ll::mod::NativeMod::current());

    handle.overload().execute(
        [this](CommandOrigin const& origin, CommandOutput& output) {
            auto* actor = origin.getEntity();
            if (!actor || !actor->isPlayer()) {
                output.error("This command can only be executed by a player.");
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
                        output.error("Command is on cooldown, please try again in " + std::to_string(remaining) + " seconds.");
                        return;
                    }
                }
            }

            auto form = std::make_shared<ll::form::CustomForm>("§d§lFortune Telling");
            form->addInput("event", "§bPlease enter what you seek:\n§7(e.g., Will I become immortal?)");

            ll::form::sendForm(
                player,
                form,
                [this](Player* player, ll::form::CustomFormResponse const& resp) {
                    if (resp.isTerminated()) {
                        return;
                    }

                    auto eventText = resp.getInput("event");
                    if (!eventText || eventText->empty()) {
                        player->sendMessage("§cYou did not enter anything, the secrets of heaven cannot be revealed.");
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
                    std::string                        outcomeText = (outcome == 0) ? "§aoccur" : "§cnot occur";

                    std::stringstream ss;
                    ss << std::fixed << std::setprecision(2) << probability;
                    std::string probabilityText = ss.str();

                    player->sendMessage("§eYour sought-after matter: §f" + processedEvent);
                    player->sendMessage(
                        "§eResult: §fThis event has a §d" + probabilityText + "%§f chance to §b" + outcomeText + "§f!"
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
    getSelf().getLogger().info("ChancePlugin Disabling...");
    mCooldowns.clear();
    return true;
}

} // namespace chance_plugin

// Register the plugin
chance_plugin::ChancePlugin& plugin = chance_plugin::ChancePlugin::getInstance();
LL_REGISTER_MOD(chance_plugin::ChancePlugin, plugin);

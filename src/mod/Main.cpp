#include "scr/ChancePlugin.h"
#include <ll/api/mod/NativeMod.h>

extern "C" LL_EXPORT bool ll_mod_init(ll::mod::NativeMod& mod) {
    chance_plugin::ChancePlugin::getInstance(mod);
    return true;
}

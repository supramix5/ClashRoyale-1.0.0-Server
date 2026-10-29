#include "logic/home/LogicSpell.h"

namespace server {
namespace LogicSpell {
namespace {
constexpr std::size_t kSpellSize = 0x30;
constexpr int kSpellLevelIndexOffset = 8;
}

library::Ptr create(library::Ptr spellData, int levelIndex) {
    const auto spell = library::allocate(kSpellSize);
    library::call("__ZN10LogicSpellC1Ev", {spell});
    library::call("__ZN10LogicSpell12setSpellDataEPK14LogicSpellData", {spell, spellData});
    setLevelIndex(spell, levelIndex);
    return spell;
}

void setLevelIndex(library::Ptr spell, int levelIndex) {
    library::writeInt(spell + kSpellLevelIndexOffset, levelIndex);
}

library::Ptr clone(library::Ptr spell) { return library::call("__ZNK10LogicSpell5cloneEv", {spell}); }
} // namespace LogicSpell
} // namespace server

#include "logic/home/LogicClientHome.h"

#include "logic/data/LogicData.h"
#include "logic/data/LogicDataTables.h"
#include "logic/home/LogicSpell.h"
#include "titan/logic/datastream/ByteStream.h"
#include "titan/logic/debug/Debugger.h"

namespace server {
using library::call;
using library::Ptr;

namespace {
// LogicClientHome (0x198 bytes): home id LogicLong* +0x10, decks LogicArrayList<LogicSpellDeck*> +0x18 (count +0x24),
// shop seed +0xec, shop weekday +0xf0.
constexpr std::size_t kSize = 0x198;
constexpr int kHomeIdOffset = 0x10;
constexpr int kDeckCountOffset = 0x24;
constexpr int kShopSeedOffset = 0xec;
constexpr int kShopWeekdayOffset = 0xf0;

std::uint64_t argument(int value) { return std::uint64_t(std::uint32_t(value)); }
} // namespace

LogicClientHome LogicClientHome::decode(const std::vector<std::uint8_t> &encoded) {
    const auto home = library::allocate(kSize);
    call("__ZN15LogicClientHomeC1Ev", {home});
    ByteStream stream(encoded);
    call("__ZN15LogicClientHome6decodeEP10ByteStream", {home, stream.getLibraryStream()});
    return LogicClientHome(home, true);
}

LogicClientHome LogicClientHome::view(Ptr home) { return LogicClientHome(home, false); }

LogicClientHome::LogicClientHome(LogicClientHome &&other) noexcept : home_(other.home_), owned_(other.owned_) {
    other.home_ = 0;
    other.owned_ = false;
}

LogicClientHome &LogicClientHome::operator=(LogicClientHome &&other) noexcept {
    if (this != &other) {
        destroy();
        home_ = other.home_;
        owned_ = other.owned_;
        other.home_ = 0;
        other.owned_ = false;
    }
    return *this;
}

LogicClientHome::~LogicClientHome() { destroy(); }

void LogicClientHome::destroy() {
    if (!home_ || !owned_) return;
    call("__ZN15LogicClientHome8destructEv", {home_});
    library::release(home_);
    home_ = 0;
}

Ptr LogicClientHome::release() {
    const auto home = home_;
    home_ = 0;
    owned_ = false;
    return home;
}

std::vector<std::uint8_t> LogicClientHome::encode() const {
    ByteStream stream;
    encode(stream);
    return stream.getByteArray();
}

void LogicClientHome::encode(ByteStream &stream) const {
    call("__ZNK15LogicClientHome6encodeEP15ChecksumEncoder", {home_, stream.getLibraryStream()});
}

LogicClientHome LogicClientHome::clone() const { return decode(encode()); }

void LogicClientHome::setHomeId(std::int64_t id) {
    library::writePointer(home_ + kHomeIdOffset, library::newLogicLong(id));
}
int LogicClientHome::getDeckCount() const { return library::readInt(home_ + kDeckCountOffset); }
void LogicClientHome::addEmptyDeck() { call("__ZN15LogicClientHome12addEmptyDeckEv", {home_}); }
Ptr LogicClientHome::getSelectedDeck() const { return call("__ZNK15LogicClientHome15getSelectedDeckEv", {home_}); }
void LogicClientHome::destroyChests() { call("__ZN15LogicClientHome13destroyChestsEv", {home_}); }
void LogicClientHome::debugFinishTutorial() { call("__ZN15LogicClientHome19debugFinishTutorialEv", {home_}); }

int LogicClientHome::getShopSeed() const { return library::readInt(home_ + kShopSeedOffset); }
int LogicClientHome::getShopWeekday() const { return library::readInt(home_ + kShopWeekdayOffset); }

void LogicClientHome::setShopValues(int seed, int secondsLeft, int weekday) {
    call("__ZN15LogicClientHome13setShopValuesEiii", {home_, argument(seed), argument(secondsLeft), argument(weekday)});
}

int LogicClientHome::getChestCount() const { return int(call("__ZNK15LogicClientHome11getChestCntEv", {home_})); }
int LogicClientHome::getMaxChestCount() const {
    return int(call("__ZNK15LogicClientHome16getMaxChestCountEv", {home_}));
}

// Index -1: the first free slot.
void LogicClientHome::addChest(Ptr chest) {
    call("__ZN15LogicClientHome8addChestEP10LogicChesti", {home_, chest, argument(-1)});
}

int LogicClientHome::getSpellCount(Ptr spellData) const {
    return int(call("__ZNK15LogicClientHome13getSpellCountEPK14LogicSpellData", {home_, spellData}));
}

void LogicClientHome::unlockAndMaxAllSpells() {
    const auto collection = call("__ZNK15LogicClientHome18getSpellCollectionEv", {home_});
    const int deckCount = getDeckCount();

    for (int i = 0; i < LogicDataTables::getSpellCount(); ++i) {
        const auto spellData = LogicDataTables::getSpellAt(i);
        if (!spellData || LogicData::isSpellNotInUse(spellData)) continue;

        const int levelIndex = LogicData::getSpellMaxLevelIndex(spellData);
        auto collectionSpell =
            call("__ZNK20LogicSpellCollection14getSpellByDataEPK14LogicSpellData", {collection, spellData});
        bool spellInDeck = false;

        for (int deckIndex = 0; deckIndex < deckCount; ++deckIndex) {
            const auto deck = call("__ZNK15LogicClientHome12getLogicDeckEi", {home_, argument(deckIndex)});
            if (!deck) continue;
            const auto deckSpell =
                call("__ZNK14LogicSpellDeck14getSpellByDataEPK14LogicSpellData", {deck, spellData});
            if (deckSpell) {
                LogicSpell::setLevelIndex(deckSpell, levelIndex);
                spellInDeck = true;
            }
        }

        if (collectionSpell) {
            LogicSpell::setLevelIndex(collectionSpell, levelIndex);
        } else if (!spellInDeck) {
            auto spell = LogicSpell::create(spellData, 0);
            call("__ZN20LogicSpellCollection8addSpellEP10LogicSpell", {collection, spell});
            collectionSpell =
                call("__ZNK20LogicSpellCollection14getSpellByDataEPK14LogicSpellData", {collection, spellData});
            if (!collectionSpell)
                Debugger::error("spell %s was not added to the collection", LogicData::getName(spellData).c_str());
            LogicSpell::setLevelIndex(collectionSpell, levelIndex);
        }
    }
}
} // namespace server

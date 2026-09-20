#pragma once
// Independent RefinedBadger code. No vendor implementation or assets are required.
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <variant>
#include <vector>
#ifndef RBITEMECONOMY_API
#define RBITEMECONOMY_API
#endif

namespace RB::Items {
using Id = std::uint64_t;
using Count = std::int64_t;
using Time = std::int64_t; // Absolute monotonic GAME seconds supplied by the host.
constexpr Id MaxId = 0x7fffffffffffffffULL;
constexpr std::size_t MaxRecords = 100000;
constexpr std::size_t MaxSnapshotBytes = 64 * 1024 * 1024;
constexpr std::int32_t BasisPoints = 10000;

enum class Error : std::uint8_t {
    None, Invalid, Forbidden, Missing, Capacity, Conflict, Stale, ReplayMismatch,
    Insufficient, Reserved, Equipped, NotReady, Expired, Corrupt, CatalogMismatch,
    Budget, WrongThread, Locked
};
struct Result {
    Error error = Error::None;
    std::string message;
    std::uint64_t revision = 0;
    std::vector<Id> created;
    bool replayed = false;
    explicit operator bool() const noexcept { return error == Error::None; }
};
struct Meta {
    std::int32_t quality = 10000;
    std::int32_t durability = 10000;
    Time expiresAt = -1;
    Time remainingShelfLife = -1; // Normal-environment seconds; -1 means nonperishable.
    Time decayUpdatedAt = 0;
    std::int32_t decayRemainderBasisPoints = 0;
    std::string provenance;
    std::map<std::string,std::string> properties;
    bool operator==(const Meta&) const = default;
};
struct Cell {
    std::int32_t x = 0, y = 0;
    bool operator==(const Cell&) const = default;
};
struct Definition {
    std::string id, category;
    Count maxStack = 1, unitMassGrams = 0;
    std::int32_t width = 1, height = 1;
    std::vector<Cell> shape; // Empty = full rectangle. Coordinates are relative to width/height.
    std::set<std::string> tags, equipmentSlots;
    std::map<std::string,std::set<std::string>> attachmentSlots;
    std::int32_t bagSlots = 0;
    Count bagMaxMassGrams = 0;
    Time defaultShelfLifeSeconds = -1;
    std::string decaysTo;
    std::map<std::string,Count> repairInputs;
    std::int32_t rarity = 0;
};
struct Recipe {
    std::string id, stationTag;
    std::map<std::string,Count> inputs, outputs, fuelInputs;
    std::set<std::string> requiredTools;
    Time durationSeconds = 0;
    bool requiresUnlock = false;
    std::int32_t minQuality = 0;
    std::int32_t toolDurabilityCost = 0;
};
struct LootEntry { std::string definition; Count minimum = 1, maximum = 1; std::uint32_t weight = 1; };
struct LootTable { std::string id; std::vector<LootEntry> entries; std::uint32_t rolls = 1; };
struct Catalog {
    std::map<std::string,Definition> definitions;
    std::map<std::string,Recipe> recipes;
    std::map<std::string,LootTable> lootTables;
    std::set<std::string> currencies;
};
struct Container {
    Id id = 0;
    std::string owner;
    std::set<std::string> delegates, tags, acceptedTags;
    bool publicAccess = false;
    std::int32_t slots = 32;
    Count maxMassGrams = 100000;
    std::int32_t gridWidth = 0, gridHeight = 0;
    Id parentItem = 0;
    std::map<std::string,Count> money;
    std::int32_t decayRateBasisPoints = BasisPoints; // 0 = suspended decay; 10000 = normal.
    std::int32_t craftQueueLimit = 8;
    bool craftEnabled = true;
};
struct Item {
    Id id = 0, container = 0;
    std::string definition;
    Count quantity = 1;
    Meta meta;
    std::int32_t slot = 0, x = 0, y = 0;
    bool rotated = false;
    Id childContainer = 0;
    Id attachmentContainer = 0;
    bool favorite = false;
    bool locked = false;
};
struct Reservation { Id item = 0; Count quantity = 0; };
struct CraftJob {
    Id id = 0, station = 0, destination = 0;
    std::string owner, recipe;
    Time startedAt = 0, due = 0;
    Count batches = 1;
    std::vector<Reservation> inputs, fuels;
    std::vector<Id> tools;
};
struct Vendor {
    Id id = 0, stock = 0;
    std::string currency;
    std::map<std::string,Count> sells, buys;
    std::set<std::string> refusedTags;
    std::int32_t sellMultiplierBasisPoints = BasisPoints;
    std::int32_t buyMultiplierBasisPoints = BasisPoints;
    Time restockEverySeconds = 0, nextRestockAt = 0;
    std::uint64_t restockGeneration = 0;
    std::map<std::string,Count> restockTargets, restockBatches;
};
struct Source {
    std::string id, table;
    std::uint64_t generation = 0, seed = 1;
    Time availableAt = 0, cooldownSeconds = 0;
    bool exhausted = false, renewable = false;
};
struct Receipt { std::uint64_t sequence = 0, revision = 0; std::vector<std::uint8_t> request; std::vector<Id> created; };
struct State {
    std::uint64_t revision = 0;
    Id nextId = 1;
    Time now = 0;
    std::map<Id,Container> containers;
    std::map<Id,Item> items;
    std::map<Id,CraftJob> jobs;
    std::map<Id,Vendor> vendors;
    std::map<std::string,Source> sources;
    std::map<std::string,std::map<std::int32_t,Id>> hotbars;
    std::map<std::string,std::map<std::string,Id>> equipment;
    std::map<std::string,std::set<std::string>> unlocks;
    std::map<std::string,Receipt> receipts;
    std::set<std::string> awards;
};
// Trusted host input. NEVER accept a client-supplied principal, privilege or permit.
struct Context { std::string actor; bool privileged = false; std::set<Id> vendorPermits; std::set<std::string> sourcePermits; };
struct CreateContainer { Container value; };
struct Grant { Id destination = 0; std::string definition; Count quantity = 1; Meta meta; };
struct Transfer { Id item = 0, destination = 0; Count quantity = 1; std::int32_t slot = -1, x = -1, y = -1; bool rotated = false; };
struct TransferAll { Id source = 0, destination = 0; };
struct QuickStack { Id source = 0, destination = 0; };
struct Merge { Id source = 0, target = 0; Count quantity = 1; };
struct Consume { Id item = 0; Count quantity = 1; };
struct Discard { Id item = 0; Count quantity = 1; };
struct Hotbar { std::int32_t slot = 0; Id item = 0; };
struct Equip { Id item = 0; bool remove = false; };
struct SetItemFlags { Id item = 0; bool setFavorite = false, favorite = false, setLocked = false, locked = false; };
struct Trade { Id vendor = 0, item = 0, customerInventory = 0; Count quantity = 1, quotedTotal = 0; bool buying = true; };
struct RestockVendor { Id vendor = 0; std::uint64_t generation = 0; };
struct StartCraft { std::string recipe; Id station = 0, destination = 0; std::vector<Id> sources; Count batches = 1; };
struct FinishCraft { Id job = 0; };
struct CancelCraft { Id job = 0; };
struct Repair { Id item = 0; std::vector<Id> sources; std::int32_t targetDurability = 10000; };
struct Attach { Id host = 0, attachment = 0; std::string slot; };
struct Detach { Id attachment = 0, destination = 0; std::int32_t slot = -1, x = -1, y = -1; bool rotated = false; };
struct AdvanceTime { Time now = 0; };
struct Unlock { std::string actor, recipe; };
struct RollLoot { std::string source; std::uint64_t generation = 0; Id destination = 0; };
struct Award { std::string eventId; std::vector<Grant> grants; };
using Operation = std::variant<CreateContainer,Grant,Transfer,TransferAll,QuickStack,Merge,Consume,Discard,
    Hotbar,Equip,SetItemFlags,Trade,RestockVendor,StartCraft,FinishCraft,CancelCraft,Repair,Attach,Detach,AdvanceTime,Unlock,RollLoot,Award>;
struct Command { std::uint64_t sequence = 1, expectedRevision = 0; Operation operation; };
struct Quote { Error error = Error::None; Count total = 0; std::uint64_t revision = 0; };

class RBITEMECONOMY_API Engine {
public:
    class Prepared;
    struct Preparation { Result result; std::unique_ptr<Prepared> candidate; };
    explicit Engine(Catalog catalog, State initial = {});
    ~Engine();
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    const State& state() const;
    const Catalog& catalog() const;
    Preparation prepare(const Context&, const Command&) const;
    Result commit(std::unique_ptr<Prepared>);
    Result execute(const Context&, const Command&);
    Quote quote(Id vendor, Id item, Count quantity, bool buying) const;
    Count reserved(Id item) const;
    Count mass(Id container) const;
    bool canAccess(const std::string& actor, Id container) const;
    std::vector<std::uint8_t> snapshot() const;
    Result restore(const std::vector<std::uint8_t>&);
    static Result validate(const Catalog&, const State&);
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
class RBITEMECONOMY_API Engine::Prepared {
public:
    ~Prepared();
private:
    friend class Engine;
    struct Data;
    explicit Prepared(std::unique_ptr<Data>);
    std::unique_ptr<Data> data_;
};
RBITEMECONOMY_API const char* errorName(Error) noexcept;
}

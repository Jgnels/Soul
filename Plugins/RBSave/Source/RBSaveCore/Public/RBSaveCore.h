#pragma once

#include <cstdint>
#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#ifdef RBSAVECORE_API
#define RB_SAVE_CORE_API RBSAVECORE_API
#else
#define RB_SAVE_CORE_API
#endif

namespace rb::save {

using Bytes = std::vector<std::uint8_t>;
using Value = std::variant<std::int64_t, std::uint64_t, double, bool, std::string, Bytes>;

struct DomainRecord {
    std::string id;
    std::uint32_t schemaVersion = 1;
    std::map<std::string, Value> fields;
};

struct Snapshot {
    std::uint32_t formatVersion = 1;
    std::uint64_t sequence = 0;
    std::int64_t createdUtcMs = 0;
    std::string slot;
    std::vector<DomainRecord> domains;
};

struct DecodeResult {
    bool ok = false;
    Snapshot snapshot;
    std::string error;
};

struct IoResult {
    bool ok = false;
    std::string error;
    std::uint32_t checksum = 0;
    std::size_t bytesWritten = 0;
};

struct RepairAudit {
    std::string reason;
    std::uint32_t beforeChecksum = 0;
    std::uint32_t afterChecksum = 0;
    std::vector<std::string> operations;
};

RB_SAVE_CORE_API std::uint32_t Crc32(const std::uint8_t* data, std::size_t size);
RB_SAVE_CORE_API Bytes Encode(const Snapshot& snapshot);
RB_SAVE_CORE_API DecodeResult Decode(const Bytes& bytes);
RB_SAVE_CORE_API std::string ToJson(const Snapshot& snapshot);
RB_SAVE_CORE_API IoResult WriteAtomic(const std::filesystem::path& path, const Snapshot& snapshot);
RB_SAVE_CORE_API DecodeResult ReadFile(const std::filesystem::path& path);
RB_SAVE_CORE_API bool RestoreBackup(const std::filesystem::path& path, std::string& error);

class RB_SAVE_CORE_API MigrationRegistry {
public:
    using StepFn = std::function<bool(DomainRecord&, std::string&)>;

    bool Register(std::string domainId, std::uint32_t fromVersion,
                  std::uint32_t toVersion, StepFn fn, std::string& error);
    bool Migrate(Snapshot& snapshot,
                 const std::map<std::string, std::uint32_t>& targets,
                 std::vector<std::string>& audit,
                 std::string& error) const;

private:
    struct Step {
        std::uint32_t toVersion = 0;
        StepFn fn;
    };
    std::map<std::pair<std::string, std::uint32_t>, Step> steps_;
};

class RB_SAVE_CORE_API RepairTransaction {
public:
    explicit RepairTransaction(Snapshot original);
    bool SetField(const std::string& domainId, const std::string& key, Value value);
    bool RemoveField(const std::string& domainId, const std::string& key);
    bool SetDomainSchema(const std::string& domainId, std::uint32_t version);

    using Validator = std::function<bool(const Snapshot&, std::string&)>;
    IoResult Commit(const std::filesystem::path& path,
                    std::string reason,
                    RepairAudit& audit,
                    Validator validator = {});

    const Snapshot& Working() const { return working_; }
    const Snapshot& Original() const { return original_; }

private:
    DomainRecord* FindDomain(const std::string& domainId);
    Snapshot original_;
    Snapshot working_;
    std::vector<std::string> operations_;
};

RB_SAVE_CORE_API std::optional<std::int64_t> AsInt64(const Value& value);
RB_SAVE_CORE_API std::optional<std::uint64_t> AsUInt64(const Value& value);
RB_SAVE_CORE_API std::optional<bool> AsBool(const Value& value);
RB_SAVE_CORE_API std::optional<std::string> AsString(const Value& value);

} // namespace rb::save

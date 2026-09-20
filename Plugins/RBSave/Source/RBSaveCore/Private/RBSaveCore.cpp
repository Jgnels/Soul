#include "RBSaveCore.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>

#if defined(_WIN32)
#if defined(RBSAVE_UE)
#include "Windows/WindowsHWrapper.h"
#else
#define NOMINMAX
#include <windows.h>
#endif
#endif

namespace rb::save {
namespace {

constexpr std::array<std::uint8_t, 8> kMagic = {'R','B','S','A','V','E','0','1'};
constexpr std::uint32_t kFormatVersion = 1;
constexpr std::uint32_t kMaxDomains = 1'000'000;
constexpr std::uint32_t kMaxFieldsPerDomain = 1'000'000;
constexpr std::uint32_t kMaxValueBytes = 64u * 1024u * 1024u;

template <typename T>
void AppendPod(Bytes& out, const T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto* p = reinterpret_cast<const std::uint8_t*>(&value);
    out.insert(out.end(), p, p + sizeof(T));
}

void AppendString(Bytes& out, const std::string& value) {
    if (value.size() > kMaxValueBytes) throw std::runtime_error("string too large");
    AppendPod(out, static_cast<std::uint32_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

void AppendBytes(Bytes& out, const Bytes& value) {
    if (value.size() > kMaxValueBytes) throw std::runtime_error("blob too large");
    AppendPod(out, static_cast<std::uint32_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

class Reader {
public:
    explicit Reader(const Bytes& bytes) : bytes_(bytes) {}

    template <typename T>
    bool Pod(T& value) {
        if (remaining() < sizeof(T)) return false;
        std::memcpy(&value, bytes_.data() + offset_, sizeof(T));
        offset_ += sizeof(T);
        return true;
    }

    bool String(std::string& value) {
        std::uint32_t size = 0;
        if (!Pod(size) || size > kMaxValueBytes || remaining() < size) return false;
        value.assign(reinterpret_cast<const char*>(bytes_.data() + offset_), size);
        offset_ += size;
        return true;
    }

    bool Blob(Bytes& value) {
        std::uint32_t size = 0;
        if (!Pod(size) || size > kMaxValueBytes || remaining() < size) return false;
        value.assign(bytes_.begin() + offset_, bytes_.begin() + offset_ + size);
        offset_ += size;
        return true;
    }

    std::size_t remaining() const { return bytes_.size() - offset_; }
    std::size_t offset() const { return offset_; }

private:
    const Bytes& bytes_;
    std::size_t offset_ = 0;
};

std::string JsonEscape(const std::string& in) {
    std::ostringstream out;
    for (unsigned char c : in) {
        switch (c) {
        case '"': out << "\\\""; break;
        case '\\': out << "\\\\"; break;
        case '\n': out << "\\n"; break;
        case '\r': out << "\\r"; break;
        case '\t': out << "\\t"; break;
        default:
            if (c < 0x20) {
                out << "\\u" << std::hex << std::setw(4) << std::setfill('0')
                    << static_cast<int>(c) << std::dec;
            } else {
                out << static_cast<char>(c);
            }
        }
    }
    return out.str();
}

std::string ValueSummary(const Value& value) {
    return std::visit([](const auto& v) -> std::string {
        using T = std::decay_t<decltype(v)>;
        if constexpr (std::is_same_v<T, std::string>) {
            return "\"" + JsonEscape(v) + "\"";
        } else if constexpr (std::is_same_v<T, Bytes>) {
            return "\"<" + std::to_string(v.size()) + " bytes>\"";
        } else if constexpr (std::is_same_v<T, bool>) {
            return v ? "true" : "false";
        } else if constexpr (std::is_same_v<T, double>) {
            std::ostringstream s; s << std::setprecision(17) << v; return s.str();
        } else {
            return std::to_string(v);
        }
    }, value);
}

bool ReadAllBytes(const std::filesystem::path& path, Bytes& bytes, std::string& error) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) { error = "unable to open file"; return false; }
    const auto end = file.tellg();
    if (end < 0) { error = "unable to determine file size"; return false; }
    bytes.resize(static_cast<std::size_t>(end));
    file.seekg(0);
    if (!bytes.empty() && !file.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) {
        error = "unable to read full file"; return false;
    }
    return true;
}

bool WriteBytesSync(const std::filesystem::path& path, const Bytes& bytes, std::string& error) {
#ifdef _WIN32
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, nullptr);
    if (h == INVALID_HANDLE_VALUE) { error = "CreateFileW failed"; return false; }
    std::size_t offset = 0;
    while (offset < bytes.size()) {
        const DWORD request = static_cast<DWORD>(std::min<std::size_t>(bytes.size() - offset, 1u << 30));
        DWORD written = 0;
        if (!WriteFile(h, bytes.data() + offset, request, &written, nullptr) || written != request) {
            error = "WriteFile failed"; CloseHandle(h); return false;
        }
        offset += written;
    }
    if (!FlushFileBuffers(h)) { error = "FlushFileBuffers failed"; CloseHandle(h); return false; }
    CloseHandle(h);
    return true;
#else
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) { error = "unable to open output file"; return false; }
    file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    file.flush();
    if (!file) { error = "unable to write output file"; return false; }
    return true;
#endif
}

bool AtomicReplace(const std::filesystem::path& source, const std::filesystem::path& dest,
                   std::string& error) {
#ifdef _WIN32
    if (!MoveFileExW(source.c_str(), dest.c_str(),
                     MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = "MoveFileExW failed";
        return false;
    }
    return true;
#else
    std::error_code ec;
    std::filesystem::rename(source, dest, ec);
    if (ec) { error = "rename failed: " + ec.message(); return false; }
    return true;
#endif
}

DomainRecord* FindDomainMutable(Snapshot& snapshot, const std::string& id) {
    auto it = std::find_if(snapshot.domains.begin(), snapshot.domains.end(),
                           [&](const DomainRecord& d) { return d.id == id; });
    return it == snapshot.domains.end() ? nullptr : &*it;
}

} // namespace

std::uint32_t Crc32(const std::uint8_t* data, std::size_t size) {
    std::uint32_t crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (int bit = 0; bit < 8; ++bit) {
            const std::uint32_t mask = 0u - (crc & 1u);
            crc = (crc >> 1u) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

Bytes Encode(const Snapshot& snapshot) {
    if (snapshot.formatVersion != kFormatVersion) throw std::runtime_error("unsupported format version");
    if (snapshot.domains.size() > kMaxDomains) throw std::runtime_error("too many domains");

    Bytes out;
    out.reserve(1024);
    out.insert(out.end(), kMagic.begin(), kMagic.end());
    AppendPod(out, snapshot.formatVersion);
    AppendPod(out, snapshot.sequence);
    AppendPod(out, snapshot.createdUtcMs);
    AppendString(out, snapshot.slot);

    auto domains = snapshot.domains;
    std::sort(domains.begin(), domains.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
    AppendPod(out, static_cast<std::uint32_t>(domains.size()));
    for (const auto& domain : domains) {
        if (domain.fields.size() > kMaxFieldsPerDomain) throw std::runtime_error("too many fields");
        AppendString(out, domain.id);
        AppendPod(out, domain.schemaVersion);
        AppendPod(out, static_cast<std::uint32_t>(domain.fields.size()));
        for (const auto& [key, value] : domain.fields) {
            AppendString(out, key);
            const auto type = static_cast<std::uint8_t>(value.index() + 1);
            AppendPod(out, type);
            std::visit([&](const auto& v) {
                using T = std::decay_t<decltype(v)>;
                if constexpr (std::is_same_v<T, std::string>) AppendString(out, v);
                else if constexpr (std::is_same_v<T, Bytes>) AppendBytes(out, v);
                else if constexpr (std::is_same_v<T, bool>) {
                    AppendPod(out, static_cast<std::uint8_t>(v ? 1 : 0));
                } else {
                    AppendPod(out, v);
                }
            }, value);
        }
    }

    const auto checksum = Crc32(out.data(), out.size());
    AppendPod(out, checksum);
    return out;
}

DecodeResult Decode(const Bytes& bytes) {
    DecodeResult result;
    if (bytes.size() < kMagic.size() + sizeof(std::uint32_t) * 2) {
        result.error = "snapshot too small"; return result;
    }
    if (!std::equal(kMagic.begin(), kMagic.end(), bytes.begin())) {
        result.error = "invalid magic"; return result;
    }

    std::uint32_t storedChecksum = 0;
    std::memcpy(&storedChecksum, bytes.data() + bytes.size() - sizeof(storedChecksum), sizeof(storedChecksum));
    const auto actualChecksum = Crc32(bytes.data(), bytes.size() - sizeof(storedChecksum));
    if (storedChecksum != actualChecksum) {
        result.error = "checksum mismatch"; return result;
    }

    Bytes body(bytes.begin(), bytes.end() - sizeof(storedChecksum));
    Reader reader(body);
    std::array<std::uint8_t, 8> magic{};
    for (auto& b : magic) if (!reader.Pod(b)) { result.error = "truncated magic"; return result; }

    Snapshot snapshot;
    if (!reader.Pod(snapshot.formatVersion) || snapshot.formatVersion != kFormatVersion) {
        result.error = "unsupported format version"; return result;
    }
    if (!reader.Pod(snapshot.sequence) || !reader.Pod(snapshot.createdUtcMs) || !reader.String(snapshot.slot)) {
        result.error = "truncated header"; return result;
    }

    std::uint32_t domainCount = 0;
    if (!reader.Pod(domainCount) || domainCount > kMaxDomains) {
        result.error = "invalid domain count"; return result;
    }
    snapshot.domains.reserve(domainCount);
    for (std::uint32_t di = 0; di < domainCount; ++di) {
        DomainRecord domain;
        std::uint32_t fieldCount = 0;
        if (!reader.String(domain.id) || !reader.Pod(domain.schemaVersion) ||
            !reader.Pod(fieldCount) || fieldCount > kMaxFieldsPerDomain) {
            result.error = "invalid domain record"; return result;
        }
        if (domain.id.empty() || FindDomainMutable(snapshot, domain.id)) {
            result.error = "empty or duplicate domain id"; return result;
        }

        for (std::uint32_t fi = 0; fi < fieldCount; ++fi) {
            std::string key;
            std::uint8_t type = 0;
            if (!reader.String(key) || key.empty() || !reader.Pod(type) || domain.fields.contains(key)) {
                result.error = "invalid field record"; return result;
            }
            switch (type) {
            case 1: { std::int64_t v{}; if (!reader.Pod(v)) { result.error="truncated int64"; return result; } domain.fields.emplace(key, v); break; }
            case 2: { std::uint64_t v{}; if (!reader.Pod(v)) { result.error="truncated uint64"; return result; } domain.fields.emplace(key, v); break; }
            case 3: { double v{}; if (!reader.Pod(v)) { result.error="truncated double"; return result; } domain.fields.emplace(key, v); break; }
            case 4: {
                std::uint8_t v{};
                if (!reader.Pod(v) || v > 1) { result.error="invalid bool"; return result; }
                domain.fields.emplace(key, v == 1); break;
            }
            case 5: {
                std::string v;
                if (!reader.String(v)) { result.error="truncated string"; return result; }
                domain.fields.emplace(key, std::move(v)); break;
            }
            case 6: {
                Bytes v;
                if (!reader.Blob(v)) { result.error="truncated blob"; return result; }
                domain.fields.emplace(key, std::move(v)); break;
            }
            default: result.error = "unknown value type"; return result;
            }
        }
        snapshot.domains.push_back(std::move(domain));
    }
    if (reader.remaining() != 0) {
        result.error = "unexpected trailing payload"; return result;
    }
    result.ok = true;
    result.snapshot = std::move(snapshot);
    return result;
}

std::string ToJson(const Snapshot& snapshot) {
    std::ostringstream out;
    out << "{\n  \"formatVersion\": " << snapshot.formatVersion
        << ",\n  \"sequence\": " << snapshot.sequence
        << ",\n  \"createdUtcMs\": " << snapshot.createdUtcMs
        << ",\n  \"slot\": \"" << JsonEscape(snapshot.slot) << "\",\n  \"domains\": [";
    auto domains = snapshot.domains;
    std::sort(domains.begin(), domains.end(), [](const auto& a, const auto& b) { return a.id < b.id; });
    for (std::size_t i = 0; i < domains.size(); ++i) {
        const auto& d = domains[i];
        out << (i ? "," : "") << "\n    {\"id\": \"" << JsonEscape(d.id)
            << "\", \"schemaVersion\": " << d.schemaVersion << ", \"fields\": {";
        std::size_t fieldIndex = 0;
        for (const auto& [key, value] : d.fields) {
            out << (fieldIndex++ ? ", " : "") << "\"" << JsonEscape(key)
                << "\": " << ValueSummary(value);
        }
        out << "}}";
    }
    out << "\n  ]\n}\n";
    return out.str();
}

IoResult WriteAtomic(const std::filesystem::path& path, const Snapshot& snapshot) {
    IoResult result;
    Bytes bytes;
    try { bytes = Encode(snapshot); }
    catch (const std::exception& e) { result.error = e.what(); return result; }

    std::error_code ec;
    std::filesystem::create_directories(path.parent_path(), ec);
    if (ec) { result.error = "create_directories failed: " + ec.message(); return result; }

    const auto temp = std::filesystem::path(path.string() + ".tmp");
    const auto backup = std::filesystem::path(path.string() + ".bak");
    const auto backupTemp = std::filesystem::path(path.string() + ".bak.tmp");
    std::filesystem::remove(temp, ec);
    std::filesystem::remove(backupTemp, ec);
    std::string error;
    if (!WriteBytesSync(temp, bytes, error)) {
        result.error = "temporary write failed: " + error; return result;
    }

    if (std::filesystem::exists(path)) {
        Bytes previousBytes;
        if (!ReadAllBytes(path, previousBytes, error)) {
            result.error = "existing snapshot read failed: " + error; return result;
        }
        const auto previous = Decode(previousBytes);
        if (!previous.ok) {
            result.error = "refusing overwrite of invalid existing snapshot: " + previous.error; return result;
        }
        const auto previousCrc = Crc32(previousBytes.data(), previousBytes.size() - sizeof(std::uint32_t));
        const auto historyDir = std::filesystem::path(path.string() + ".history");
        std::filesystem::create_directories(historyDir, ec);
        if (ec) { result.error = "history directory failed: " + ec.message(); return result; }
        std::ostringstream historyName;
        historyName << std::setw(20) << std::setfill('0') << previous.snapshot.sequence
                    << "-" << std::hex << std::uppercase << std::setw(8) << std::setfill('0')
                    << previousCrc << ".rbsave";
        const auto historyPath = historyDir / historyName.str();
        if (!std::filesystem::exists(historyPath)) {
            const auto historyTemp = std::filesystem::path(historyPath.string() + ".tmp");
            if (!WriteBytesSync(historyTemp, previousBytes, error) || !AtomicReplace(historyTemp, historyPath, error)) {
                result.error = "history snapshot failed: " + error; return result;
            }
        }
        std::filesystem::copy_file(path, backupTemp,
                                   std::filesystem::copy_options::overwrite_existing, ec);
        if (ec) { result.error = "backup copy failed: " + ec.message(); return result; }
        if (!AtomicReplace(backupTemp, backup, error)) {
            result.error = "backup replace failed: " + error; return result;
        }
    }

    if (!AtomicReplace(temp, path, error)) {
        result.error = "snapshot replace failed: " + error;
        if (std::filesystem::exists(backup)) {
            std::error_code restoreEc;
            std::filesystem::copy_file(backup, path,
                                       std::filesystem::copy_options::overwrite_existing, restoreEc);
        }
        return result;
    }

    result.ok = true;
    result.bytesWritten = bytes.size();
    result.checksum = Crc32(bytes.data(), bytes.size() - sizeof(std::uint32_t));
    return result;
}

DecodeResult ReadFile(const std::filesystem::path& path) {
    Bytes bytes;
    std::string error;
    if (!ReadAllBytes(path, bytes, error)) return {false, {}, error};
    return Decode(bytes);
}

bool RestoreBackup(const std::filesystem::path& path, std::string& error) {
    const auto backup = std::filesystem::path(path.string() + ".bak");
    if (!std::filesystem::exists(backup)) { error = "backup does not exist"; return false; }

    Bytes bytes;
    if (!ReadAllBytes(backup, bytes, error)) return false;
    const auto decoded = Decode(bytes);
    if (!decoded.ok) { error = "backup invalid: " + decoded.error; return false; }

    const auto temp = std::filesystem::path(path.string() + ".restore.tmp");
    if (!WriteBytesSync(temp, bytes, error)) return false;
    if (!AtomicReplace(temp, path, error)) return false;
    return true;
}

bool MigrationRegistry::Register(std::string domainId, std::uint32_t fromVersion,
                                 std::uint32_t toVersion, StepFn fn, std::string& error) {
    if (domainId.empty() || !fn || toVersion <= fromVersion) {
        error = "invalid migration registration"; return false;
    }
    const auto key = std::make_pair(domainId, fromVersion);
    if (steps_.contains(key)) { error = "duplicate migration step"; return false; }
    steps_.emplace(key, Step{toVersion, std::move(fn)});
    return true;
}

bool MigrationRegistry::Migrate(Snapshot& snapshot,
                                const std::map<std::string, std::uint32_t>& targets,
                                std::vector<std::string>& audit,
                                std::string& error) const {
    for (auto& domain : snapshot.domains) {
        const auto targetIt = targets.find(domain.id);
        if (targetIt == targets.end()) continue;
        const auto target = targetIt->second;
        if (domain.schemaVersion > target) {
            error = "refusing downgrade for domain " + domain.id; return false;
        }
        while (domain.schemaVersion < target) {
            const auto stepIt = steps_.find({domain.id, domain.schemaVersion});
            if (stepIt == steps_.end()) {
                error = "missing migration for " + domain.id + " v" + std::to_string(domain.schemaVersion);
                return false;
            }
            const auto from = domain.schemaVersion;
            std::string stepError;
            if (!stepIt->second.fn(domain, stepError)) {
                error = "migration failed for " + domain.id + ": " + stepError; return false;
            }
            domain.schemaVersion = stepIt->second.toVersion;
            if (domain.schemaVersion > target) {
                error = "migration overshot target for " + domain.id; return false;
            }
            audit.push_back(domain.id + " v" + std::to_string(from) + "->v" +
                            std::to_string(domain.schemaVersion));
        }
    }
    return true;
}

RepairTransaction::RepairTransaction(Snapshot original)
    : original_(std::move(original)), working_(original_) {}

DomainRecord* RepairTransaction::FindDomain(const std::string& domainId) {
    return FindDomainMutable(working_, domainId);
}

bool RepairTransaction::SetField(const std::string& domainId, const std::string& key, Value value) {
    auto* domain = FindDomain(domainId);
    if (!domain || key.empty()) return false;
    domain->fields[key] = std::move(value);
    operations_.push_back("set " + domainId + "." + key);
    return true;
}

bool RepairTransaction::RemoveField(const std::string& domainId, const std::string& key) {
    auto* domain = FindDomain(domainId);
    if (!domain || !domain->fields.erase(key)) return false;
    operations_.push_back("remove " + domainId + "." + key);
    return true;
}

bool RepairTransaction::SetDomainSchema(const std::string& domainId, std::uint32_t version) {
    auto* domain = FindDomain(domainId);
    if (!domain || version == 0) return false;
    domain->schemaVersion = version;
    operations_.push_back("schema " + domainId + "=" + std::to_string(version));
    return true;
}

IoResult RepairTransaction::Commit(const std::filesystem::path& path,
                                   std::string reason,
                                   RepairAudit& audit,
                                   Validator validator) {
    IoResult result;
    if (reason.empty()) { result.error = "repair reason required"; return result; }
    if (operations_.empty()) { result.error = "repair has no operations"; return result; }
    if (validator) {
        std::string validationError;
        if (!validator(working_, validationError)) {
            result.error = "repair validation failed: " + validationError; return result;
        }
    }

    Bytes beforeBytes;
    Bytes afterBytes;
    try {
        beforeBytes = Encode(original_);
        afterBytes = Encode(working_);
    } catch (const std::exception& e) {
        result.error = e.what(); return result;
    }

    audit.reason = std::move(reason);
    audit.beforeChecksum = Crc32(beforeBytes.data(), beforeBytes.size() - sizeof(std::uint32_t));
    audit.afterChecksum = Crc32(afterBytes.data(), afterBytes.size() - sizeof(std::uint32_t));
    audit.operations = operations_;

    result = WriteAtomic(path, working_);
    return result;
}

std::optional<std::int64_t> AsInt64(const Value& value) {
    if (auto p = std::get_if<std::int64_t>(&value)) return *p;
    return std::nullopt;
}

std::optional<std::uint64_t> AsUInt64(const Value& value) {
    if (auto p = std::get_if<std::uint64_t>(&value)) return *p;
    return std::nullopt;
}

std::optional<bool> AsBool(const Value& value) {
    if (auto p = std::get_if<bool>(&value)) return *p;
    return std::nullopt;
}

std::optional<std::string> AsString(const Value& value) {
    if (auto p = std::get_if<std::string>(&value)) return *p;
    return std::nullopt;
}

} // namespace rb::save

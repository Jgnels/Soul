#include "RBSaveGeneration.h"

#include <algorithm>
#include <charconv>
#include <fstream>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>

namespace rb::save {
namespace {

const DomainRecord* FindDomain(const Snapshot& snapshot, const std::string& id) {
    for (const auto& domain : snapshot.domains) {
        if (domain.id == id) return &domain;
    }
    return nullptr;
}

bool GetUInt(const DomainRecord& domain, const std::string& key, std::uint64_t& out) {
    const auto it = domain.fields.find(key);
    if (it == domain.fields.end()) return false;
    const auto value = AsUInt64(it->second);
    if (!value) return false;
    out = *value;
    return true;
}

bool GetString(const DomainRecord& domain, const std::string& key, std::string& out) {
    const auto it = domain.fields.find(key);
    if (it == domain.fields.end()) return false;
    const auto value = AsString(it->second);
    if (!value) return false;
    out = *value;
    return true;
}

bool ValidateManifest(const GenerationManifest& manifest, std::string& error) {
    if (manifest.formatVersion < 1 || manifest.formatVersion > 2) {
        error = "unsupported generation manifest version";
        return false;
    }
    if (manifest.generation == 0 || manifest.logicalSlot.empty()) {
        error = "generation and logical slot are required";
        return false;
    }
    if (manifest.artifacts.empty()) {
        error = "generation has no artifacts";
        return false;
    }
    std::set<std::string> roles;
    for (const auto& artifact : manifest.artifacts) {
        if (artifact.role.empty() || artifact.backend.empty() || artifact.locator.empty()) {
            error = "artifact role, backend and locator are required";
            return false;
        }
        if (manifest.formatVersion >= 2 && artifact.path.empty()) {
            error = "manifest v2 artifact path is required";
            return false;
        }
        if (!roles.insert(artifact.role).second) {
            error = "duplicate artifact role: " + artifact.role;
            return false;
        }
    }
    return true;
}

bool VerifyArtifacts(const GenerationManifest& manifest,
                     const ArtifactVerifier& verifier,
                     std::string& error) {
    if (!verifier) return true;
    for (const auto& artifact : manifest.artifacts) {
        std::string artifactError;
        if (!verifier(artifact, artifactError)) {
            error = "artifact " + artifact.role + " failed verification";
            if (!artifactError.empty()) error += ": " + artifactError;
            return false;
        }
    }
    return true;
}

Snapshot CurrentPointerSnapshot(const GenerationManifest& manifest) {
    Snapshot snapshot;
    snapshot.sequence = manifest.generation;
    snapshot.createdUtcMs = manifest.createdUtcMs;
    snapshot.slot = manifest.logicalSlot;
    DomainRecord current;
    current.id = "rb.current";
    current.fields["Generation"] = manifest.generation;
    current.fields["LogicalSlot"] = manifest.logicalSlot;
    snapshot.domains.push_back(std::move(current));
    return snapshot;
}

GenerationResult WriteCurrentPointer(const std::filesystem::path& root,
                                     const GenerationManifest& manifest) {
    GenerationResult result;
    result.generation = manifest.generation;
    result.manifestPath = GenerationManifestPath(root, manifest.generation);
    const auto write = WriteAtomic(root / "current.rbsave", CurrentPointerSnapshot(manifest));
    if (!write.ok) {
        result.error = "unable to activate generation: " + write.error;
        return result;
    }
    result.ok = true;
    result.manifestChecksum = write.checksum;
    return result;
}

bool ReadCurrentNumber(const std::filesystem::path& root,
                       std::uint64_t& generation,
                       std::string& logicalSlot,
                       std::string& error) {
    const auto current = ReadFile(root / "current.rbsave");
    if (!current.ok) {
        error = "unable to read current generation: " + current.error;
        return false;
    }
    const auto* domain = FindDomain(current.snapshot, "rb.current");
    if (!domain || !GetUInt(*domain, "Generation", generation) ||
        !GetString(*domain, "LogicalSlot", logicalSlot)) {
        error = "current generation pointer is malformed";
        return false;
    }
    if (generation == 0 || logicalSlot.empty()) {
        error = "current generation pointer is invalid";
        return false;
    }
    return true;
}

} // namespace

FileFingerprint FingerprintFile(const std::filesystem::path& path) {
    FileFingerprint result;
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        result.error = "unable to open file";
        return result;
    }
    const auto end = file.tellg();
    if (end < 0) {
        result.error = "unable to determine file size";
        return result;
    }
    const auto size = static_cast<std::uint64_t>(end);
    if (size > static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
        result.error = "file too large to fingerprint";
        return result;
    }
    Bytes bytes(static_cast<std::size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!bytes.empty() && !file.read(reinterpret_cast<char*>(bytes.data()),
                                     static_cast<std::streamsize>(bytes.size()))) {
        result.error = "unable to read file";
        return result;
    }
    result.bytes = size;
    result.checksum = Crc32(bytes.data(), bytes.size());
    result.ok = true;
    return result;
}

bool VerifyFileFingerprint(const GenerationArtifact& artifact,
                           const std::filesystem::path& path,
                           std::string& error) {
    const auto actual = FingerprintFile(path);
    if (!actual.ok) {
        error = actual.error;
        return false;
    }
    if (actual.bytes != artifact.bytes) {
        error = "byte length mismatch";
        return false;
    }
    if (actual.checksum != artifact.checksum) {
        error = "checksum mismatch";
        return false;
    }
    return true;
}

Snapshot ManifestToSnapshot(const GenerationManifest& manifest) {
    Snapshot snapshot;
    snapshot.sequence = manifest.generation;
    snapshot.createdUtcMs = manifest.createdUtcMs;
    snapshot.slot = manifest.logicalSlot;

    DomainRecord header;
    header.id = "rb.generation";
    header.fields["FormatVersion"] = std::uint64_t{manifest.formatVersion};
    header.fields["Generation"] = manifest.generation;
    header.fields["LogicalSlot"] = manifest.logicalSlot;
    header.fields["ArtifactCount"] = std::uint64_t{manifest.artifacts.size()};
    snapshot.domains.push_back(std::move(header));

    auto artifacts = manifest.artifacts;
    std::sort(artifacts.begin(), artifacts.end(),
              [](const GenerationArtifact& a, const GenerationArtifact& b) {
                  return a.role < b.role;
              });
    for (std::size_t index = 0; index < artifacts.size(); ++index) {
        const auto& artifact = artifacts[index];
        std::ostringstream id;
        id << "rb.artifact." << std::setw(4) << std::setfill('0') << index;
        DomainRecord domain;
        domain.id = id.str();
        domain.fields["Role"] = artifact.role;
        domain.fields["Backend"] = artifact.backend;
        domain.fields["Locator"] = artifact.locator;
        if (!artifact.path.empty()) domain.fields["Path"] = artifact.path;
        domain.fields["Bytes"] = artifact.bytes;
        domain.fields["Checksum"] = std::uint64_t{artifact.checksum};
        snapshot.domains.push_back(std::move(domain));
    }
    return snapshot;
}

bool ManifestFromSnapshot(const Snapshot& snapshot,
                          GenerationManifest& manifest,
                          std::string& error) {
    const auto* header = FindDomain(snapshot, "rb.generation");
    if (!header) {
        error = "generation header missing";
        return false;
    }
    std::uint64_t version = 0;
    std::uint64_t generation = 0;
    std::uint64_t artifactCount = 0;
    std::string logicalSlot;
    if (!GetUInt(*header, "FormatVersion", version) ||
        !GetUInt(*header, "Generation", generation) ||
        !GetUInt(*header, "ArtifactCount", artifactCount) ||
        !GetString(*header, "LogicalSlot", logicalSlot)) {
        error = "generation header malformed";
        return false;
    }
    if (version > std::numeric_limits<std::uint32_t>::max()) {
        error = "generation format version out of range";
        return false;
    }

    GenerationManifest parsed;
    parsed.formatVersion = static_cast<std::uint32_t>(version);
    parsed.generation = generation;
    parsed.createdUtcMs = snapshot.createdUtcMs;
    parsed.logicalSlot = std::move(logicalSlot);

    for (const auto& domain : snapshot.domains) {
        if (!domain.id.starts_with("rb.artifact.")) continue;
        GenerationArtifact artifact;
        std::uint64_t checksum = 0;
        if (!GetString(domain, "Role", artifact.role) ||
            !GetString(domain, "Backend", artifact.backend) ||
            !GetString(domain, "Locator", artifact.locator) ||
            !GetUInt(domain, "Bytes", artifact.bytes) ||
            !GetUInt(domain, "Checksum", checksum)) {
            error = "artifact record malformed";
            return false;
        }
        if (checksum > std::numeric_limits<std::uint32_t>::max()) {
            error = "artifact checksum out of range";
            return false;
        }
        artifact.checksum = static_cast<std::uint32_t>(checksum);
        if (parsed.formatVersion >= 2 && !GetString(domain, "Path", artifact.path)) {
            error = "manifest v2 artifact path missing";
            return false;
        }
        if (parsed.formatVersion == 1) GetString(domain, "Path", artifact.path);
        parsed.artifacts.push_back(std::move(artifact));
    }
    if (parsed.artifacts.size() != artifactCount) {
        error = "artifact count mismatch";
        return false;
    }
    if (snapshot.sequence != parsed.generation || snapshot.slot != parsed.logicalSlot) {
        error = "generation envelope mismatch";
        return false;
    }
    std::sort(parsed.artifacts.begin(), parsed.artifacts.end(),
              [](const GenerationArtifact& a, const GenerationArtifact& b) {
                  return a.role < b.role;
              });
    if (!ValidateManifest(parsed, error)) return false;
    manifest = std::move(parsed);
    return true;
}

std::filesystem::path GenerationManifestPath(const std::filesystem::path& root,
                                             std::uint64_t generation) {
    std::ostringstream name;
    name << std::setw(20) << std::setfill('0') << generation;
    return root / "generations" / name.str() / "manifest.rbsave";
}

GenerationResult CommitGeneration(const std::filesystem::path& root,
                                  const GenerationManifest& manifest,
                                  ArtifactVerifier verifier) {
    GenerationResult result;
    result.generation = manifest.generation;
    result.manifestPath = GenerationManifestPath(root, manifest.generation);

    std::string error;
    if (!ValidateManifest(manifest, error)) {
        result.error = std::move(error);
        return result;
    }
    if (!VerifyArtifacts(manifest, verifier, error)) {
        result.error = std::move(error);
        return result;
    }
    if (std::filesystem::exists(result.manifestPath)) {
        result.error = "generation already exists";
        return result;
    }

    std::error_code ec;
    std::filesystem::create_directories(result.manifestPath.parent_path(), ec);
    if (ec) {
        result.error = "unable to create generation directory: " + ec.message();
        return result;
    }
    const auto write = WriteAtomic(result.manifestPath, ManifestToSnapshot(manifest));
    if (!write.ok) {
        result.error = "unable to write generation manifest: " + write.error;
        return result;
    }
    const auto reread = ReadFile(result.manifestPath);
    GenerationManifest verified;
    if (!reread.ok || !ManifestFromSnapshot(reread.snapshot, verified, error) ||
        verified.generation != manifest.generation || verified.logicalSlot != manifest.logicalSlot) {
        result.error = "generation manifest failed read-back verification";
        if (!error.empty()) result.error += ": " + error;
        return result;
    }

    result = WriteCurrentPointer(root, manifest);
    if (!result.ok) return result;
    result.manifestChecksum = write.checksum;
    result.manifestPath = GenerationManifestPath(root, manifest.generation);
    return result;
}

GenerationResult ActivateGeneration(const std::filesystem::path& root,
                                    std::uint64_t generation,
                                    ArtifactVerifier verifier) {
    GenerationResult result;
    result.generation = generation;
    result.manifestPath = GenerationManifestPath(root, generation);
    const auto loaded = ReadFile(result.manifestPath);
    if (!loaded.ok) {
        result.error = "unable to read generation manifest: " + loaded.error;
        return result;
    }
    GenerationManifest manifest;
    std::string error;
    if (!ManifestFromSnapshot(loaded.snapshot, manifest, error)) {
        result.error = "generation manifest invalid: " + error;
        return result;
    }
    if (!VerifyArtifacts(manifest, verifier, error)) {
        result.error = std::move(error);
        return result;
    }
    return WriteCurrentPointer(root, manifest);
}

std::optional<GenerationManifest> ReadActiveGeneration(
    const std::filesystem::path& root,
    std::string& error,
    ArtifactVerifier verifier) {
    std::uint64_t generation = 0;
    std::string logicalSlot;
    if (!ReadCurrentNumber(root, generation, logicalSlot, error)) {
        return std::nullopt;
    }
    const auto loaded = ReadFile(GenerationManifestPath(root, generation));
    if (!loaded.ok) {
        error = "active generation manifest unreadable: " + loaded.error;
        return std::nullopt;
    }
    GenerationManifest manifest;
    if (!ManifestFromSnapshot(loaded.snapshot, manifest, error)) {
        return std::nullopt;
    }
    if (manifest.generation != generation || manifest.logicalSlot != logicalSlot) {
        error = "active generation does not match current pointer";
        return std::nullopt;
    }
    if (!VerifyArtifacts(manifest, verifier, error)) {
        return std::nullopt;
    }
    return manifest;
}


std::vector<std::uint64_t> ListGenerations(const std::filesystem::path& root,
                                            std::string& error,
                                            std::size_t maxEntries) {
    error.clear();
    std::vector<std::uint64_t> result;
    const auto dir = root / "generations";
    std::error_code ec;
    if (!std::filesystem::exists(dir, ec)) return result;
    if (ec) { error = "unable to inspect generations: " + ec.message(); return {}; }
    std::size_t visited = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (ec) { error = "unable to enumerate generations: " + ec.message(); return {}; }
        if (!entry.is_directory()) continue;
        if (++visited > maxEntries) { error = "generation scan limit exceeded"; return {}; }
        const auto name = entry.path().filename().string();
        std::uint64_t number = 0;
        const auto parsed = std::from_chars(name.data(), name.data() + name.size(), number, 10);
        if (parsed.ec == std::errc{} && parsed.ptr == name.data() + name.size() && number > 0)
            result.push_back(number);
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::uint64_t NextGenerationNumber(const std::filesystem::path& root,
                                   std::string& error,
                                   std::size_t maxEntries) {
    const auto generations = ListGenerations(root, error, maxEntries);
    if (!error.empty()) return 0;
    if (generations.empty()) return 1;
    if (generations.back() == std::numeric_limits<std::uint64_t>::max()) {
        error = "generation counter exhausted";
        return 0;
    }
    return generations.back() + 1;
}

} // namespace rb::save

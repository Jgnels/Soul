#pragma once

#include "RBSaveCore.h"

#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace rb::save {

struct GenerationArtifact {
    std::string role;
    std::string backend;
    std::string locator;
    std::string path;
    std::uint64_t bytes = 0;
    std::uint32_t checksum = 0;

    GenerationArtifact() = default;
    GenerationArtifact(std::string inRole, std::string inBackend, std::string inLocator,
                       std::uint64_t inBytes, std::uint32_t inChecksum)
        : role(std::move(inRole)), backend(std::move(inBackend)), locator(inLocator),
          path(std::move(inLocator)), bytes(inBytes), checksum(inChecksum) {}
    GenerationArtifact(std::string inRole, std::string inBackend, std::string inLocator,
                       std::string inPath, std::uint64_t inBytes, std::uint32_t inChecksum)
        : role(std::move(inRole)), backend(std::move(inBackend)), locator(std::move(inLocator)),
          path(std::move(inPath)), bytes(inBytes), checksum(inChecksum) {}
};
struct GenerationManifest {
    std::uint32_t formatVersion = 2;
    std::uint64_t generation = 0;
    std::int64_t createdUtcMs = 0;
    std::string logicalSlot;
    std::vector<GenerationArtifact> artifacts;
};

struct FileFingerprint {
    bool ok = false;
    std::uint64_t bytes = 0;
    std::uint32_t checksum = 0;
    std::string error;
};

struct GenerationResult {
    bool ok = false;
    std::uint64_t generation = 0;
    std::uint32_t manifestChecksum = 0;
    std::filesystem::path manifestPath;
    std::string error;
};

using ArtifactVerifier = std::function<bool(const GenerationArtifact&, std::string&)>;

RB_SAVE_CORE_API FileFingerprint FingerprintFile(const std::filesystem::path& path);
RB_SAVE_CORE_API bool VerifyFileFingerprint(const GenerationArtifact& artifact,
                                            const std::filesystem::path& path,
                                            std::string& error);RB_SAVE_CORE_API Snapshot ManifestToSnapshot(const GenerationManifest& manifest);
RB_SAVE_CORE_API bool ManifestFromSnapshot(const Snapshot& snapshot,
                                           GenerationManifest& manifest,
                                           std::string& error);

RB_SAVE_CORE_API std::filesystem::path GenerationManifestPath(
    const std::filesystem::path& root, std::uint64_t generation);

RB_SAVE_CORE_API GenerationResult CommitGeneration(
    const std::filesystem::path& root, const GenerationManifest& manifest,
    ArtifactVerifier verifier = {});

RB_SAVE_CORE_API GenerationResult ActivateGeneration(
    const std::filesystem::path& root, std::uint64_t generation,
    ArtifactVerifier verifier = {});

RB_SAVE_CORE_API std::optional<GenerationManifest> ReadActiveGeneration(
    const std::filesystem::path& root, std::string& error,
    ArtifactVerifier verifier = {});

RB_SAVE_CORE_API std::vector<std::uint64_t> ListGenerations(
    const std::filesystem::path& root, std::string& error,
    std::size_t maxEntries = 1000);

RB_SAVE_CORE_API std::uint64_t NextGenerationNumber(
    const std::filesystem::path& root, std::string& error,
    std::size_t maxEntries = 1000);

} // namespace rb::save

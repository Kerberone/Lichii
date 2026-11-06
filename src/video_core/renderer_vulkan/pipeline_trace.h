// SPDX-FileCopyrightText: Copyright 2026 Eden Emulator Project
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <array>
#include <chrono>
#include <cstdlib>
#include <string>

#include <fmt/format.h>

#include "common/common_types.h"

// EDEN_NX2_PIPE_TRACE=1 logs every pipeline build as a begin line before the work and a done or
// fail line after it, so a begin without its end names the pipeline a translation, a SPIR-V
// emission or a driver compile hung or crashed on. Lines are logged at Error level, which the
// file backend flushes at once, so they reach the log before a crash.
//
// Phases: cache (disk cache load), prebuild (background prebuild of cached pipelines), rt
// (translation on the GPU thread at run time), emit (SPIR-V emission on a worker), drv (driver
// vkCreate*Pipelines).
namespace Vulkan::PipeTrace {

inline bool Enabled() {
    static const bool enabled = [] {
        const char* value = std::getenv("EDEN_NX2_PIPE_TRACE");
        return value != nullptr && value[0] != '\0' && value[0] != '0';
    }();
    return enabled;
}

inline double Ms(std::chrono::steady_clock::time_point begin) {
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - begin)
        .count();
}

/// Stage hashes of a graphics key (vertex A, vertex B, tess control, tess eval, geometry, pixel).
template <size_t N>
std::string Stages(const std::array<u64, N>& hashes) {
    static constexpr std::array<const char*, 6> names{"vsa", "vs", "tcs", "tes", "gs", "ps"};
    std::string result;
    for (size_t stage = 0; stage < N; ++stage) {
        if (hashes[stage] != 0) {
            result += fmt::format(" {}={:016x}", stage < names.size() ? names[stage] : "s",
                                  hashes[stage]);
        }
    }
    return result;
}

} // namespace Vulkan::PipeTrace

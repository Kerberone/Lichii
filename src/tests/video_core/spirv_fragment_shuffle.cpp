// SPDX-License-Identifier: GPL-3.0-or-later

#include <vector>
#include <catch2/catch_test_macros.hpp>

#include "shader_recompiler/backend/spirv/emit_spirv.h"
#include "shader_recompiler/frontend/ir/ir_emitter.h"

namespace {
using namespace Shader;

constexpr u32 OpGroupNonUniformBroadcast = 337;
constexpr u32 OpGroupNonUniformShuffle = 345;
constexpr u32 OpGroupNonUniformShuffleXor = 346;
constexpr u32 OpGroupNonUniformShuffleUp = 347;
constexpr u32 OpGroupNonUniformShuffleDown = 348;

std::vector<u32> EmitShuffles(Stage stage, bool fallback, u32 max_subgroup_size = 32) {
    ObjectPool<IR::Inst> pool;
    IR::Block block{pool};
    IR::Program program;
    program.stage = stage;
    program.workgroup_size = {32, 1, 1};
    program.blocks = {&block};
    program.post_order_blocks = {&block};
    program.syntax_list = {{{.block = &block}, IR::AbstractSyntaxNode::Type::Block},
                           {{}, IR::AbstractSyntaxNode::Type::Return}};
    program.info.uses_subgroup_shuffles = true;
    program.info.uses_subgroup_invocation_id = true;

    Profile profile{};
    profile.supported_spirv = 0x00010500;
    profile.supported_subgroup_stages = 1U << static_cast<u32>(stage);
    profile.support_shuffle_relative = true;
    profile.max_subgroup_size = max_subgroup_size;
    profile.warp_size_potentially_larger_than_guest = max_subgroup_size > 32;
    profile.fragment_subgroup_shuffle_fallback = fallback;

    IR::IREmitter ir{block};
    const IR::U32 lane = ir.LaneId();
    const IR::U32 clamp = ir.Imm32(31);
    const IR::U32 seg_mask = ir.Imm32(0);
    // Dynamic indices keep every shuffle on the generic path.
    static_cast<void>(ir.ShuffleIndex(lane, lane, clamp, seg_mask));
    static_cast<void>(ir.ShuffleUp(lane, lane, clamp, seg_mask));
    static_cast<void>(ir.ShuffleDown(lane, lane, clamp, seg_mask));
    static_cast<void>(ir.ShuffleButterfly(lane, lane, clamp, seg_mask));
    return Backend::SPIRV::EmitSPIRV(profile, program);
}

u32 CountOps(const std::vector<u32>& code, u32 opcode) {
    u32 count = 0;
    for (size_t offset = 5; offset < code.size();) {
        const u32 size = code[offset] >> 16;
        REQUIRE(size != 0);
        if ((code[offset] & 0xffffU) == opcode) {
            ++count;
        }
        offset += size;
    }
    return count;
}

u32 CountNativeShuffles(const std::vector<u32>& code) {
    return CountOps(code, OpGroupNonUniformShuffle) + CountOps(code, OpGroupNonUniformShuffleXor) +
           CountOps(code, OpGroupNonUniformShuffleUp) +
           CountOps(code, OpGroupNonUniformShuffleDown);
}
} // Anonymous namespace

TEST_CASE("Fragment shuffle fallback replaces native shuffles with broadcasts",
          "[video_core][spirv][shuffle]") {
    const auto native = EmitShuffles(Stage::Fragment, false);
    REQUIRE(CountNativeShuffles(native) > 0);

    const auto fallback = EmitShuffles(Stage::Fragment, true);
    CHECK(CountNativeShuffles(fallback) == 0);
    // One broadcast per subgroup lane for each of the four shuffles.
    CHECK(CountOps(fallback, OpGroupNonUniformBroadcast) >= 4 * 32);
}

TEST_CASE("Fragment shuffle fallback covers every lane of a Wave64 subgroup",
          "[video_core][spirv][shuffle]") {
    const auto fallback = EmitShuffles(Stage::Fragment, true, 64);
    CHECK(CountNativeShuffles(fallback) == 0);
    CHECK(CountOps(fallback, OpGroupNonUniformBroadcast) >= 4 * 64);
}

TEST_CASE("Fragment shuffle fallback leaves other stages untouched",
          "[video_core][spirv][shuffle]") {
    for (const Stage stage : {Stage::Compute, Stage::VertexB}) {
        const auto off = EmitShuffles(stage, false);
        const auto on = EmitShuffles(stage, true);
        CHECK(on == off);
        CHECK(CountNativeShuffles(on) > 0);
    }
}

TEST_CASE("Fragment shuffle fallback is off by default", "[video_core][spirv][shuffle]") {
    CHECK_FALSE(Profile{}.fragment_subgroup_shuffle_fallback);
}

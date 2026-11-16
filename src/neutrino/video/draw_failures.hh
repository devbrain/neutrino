//
// Created by igor on 16/11/2026.
//

#pragma once

namespace neutrino::detail {
    /// Counts one failure in draw_failure_count(), for engine code outside draw.cc whose failure
    /// the counter must include: a render target the texture allocator cannot make (painter.md
    /// §2.15). The caller logs it.
    void count_draw_failure() noexcept;
}

#pragma once

namespace levk::thread {
/// \brief Open set of monotonically increasing thread index.
enum struct Index : int { eMain = 0 };

/// \brief Associate the main thread id with its Index.
/// Must be called from the main thread.
void initialize();
/// \brief Get this thread's Index.
/// \returns This thread's Index.
[[nodiscard]] auto get_index() -> Index;
} // namespace levk::thread

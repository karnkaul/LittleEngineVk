#pragma once
#include <capo/pcm.hpp>
#include <le/resources/asset.hpp>

namespace le {
class PcmAsset : public Asset {
  public:
	[[nodiscard]] auto try_load(Uri const& uri) -> bool override;

	capo::Pcm pcm{};
	capo::Compression compression{};
};
} // namespace le

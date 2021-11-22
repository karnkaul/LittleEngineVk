#include <core/utils/expect.hpp>
#include <graphics/render/pipeline_factory.hpp>
#include <graphics/utils/utils.hpp>

namespace le::graphics {
std::size_t const PipelineSpec::default_hash_v = PipelineSpec{}.hash();

Hash PipelineSpec::hash() const { return Hasher{}(*this); }

std::size_t PipelineSpec::Hasher::operator()(PipelineSpec const& spec) const {
	utils::HashGen ret;
	ret << spec;
	return ret;
}
} // namespace le::graphics

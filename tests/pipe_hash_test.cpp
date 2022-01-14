#include <dumb_test/dtest.hpp>
#include <levk/graphics/utils/utils.hpp>
#include <random>

using namespace le::graphics;

namespace {
template <typename T>
std::size_t hash(T const& t) {
	utils::HashGen hg;
	hg << t;
	return hg;
}

TEST(blank) {
	PipelineSpec spec;
	EXPECT_EQ(hash(spec), PipelineSpec::default_hash_v);
	ShaderSpec ss;
	EXPECT_EQ(hash(ss), std::size_t(0));
}

TEST(shader) {
	ShaderSpec ss;
	ss.moduleURIs.push_back("shader.vert");
	ss.moduleURIs.push_back("shader.frag");
	EXPECT_NE(hash(ss), std::size_t(0));
	auto copy = ss;
	EXPECT_EQ(hash(ss), hash(copy));
}

TEST(fixed_state) {
	FixedStateSpec fs;
	auto copy = fs;
	EXPECT_EQ(hash(fs), hash(copy));
	fs.lineWidth = 3.0f;
	EXPECT_NE(hash(fs), hash(copy));
	copy = fs;
	EXPECT_EQ(hash(fs), hash(copy));
	fs.mode = vk::PolygonMode(1);
	fs.topology = vk::PrimitiveTopology(int(fs.mode));
	EXPECT_NE(hash(fs), hash(copy));
}

VertexInputInfo build() {
	VertexInputInfo vi;
	vk::VertexInputAttributeDescription vad;
	vk::VertexInputBindingDescription vbd;
	vbd.binding = 1;
	vbd.stride = sizeof(Vertex);
	vbd.inputRate = vk::VertexInputRate::eVertex;
	vi.bindings.push_back(vbd);
	vad.binding = 1;
	vad.location = 0;
	vad.format = vk::Format::eR32G32B32Sfloat;
	vad.offset = 0;
	vi.attributes.push_back(vad);
	vad.binding = 1;
	vad.location = 1;
	vad.format = vk::Format::eR32G32Sfloat;
	vad.offset = sizeof(glm::vec3);
	vi.attributes.push_back(vad);
	return vi;
}

TEST(vertex_input) {
	VertexInputInfo vi;
	EXPECT_EQ(hash(vi), std::size_t(0));
	vk::VertexInputAttributeDescription vad;
	vk::VertexInputBindingDescription vbd;
	vi.attributes.push_back(vad);
	vi.bindings.push_back(vbd);
	EXPECT_NE(hash(vi), hash(VertexInputInfo{}));
	auto copy = vi;
	EXPECT_EQ(hash(vi), hash(copy));
	vi = build();
	EXPECT_NE(hash(vi), hash(copy));
	copy = vi;
	EXPECT_EQ(hash(vi), hash(copy));
}

TEST(pipe_hash) {
	PipelineSpec ps;
	ps.vertexInput = build();
	ps.shader.moduleURIs.push_back("shader.vert");
	ps.fixedState.flags = PFlags(PFlag::eAlphaBlend, PFlag::eDepthTest, PFlag::eDepthWrite);
	ps.fixedState.dynamicStates = {std::begin(FixedStateSpec::required_states_v), std::end(FixedStateSpec::required_states_v)};
	EXPECT_NE(hash(ps), PipelineSpec::default_hash_v);
	auto copy = ps;
	EXPECT_EQ(hash(ps), hash(copy));
	ps.fixedState.lineWidth = 3.0;
	EXPECT_NE(hash(ps), hash(copy));
	copy = ps;
	EXPECT_EQ(hash(ps), hash(copy));
}
} // namespace

#include <stb/stb_image.h>
#include <glm/vec3.hpp>
#include <le/graphics/image_file.hpp>

namespace le::graphics {
struct ImageFile::Impl {
	stbi_uc* stb_data{};
	std::size_t size{};
	glm::uvec2 extent{};
	int channels{};
};

auto ImageFile::Deleter::operator()(Impl* ptr) const -> void {
	stbi_image_free(ptr->stb_data);
	std::default_delete<Impl>{}(ptr);
}

auto ImageFile::decompress(std::span<std::byte const> compressed) -> bool {
	if (compressed.empty()) { return false; }
	auto extent = glm::ivec3{};
	auto const* stbi_data = reinterpret_cast<stbi_uc const*>(compressed.data()); // NOLINT
	auto* ptr = stbi_load_from_memory(stbi_data, static_cast<int>(compressed.size_bytes()), &extent.x, &extent.y, &extent.z, 4);
	if (ptr == nullptr) { return false; }
	auto const size = static_cast<std::size_t>(extent.x * extent.y * 4);
	m_impl = std::unique_ptr<Impl, Deleter>{new Impl{ptr, size, extent, 4}};
	return true;
}

auto ImageFile::bitmap() const -> Bitmap {
	if (!m_impl) { return {}; }

	return Bitmap{
		.bytes = {reinterpret_cast<std::byte const*>(m_impl->stb_data), m_impl->size}, // NOLINT
		.extent = m_impl->extent,
	};
}
} // namespace le::graphics

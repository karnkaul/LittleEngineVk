#pragma once
#include <levk/core/flex_array.hpp>
#include <levk/core/polymorphic.hpp>
#include <levk/descriptor_info.hpp>
#include <levk/render_shader.hpp>
#include <string>

namespace levk {
/// \brief Abtract base for materials.
class IMaterial : public Polymorphic {
  public:
	using DescriptorBuffer = FlexArray<DescriptorInfo, 8>;

	explicit IMaterial(RenderShader const& fragment_shader) : fragment_shader(fragment_shader) {}

	[[nodiscard]] virtual auto get_type_name() const -> std::string_view = 0;

	/// \brief Push DescriptorInfos to write, if any.
	/// \param descriptor_buffer Buffer to push into.
	virtual void push_descriptors(DescriptorBuffer& descriptor_buffer) const = 0;

	/// \brief Fragment shader for this material.
	RenderShader fragment_shader{};

	std::string name{};
};
} // namespace levk

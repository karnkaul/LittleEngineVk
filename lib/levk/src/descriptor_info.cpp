#include <levk/core/visitor.hpp>
#include <levk/descriptor_info.hpp>

namespace levk {
auto DescriptorInfo::get_write_descriptor_set(vk::DescriptorSet const descriptor_set) const -> vk::WriteDescriptorSet {
	auto const visitor = Visitor{
		[this, descriptor_set](vk::DescriptorBufferInfo const& dbi) {
			auto ret = vk::WriteDescriptorSet{descriptor_set, binding, 0, 1, type};
			ret.pBufferInfo = &dbi;
			return ret;
		},
		[this, descriptor_set](vk::DescriptorImageInfo const& dii) { return vk::WriteDescriptorSet{descriptor_set, binding, 0, 1, type, &dii}; },
		[](std::monostate) { return vk::WriteDescriptorSet{}; },
	};
	return std::visit(visitor, payload);
}
} // namespace levk

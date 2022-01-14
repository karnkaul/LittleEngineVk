#include <core/utils/expect.hpp>
#include <levk/engine/render/descriptor_helper.hpp>
#include <levk/engine/render/drawable.hpp>
#include <algorithm>
namespace le {
DescriptorUpdater DrawBindable::set(DescriptorMap& map, u32 set) const {
	if (std::find(sets.begin(), sets.end(), set) == sets.end()) {
		EXPECT(sets.has_space());
		sets.push_back(set);
	}
	return map.set(set);
}
} // namespace le

#pragma once
#include <levk/node_tree.hpp>
#include <levk/transform_sampler.hpp>

namespace levk {
/// \brief Animation channel for a TreeAnimation.
/// A channel connects a source AnimationSampler to a target TreeNode.
struct AnimationChannel {
	TransformSampler sampler{};
	ImportIndex target{};
};

/// \brief Collection of AnimationChannels with a shared timeline clock.
template <std::derived_from<TreeNode> NodeTypeT>
class TreeAnimation {
  public:
	/// \brief Get the duration of this animation.
	[[nodiscard]] auto get_duration() const -> Seconds { return m_duration; }

	/// \brief Add a channel.
	void add_channel(AnimationChannel channel) {
		m_duration = std::max(m_duration, channel.sampler.get_exit());
		m_channels.push_back(std::move(channel));
	}

	/// \brief Update all nodes targeted by stored channels.
	/// \param node_tree Owning tree.
	/// \param dt Delta time.
	void update(BasicNodeTree<NodeTypeT>& node_tree, Seconds const elapsed) const {
		for (auto const& channel : m_channels) {
			auto* entity = node_tree.find_node_by_index(channel.target);
			if (entity == nullptr) { continue; }
			channel.sampler.update(entity->transform, elapsed);
		}
	}

	std::string name{};

  private:
	std::vector<AnimationChannel> m_channels{};
	Seconds m_duration{};
};
} // namespace levk

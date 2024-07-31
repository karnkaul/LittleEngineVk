#pragma once
#include <levk/animation_sampler.hpp>
#include <levk/transform.hpp>
#include <variant>

namespace levk {
/// \brief Animation sampler that affects a Transform.
class TransformSampler {
  public:
	/// \brief Interpolator for vec3 and quat.
	struct Interpolator {
		[[nodiscard]] auto operator()(LerpType const lerp_type, InclusiveRange<glm::vec3> const& samples, float const alpha) const -> glm::vec3 {
			if (lerp_type == LerpType::eStep) { return samples.lo; }
			return glm::mix(samples.lo, samples.hi, alpha);
		}

		[[nodiscard]] auto operator()(LerpType const lerp_type, InclusiveRange<glm::quat> const& samples, float const alpha) const -> glm::quat {
			if (lerp_type == LerpType::eStep) { return samples.lo; }
			return glm::slerp(samples.lo, samples.hi, alpha);
		}
	};

	/// \brief Sampler for positions.
	class Translator : public AnimationSampler<glm::vec3, Interpolator> {
	  public:
		using AnimationSampler<glm::vec3, Interpolator>::AnimationSampler;

		void operator()(Transform& out, Seconds const elapsed) const { out.set_position(sample(elapsed, out.get_position())); }
	};

	/// \brief Sampler for orientations.
	class Rotator : public AnimationSampler<glm::quat, Interpolator> {
	  public:
		using AnimationSampler<glm::quat, Interpolator>::AnimationSampler;

		void operator()(Transform& out, Seconds const elapsed) const { out.set_orientation(sample(elapsed, out.get_orientation())); }
	};

	/// \brief Sampler for scales.
	class Scaler : public AnimationSampler<glm::vec3, Interpolator> {
	  public:
		using AnimationSampler<glm::vec3, Interpolator>::AnimationSampler;

		void operator()(Transform& out, Seconds const elapsed) const { out.set_scale(sample(elapsed, out.get_scale())); }
	};

	[[nodiscard]] auto is_empty() const -> bool {
		return std::visit([](auto const& s) { return s.is_empty(); }, sampler);
	}

	[[nodiscard]] auto get_entry() const -> Seconds {
		return std::visit([](auto const& s) { return s.get_entry(); }, sampler);
	}

	[[nodiscard]] auto get_exit() const -> Seconds {
		return std::visit([](auto const& s) { return s.get_exit(); }, sampler);
	}

	/// \brief Update the transform based on sampled values.
	/// \param out Transform to update.
	/// \param elapsed Timestamp to sample.
	void update(Transform& out, Seconds const elapsed) const {
		std::visit([&out, elapsed](auto const& sampler) { sampler(out, elapsed); }, sampler);
	}

	void operator()(Transform& out, Seconds const elapsed) const { update(out, elapsed); }

	/// \brief Sampler for position / orientation / scale.
	std::variant<Translator, Rotator, Scaler> sampler{};
};
} // namespace levk

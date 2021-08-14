#pragma once
#include <concepts>
#include <core/hash.hpp>
#include <core/services.hpp>
#include <core/span.hpp>
#include <engine/assets/asset_store.hpp>
#include <engine/scene/prop.hpp>

namespace le {
namespace graphics {
class Mesh;
}
class Model;
class Text2D;

template <typename T>
struct PropExtractor {
	Span<Prop const> operator()(T const& source) const noexcept;
};

class PropProvider {
  public:
	template <typename T>
	static PropProvider make(Hash assetID) noexcept;
	template <typename T>
	static PropProvider make(T const& source) noexcept;

	PropProvider() = default;
	explicit PropProvider(Hash meshID, Material material = {}) noexcept;
	explicit PropProvider(Span<Prop const> props) noexcept : m_props({{}, props}) {}

	bool active() const noexcept { return cached() || m_extract; }
	explicit operator bool() const noexcept { return active(); }

	Span<Prop const> props() const { return m_extract ? m_extract(this) : m_props.get<Props>().props; }
	bool cached() const noexcept { return m_props.contains<Prop>() || !m_props.get<Props>().props.empty(); }

  private:
	struct Props {
		Material material;
		Span<Prop const> props;
	};

	template <typename T>
	struct tag_t {};

	using Extract = Span<Prop const> (*)(PropProvider const*);

	template <typename T>
	PropProvider(Hash id, Material material, tag_t<T>) noexcept;

	template <typename T>
	static Span<Prop const> extract(PropProvider const* self);
	template <typename T>
	void refresh() const;

	mutable ktl::either<Props, Prop> m_props;
	Extract m_extract{};
	Hash m_id{};
};

template <>
struct PropExtractor<Model> {
	Span<Prop const> operator()(Model const& model) const noexcept;
};

template <>
struct PropExtractor<Text2D> {
	Span<Prop const> operator()(Text2D const& text) const noexcept;
};

// impl

template <typename T>
PropProvider PropProvider::make(Hash id) noexcept {
	return PropProvider(id, {}, tag_t<T>{});
}

template <typename T>
PropProvider PropProvider::make(T const& source) noexcept {
	return PropProvider(PropExtractor<T>{}(source));
}

inline PropProvider::PropProvider(Hash meshID, Material material) noexcept : PropProvider(meshID, material, tag_t<graphics::Mesh>{}) {}

template <typename T>
PropProvider::PropProvider(Hash id, Material material, tag_t<T>) noexcept : m_extract(&extract<T>), m_id(id) {
	m_props.get<Props>().material = material;
}

template <typename T>
Span<Prop const> PropProvider::extract(PropProvider const* self) {
	if (!self->m_props.contains<Prop>() && self->m_props.get<Props>().props.empty()) { self->refresh<T>(); }
	if (self->m_props.contains<Prop>()) { return self->m_props.get<Prop>(); }
	return self->m_props.get<Props>().props;
}

template <typename T>
void PropProvider::refresh() const {
	if (m_id != Hash()) {
		if (auto store = Services::locate<AssetStore>(false)) {
			if (auto asset = store->find<T>(m_id)) {
				if constexpr (std::is_same_v<T, graphics::Mesh>) {
					auto mat = m_props.get<Props>().material;
					m_props = Prop{mat, &*asset};
				} else {
					m_props.get<Props>().props = PropExtractor<T>{}(*asset);
				}
			}
		}
	}
}
} // namespace le

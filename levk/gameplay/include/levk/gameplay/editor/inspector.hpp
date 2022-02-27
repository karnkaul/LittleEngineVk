#pragma once
#include <dens/detail/sign.hpp>
#include <levk/core/utils/expect.hpp>
#include <levk/core/utils/string.hpp>
#include <levk/gameplay/editor/inspect.hpp>
#include <levk/gameplay/editor/types.hpp>

namespace le {
class AssetStore;
}

namespace le::editor {
class Inspector {
  public:
	static constexpr std::string_view title_v = "Inspector";

	template <dens::Component T>
	using OnAttach = ktl::kfunction<T()>;
	template <dens::Component T>
	using OnInspect = ktl::kfunction<void(Inspect<T>)>;

	template <dens::Component T>
	static std::string_view defaultName() {
		return utils::removeNamespaces(utils::tName<T>());
	}

	template <dens::Component T>
	static void attach(OnInspect<T>&& onInspect, OnAttach<T>&& attach = {}, std::string_view name = defaultName<T>());
	static bool detach(std::string const& id);

  private:
	static void update(SceneRef const& scene);
	static void clear();

	static void attach(dens::entity entity, dens::registry& registry, AssetStore const& store);

	struct GadgetBase {
		virtual ~GadgetBase() = default;
		virtual bool attachable() const noexcept = 0;
		virtual void attach(dens::entity, dens::registry&) const = 0;
		virtual bool inspect(std::string_view, dens::entity, dens::registry&, AssetStore const& store, gui::TreeRoot* tree) const = 0;
	};
	template <typename T>
	struct TGadget : GadgetBase {
		OnInspect<T> inspect_;
		OnAttach<T> attach_;
		TGadget(OnInspect<T>&& inspect, OnAttach<T>&& attach) : inspect_(std::move(inspect)), attach_(std::move(attach)) {}
		bool attachable() const noexcept override { return attach_.has_value(); }
		void attach(dens::entity e, dens::registry& r) const override {
			if (attachable()) { r.attach<T>(e, attach_()); }
		}
		bool inspect(std::string_view id, dens::entity entity, dens::registry& registry, AssetStore const& store, gui::TreeRoot* tree) const override;
	};

	using GadgetMap = std::unordered_map<std::string, std::unique_ptr<GadgetBase>>;
	inline static GadgetMap s_gadgets;
	inline static GadgetMap s_guiGadgets;
	friend class Sudo;
};

// impl

template <typename T>
bool Inspector::TGadget<T>::inspect(std::string_view id, dens::entity entity, dens::registry& registry, AssetStore const& store, gui::TreeRoot* tree) const {
	if constexpr (std::is_base_of_v<gui::TreeRoot, T>) {
		if (auto t = dynamic_cast<T*>(tree)) {
			if (auto tn = TreeNode(id)) { inspect_(Inspect<T>{*t, registry, entity, store}); }
			Styler s(Style::eSeparator);
			return true;
		}
	} else {
		if (auto t = registry.find<T>(entity)) {
			auto tn = TreeNode(id);
			auto const detach = ktl::stack_string<64>("x##{}", id);
			Styler s(getWindowWidth() - 30.0f);
			if (Button(detach, 0.0f, true)) { registry.detach<T>(entity); }
			if (tn) { inspect_(Inspect<T>{*t, registry, entity, store}); }
			s = Styler(Style::eSeparator);
			return true;
		}
	}
	return false;
}

template <dens::Component T>
void Inspector::attach(OnInspect<T>&& inspect, OnAttach<T>&& attach, std::string_view name) {
	if constexpr (std::is_base_of_v<gui::TreeRoot, T>) {
		s_guiGadgets.insert_or_assign(std::string(name), std::make_unique<TGadget<T>>(std::move(inspect), std::move(attach)));
	} else {
		if constexpr (std::is_default_constructible_v<T>) {
			if (!attach) {
				attach = []() { return T{}; };
			}
		}
		s_gadgets.insert_or_assign(std::string(name), std::make_unique<TGadget<T>>(std::move(inspect), std::move(attach)));
	}
}
} // namespace le::editor

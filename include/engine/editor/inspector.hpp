#pragma once
#include <core/utils/expect.hpp>
#include <core/utils/string.hpp>
#include <dens/detail/sign.hpp>
#include <engine/editor/inspect.hpp>
#include <engine/editor/types.hpp>

namespace le::edi {
class Inspector {
  public:
	static constexpr std::string_view title_v = "Inspector";

	template <typename T>
	using OnAttach = ktl::move_only_function<T()>;
	template <typename T>
	using OnInspect = ktl::move_only_function<void(Inspect<T>)>;

	template <typename T>
	static std::string_view defaultName() {
		return utils::removeNamespaces(utils::tName<T>());
	}

	template <typename T>
	static void attach(OnInspect<T>&& onInspect, OnAttach<T>&& attach = {}, std::string_view name = defaultName<T>());
	static bool detach(std::string const& id);

  private:
	static void update(SceneRef const& scene);
	static void clear();

	static void attach(dens::entity entity, dens::registry& registry);

	struct GadgetBase {
		virtual ~GadgetBase() = default;
		virtual bool attachable() const noexcept = 0;
		virtual void attach(dens::entity, dens::registry&) const = 0;
		virtual bool inspect(std::string_view, dens::entity, dens::registry&, gui::TreeRoot* tree) const = 0;
	};
	template <typename T>
	struct TGadget : GadgetBase {
		OnAttach<T> attach_;
		OnInspect<T> inspect_;
		TGadget(OnInspect<T>&& inspect, OnAttach<T>&& attach) : inspect_(std::move(inspect)), attach_(std::move(attach)) {}
		bool attachable() const noexcept override { return attach_.has_value(); }
		void attach(dens::entity e, dens::registry& r) const override {
			if (attachable()) { r.attach<T>(e, attach_()); }
		}
		bool inspect(std::string_view id, dens::entity entity, dens::registry& registry, gui::TreeRoot* tree) const override;
	};

	using GadgetMap = std::unordered_map<std::string, std::unique_ptr<GadgetBase>>;
	inline static GadgetMap s_gadgets;
	inline static GadgetMap s_guiGadgets;
	friend class Sudo;
};

// impl

template <typename T>
bool Inspector::TGadget<T>::inspect(std::string_view id, dens::entity entity, dens::registry& registry, gui::TreeRoot* tree) const {
	if constexpr (std::is_base_of_v<gui::TreeRoot, T>) {
		if (auto t = dynamic_cast<T*>(tree)) {
			if (auto tn = TreeNode(id)) { inspect_(Inspect<T>{*t, registry, entity}); }
			Styler s(Style::eSeparator);
			return true;
		}
	} else {
		if (auto t = registry.find<T>(entity)) {
			if (auto tn = TreeNode(id)) {
				inspect_(Inspect<T>{*t, registry, entity});
				auto const detach = ktl::stack_string<64>("Detach##%s", id.data());
				if (Button(detach.get())) { registry.detach<T>(entity); }
			}
			Styler s(Style::eSeparator);
			return true;
		}
	}
	return false;
}

template <typename T>
void Inspector::attach(OnInspect<T>&& inspect, OnAttach<T>&& attach, std::string_view name) {
	if constexpr (std::is_base_of_v<gui::TreeRoot, T>) {
		s_guiGadgets.insert_or_assign(std::string(name), std::make_unique<TGadget<T>>(std::move(inspect), std::move(attach)));
	} else {
		s_gadgets.insert_or_assign(std::string(name), std::make_unique<TGadget<T>>(std::move(inspect), std::move(attach)));
	}
}
} // namespace le::edi

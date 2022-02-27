#pragma once
#include <dumb_json/json.hpp>
#include <dumb_tasks/executor.hpp>
#include <levk/engine/assets/asset_store.hpp>
#include <levk/engine/engine.hpp>
#include <levk/engine/render/texture_refs.hpp>
#include <levk/graphics/draw_primitive.hpp>
#include <map>

namespace le {
namespace graphics {
struct Mesh;
class Sampler;
class Texture;
struct SpirV;
class Font;
} // namespace graphics
struct RenderLayer;
struct RenderPipeline;
class Skybox;
class Model;

struct AssetManifest {
	class Parser;

	using Metadata = dj::ptr<dj::json>;
	using Group = std::unordered_map<std::string, Metadata>;
	using List = std::unordered_map<std::string, Group>;

	List list;

	static List populate(dj::json const& root);

	AssetManifest& include(List add);
	AssetManifest& exclude(List const& remove);
};

class AssetManifest::Parser : public utils::VBase {
  public:
	enum struct Order : u32 { eZeroth, eFirst, eSecond, eThird };

	using Stages = std::map<Order, std::vector<dts::task_t>>;
	using Group = AssetManifest::Group;

	template <typename T>
	static constexpr Order order(Order fallback = Order{4}) noexcept;
	template <typename... T>
	static constexpr Order depend(Order fallback = Order{4}) noexcept;
	static constexpr Order maxOrder(std::span<Order const> orders, Order fallback) noexcept;

	Parser(Engine::Service engine, not_null<Stages*> stages, Opt<Parser const> next = {}) noexcept : m_next(next), m_engine(engine), m_stages(stages) {}

	virtual std::optional<std::size_t> operator()(std::string_view name, Group const& group) const = 0;

	Opt<Parser const> m_next{};

  protected:
	void enqueue(Order order, dts::task_t task) const;
	template <typename T>
	void add(Order order, std::string uri, T asset) const;

	Engine::Service m_engine;
	not_null<Stages*> m_stages;
};

class ManifestLoader {
  public:
	using Parser = AssetManifest::Parser;
	using Order = Parser::Order;

	ManifestLoader(Engine::Service engine) noexcept : m_engine(engine) {}

	std::size_t preload(dj::json const& root, Opt<Parser const> custom = {});
	void loadAsync();
	void loadBlocking();
	void load(io::Path const& jsonURI, Opt<Parser> custom = {}, bool async = true, bool reload = false);
	std::size_t unload();

	AssetManifest const& manifest() const noexcept { return m_manifest; }
	void clear();
	void wait() const;
	bool busy() const { return m_future.busy(); }

  private:
	AssetManifest m_manifest;
	Parser::Stages m_stages;
	dts::future_t m_future;
	Engine::Service m_engine;
};

// impl

namespace detail {
template <typename T, typename... Compare>
constexpr bool any_same_v = (... || std::is_same_v<T, Compare>);
}

template <typename T>
constexpr AssetManifest::Parser::Order AssetManifest::Parser::order(Order fallback) noexcept {
	using namespace graphics;
	if constexpr (detail::any_same_v<T, Sampler, SpirV, RenderLayer, BPMaterialData, PBRMaterialData, TextureRefs>) {
		return Order::eZeroth;
	} else if constexpr (detail::any_same_v<T, Texture, RenderPipeline>) {
		return Order::eFirst;
	} else if constexpr (detail::any_same_v<T, Font, Skybox, Model, Mesh>) {
		return Order::eSecond;
	}
	return fallback;
}

template <typename... T>
constexpr AssetManifest::Parser::Order AssetManifest::Parser::depend(Order fallback) noexcept {
	constexpr Order orders[] = {order<T>()...};
	return Order{static_cast<u32>(maxOrder(orders, fallback)) + 1U};
}

constexpr AssetManifest::Parser::Order AssetManifest::Parser::maxOrder(std::span<Order const> orders, Order fallback) noexcept {
	if (orders.empty()) { return fallback; }
	Order ret = orders[0];
	for (auto const order : orders) {
		if (order > ret) { ret = order; }
	}
	return ret;
}

template <typename T>
void AssetManifest::Parser::add(Order order, std::string uri, T asset) const {
	enqueue(order, [e = m_engine, uri = std::move(uri), asset = std::move(asset)]() mutable { e.store().add(std::move(uri), std::move(asset)); });
}
} // namespace le

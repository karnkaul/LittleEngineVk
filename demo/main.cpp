#include <core/io/fs_media.hpp>
#include <core/io/zip_media.hpp>
#include <core/log.hpp>
#include <core/utils/execute.hpp>
#include <demo.hpp>
#include <engine/utils/env.hpp>

#include <compare>
#include <span>
#include <tuple>
#include <typeinfo>
#include <unordered_map>
#include <utility>

template <typename... Types>
using view_t = std::tuple<Types&...>;

struct sign_t {
	std::size_t hash{};

	bool operator==(sign_t const& rhs) const = default;
	operator std::size_t() const noexcept { return hash; }

	void add_type(sign_t rhs) noexcept { hash ^= rhs.hash; }

	struct hasher {
		std::size_t operator()(sign_t const& s) const noexcept { return s.hash; }
	};

	template <typename T>
	static sign_t make() noexcept {
		static std::size_t const ret = typeid(T).hash_code();
		return {ret};
	}

	static sign_t combine(std::span<sign_t const> types) noexcept {
		sign_t ret{};
		for (auto const sign : types) { ret.hash ^= sign.hash; }
		return ret;
	}
};

class row_base {
  public:
	virtual ~row_base() = default;

	sign_t sign() const noexcept { return m_sign; }
	bool match(sign_t s) const noexcept { return sign() == s; }

	virtual std::size_t size() const noexcept = 0;
	virtual void swap_back(std::size_t index) = 0;
	virtual void copy_back(void* ptr) = 0;
	virtual void move_back(void* ptr) = 0;
	virtual void pop_push_back(row_base& out) = 0;

  protected:
	row_base(sign_t s) noexcept : m_sign(s) {}

	sign_t m_sign{};
};

template <typename T>
class row : public row_base {
  public:
	row() noexcept : row_base(sign_t::make<T>()) {}

	std::size_t size() const noexcept override { return m_storage.size(); }
	void swap_back(std::size_t index) override {
		if (index + 1 < m_storage.size()) { std::swap(m_storage[index], m_storage.back()); }
	}
	void copy_back(void* ptr) override {
		auto& t = reinterpret_cast<T const&>(ptr);
		m_storage.push_back(t);
	}
	void move_back(void* ptr) override {
		auto t = reinterpret_cast<T*>(ptr);
		m_storage.push_back(std::move(*t));
	}
	void pop_push_back(row_base& out) override {
		assert(!m_storage.empty());
		out.move_back(&m_storage.back());
		m_storage.pop_back();
	}

	std::vector<T> m_storage;
};

class row_factory {
  public:
	template <typename T>
	sign_t register_type() {
		static auto const ret = sign_t::make<T>();
		if (!registered(ret)) {
			m_map[ret] = []() -> std::unique_ptr<row_base> { return std::make_unique<row<T>>(); };
		}
		return ret;
	}

	bool registered(sign_t sign) const noexcept { return m_map.contains(sign); }

	std::unique_ptr<row_base> make_row(sign_t type) const {
		auto it = m_map.find(type);
		assert(it != m_map.end() && it->second != nullptr);
		return it->second();
	}

  private:
	using make_row_t = std::unique_ptr<row_base> (*)();
	std::unordered_map<sign_t, make_row_t, sign_t::hasher> m_map;
};

struct entity {
	std::size_t id{};

	constexpr auto operator<=>(entity const&) const = default;
	constexpr operator std::size_t() const noexcept { return id; }

	struct hasher {
		std::size_t operator()(entity const& e) const noexcept { return e.id; }
	};
};

class archetype {
  public:
	struct id_t {
		std::vector<sign_t> types;
		sign_t combined{};

		static id_t make(std::span<sign_t const> signs) {
			id_t ret;
			ret.types = {signs.begin(), signs.end()};
			ret.combined = sign_t::combine(signs);
			return ret;
		}

		bool operator==(id_t const& rhs) const noexcept { return combined == rhs.combined; }

		struct hasher {
			std::size_t operator()(id_t const& id) const noexcept { return id.combined; }
		};
	};

	static archetype make(row_factory const& factory, std::span<sign_t const> signs) {
		archetype ret;
		ret.m_id = id_t::make(signs);
		ret.m_rows.reserve(signs.size());
		for (auto const sign : signs) { ret.m_rows.push_back(factory.make_row(sign)); }
		return ret;
	}

	id_t const& id() const noexcept { return m_id; }
	std::size_t size() const noexcept { return m_rows.empty() ? 0 : m_rows[0]->size(); }
	bool empty() const noexcept { return size() == 0; }

	row_base* find_base(sign_t sign) const noexcept {
		for (auto const& r : m_rows) {
			if (r->match(sign)) { return r.get(); }
		}
		return {};
	}

	template <typename T>
	row<T>* find() const noexcept {
		if (auto ret = find_base(sign_t::make<T>())) { return static_cast<row<T>*>(ret); }
		return {};
	}

	template <typename T>
	row<T>& get() const {
		auto found = find<T>();
		assert(found);
		return *found;
	}

	template <typename... Types>
	view_t<Types...> view(std::size_t index) const {
		return std::tie(get<Types>().m_storage.at(index)...);
	}

	template <typename... Types>
	view_t<Types...> push_row(Types&&... args) {
		(get<Types>().m_storage.push_back(std::forward<Types>(args)), ...);
		return view<Types...>(size() - 1);
	}

	bool is_last(std::size_t index) const noexcept { return index + 1 == size(); }

	entity swap_back(std::size_t index) {
		assert(index < size() && !is_last(index));
		std::swap(m_entities.at(index), m_entities.back());
		for (auto& row : m_rows) { row->swap_back(index); }
		return m_entities.at(index);
	}

	entity migrate_back(archetype& target) {
		auto const ret = m_entities.back();
		m_entities.pop_back();
		for (auto& row : m_rows) {
			auto dest = target.find_base(row->sign());
			assert(dest);
			row->pop_push_back(*dest);
		}
		return ret;
	}

	template <typename T, typename... Args>
	std::vector<T>& emplace_back(entity e, Args&&... args) {
		auto& vec = get<T>().m_storage;
		vec.emplace_back(std::forward<Args>(args)...);
		assert(!contains(e));
		m_entities.push_back(e);
		return vec;
	}

	bool contains(entity e) const noexcept {
		for (auto const& entity : m_entities) {
			if (entity == e) { return true; }
		}
		return false;
	}

  private:
	std::vector<std::unique_ptr<row_base>> m_rows;
	std::vector<entity> m_entities; // must be index-locked to m_rows[0]
	id_t m_id;

	friend class registry;
};

class archetype_map {
  public:
	using id_t = archetype::id_t;

	template <typename... Types>
	void register_types() {
		(m_factory.register_type<Types>(), ...);
	}

	bool registered(sign_t sign) const noexcept { return m_factory.registered(sign); }

	archetype& get_or_make(std::span<sign_t const> signs) {
		auto const id = id_t::make(signs);
		auto& ret = m_map[id];
		if (ret.id() == id_t{}) { ret = archetype::make(m_factory, signs); }
		return ret;
	}

	template <typename T>
	archetype& copy_append(archetype const& rhs) {
		auto signs = rhs.id().types;
		signs.push_back(sign_t::make<T>());
		return get_or_make(signs);
	}

	template <typename... Types>
	archetype const* find() const noexcept {
		static auto const combined = sign_t::make<Types...>().combined;
		if (auto it = m_map.find(combined); it != m_map.end()) { return &it->second; }
		return {};
	}

	using storage_map = std::unordered_map<id_t, archetype, id_t::hasher>;
	storage_map m_map;
	row_factory m_factory;
};

template <typename... Types>
class component_view {
  public:
	component_view() = default;

	std::size_t size() const noexcept { return m_size; }
	view_t<Types...> operator[](std::size_t index) const noexcept {
		assert(m_archetype);
		return m_archetype->view<Types...>(index);
	}

  private:
	component_view(archetype const* archetype) noexcept : m_archetype(archetype), m_size(m_archetype->size()) {}
	archetype const* m_archetype{};
	std::size_t m_size{};

	friend class registry;
};

class registry {
  public:
	template <typename... Types>
	entity make_entity() {
		auto [it, _] = m_records.emplace(entity{++m_next_id}, record{});
		if constexpr (sizeof...(Types) > 0) {
			sign_t const signs[] = {sign_t::make<Types>()...};
			m_map.register_types<Types...>();
			archetype& arch = m_map.get_or_make(signs);
			(emplace_back<Types>(it->second, arch), ...);
			arch.m_entities.push_back(it->first);
		}
		return it->first;
	}

	template <typename T>
	T* find(entity e) const {
		if (auto it = m_records.find(e); it != m_records.end() && it->second.arch) { return &it->second.arch->get<T>().m_storage.at(it->second.index); }
		return {};
	}

	template <typename T>
	T& get(entity e) const {
		auto ret = find<T>(e);
		assert(ret);
		return *ret;
	}

	template <typename T, typename... Args>
	T& attach(entity e, Args&&... args) {
		m_map.register_types<T>();
		record& rec = get_or_make(e);
		if (rec.arch) {
			migrate(rec, m_map.copy_append<T>(*rec.arch));
		} else {
			sign_t const signs[] = {sign_t::make<T>()};
			rec.arch = &m_map.get_or_make(signs);
		}
		assert(rec.arch);
		auto& vec = rec.arch->emplace_back<T>(e, std::forward<Args>(args)...);
		assert(!vec.empty());
		rec.index = vec.size() - 1;
		return vec.back();
	}

	archetype const* owner(entity e) const noexcept {
		if (auto it = m_records.find(e); it != m_records.end()) { return it->second.arch; }
		return {};
	}

	template <typename... Types>
	std::vector<component_view<Types...>> view() const {
		std::vector<component_view<Types...>> ret;
		for (auto const& [_, archetype] : m_map.m_map) {
			if ((archetype.find<Types>() && ...)) { ret.push_back(&archetype); }
		}
		return ret;
	}

  private:
	struct record {
		archetype* arch{};
		std::size_t index{};
	};

	record& get_or_make(entity e) {
		auto it = m_records.find(e);
		if (it == m_records.end()) {
			auto [i, _] = m_records.emplace(e, record{});
			it = i;
		}
		return it->second;
	}

	template <typename T>
	void emplace_back(record& r, archetype& arch) {
		r.arch = &arch;
		auto& vec = r.arch->get<T>().m_storage;
		r.index = vec.size();
		vec.emplace_back();
	}

	void migrate(record& out_record, archetype& out_arch) {
		send_to_back(out_record);
		auto popped = out_record.arch->migrate_back(out_arch);
		auto& record = m_records[popped];
		assert(record.arch == out_record.arch);
		record.arch = &out_arch;
		// record.index = out_arch.size(); must be done by caller
	}

	void send_to_back(record& r) {
		if (!r.arch->is_last(r.index)) {
			// swap with last
			entity displaced = r.arch->swap_back(r.index);
			// reindex displaced
			assert(m_records[displaced].arch == r.arch);
			m_records[displaced].index = r.index;
		}
	}

	archetype_map m_map;
	std::unordered_map<entity, record, entity::hasher> m_records;
	std::size_t m_next_id{};
};

int main(int argc, char const* const argv[]) {
	registry r;
	auto e = r.make_entity();
	r.attach<int>(e) = 50;
	r.attach<float>(e) = -100.0f;
	auto e1 = r.make_entity<int, float>();
	r.get<int>(e1) = 0;
	r.get<float>(e1) = 0.0f;
	auto e2 = r.make_entity<int, float>();
	r.get<int>(e2) = 42;
	r.get<float>(e2) = 3.14f;
	auto e3 = r.make_entity<char, float, int>();
	r.get<int>(e3) = -6;
	r.get<float>(e3) = 65.8f;
	for (auto view : r.view<int, float>()) {
		for (std::size_t i = 0; i < view.size(); ++i) { le::logD("int: {}, float: {}", std::get<int&>(view[i]), std::get<float&>(view[i])); }
	}

	using namespace le;
	switch (env::init(argc, argv)) {
	case clap::parse_result::quit: return 0;
	case clap::parse_result::parse_error: return 10;
	default: break;
	}
	// {
	// 	auto data = env::findData("demo/data.zip");
	// 	if (data) {
	// 		io::FSMedia fr;
	// 		if (auto zip = fr.bytes(*data)) {
	// 			io::ZIPMedia zr;
	// 			if (zr.mount("demo/data.zip", std::move(*zip))) {
	// 				return utils::Execute([&zr]() { return demo::run(zr) ? 0 : 10; });
	// 			}
	// 		}
	// 	}
	// }
	auto data = env::findData("demo/data");
	if (!data) {
		logE("FATAL: Failed to locate data!");
		return 1;
	}
	io::FSMedia media;
	media.mount(std::move(*data));
	return utils::Execute([&media]() { return demo::run(media) ? 0 : 10; });
}

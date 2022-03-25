#pragma once
#include <ktl/hash_table.hpp>
#include <ktl/kunique_ptr.hpp>
#include <levk/core/hash.hpp>
#include <typeinfo>

namespace le {
///
/// \brief Handle to an object in (global) DataStore
///
template <typename T>
struct DataObject {
	using type = T;

	T* t{};
	Hash id{};

	///
	/// \brief Point to object if exists
	///
	DataObject(Hash id);

	///
	/// \brief Assign t to existing / new value
	///
	DataObject& set(T t);
	///
	/// \brief Unset value if exists
	///
	void unset() noexcept;

	DataObject& operator=(T t) { return set(std::move(t)); }
	bool operator==(DataObject const& rhs) const { return t && t == rhs.t; };
	explicit operator bool() const noexcept { return t != nullptr; }

	T& operator*() const { return *t; }
	T* operator->() const { return t; }
};

///
/// \brief Global data store
///
class DataStore {
  public:
	///
	/// \brief Add / replace t associated with id
	///
	template <typename T>
	static T& set(Hash id, T t) {
		return emplace<T>(id, std::move(t));
	}
	///
	/// \brief Add / replace t associated with id
	///
	template <typename T, typename... Args>
	static T& emplace(Hash id, Args&&... args) {
		return cast<T>(s_map.emplace(id, make<T>(std::forward<Args>(args)...)).first->second);
	}
	///
	/// \brief Remove data associated with id
	///
	static void erase(Hash id) noexcept { s_map.erase(id); }
	///
	/// \brief Check if there is data associated with id
	///
	static bool contains(Hash id) noexcept { return s_map.contains(id); }
	///
	/// \brief Check if data associated with id is of type T
	///
	template <typename T>
	static T* exists(Hash id) noexcept {
		auto it = s_map.find(id);
		return it != s_map.end() && it->second->typeHash == typeHash<T>();
	}
	///
	/// \brief Obtain pointer to data associated with id (if exists)
	///
	template <typename T>
	static T* find(Hash id) noexcept {
		auto it = s_map.find(id);
		return it != s_map.end() ? extract<T>(it->second) : nullptr;
	}
	///
	/// \brief Obtain reference to data associated with id (assumed to exist)
	///
	template <typename T>
	static T& get(Hash id) {
		return *find<T>(id);
	}
	///
	/// \brief Obtain reference to / default construct data associated with id
	///
	template <typename T>
	static T& getOrSet(Hash id) noexcept {
		return contains(id) ? *find<T>(id) : set<T>(id, T{});
	}

  private:
	struct Concept {
		std::size_t typeHash{};
		virtual ~Concept() = default;
	};
	template <typename T>
	struct Model : Concept {
		T t;
		Model(T t) : t(std::move(t)) { typeHash = DataStore::typeHash<T>(); }
	};
	using Entry = ktl::kunique_ptr<Concept>;
	using Map = ktl::hash_table<Hash, Entry>;

	template <typename T>
	static std::size_t typeHash() noexcept {
		return typeid(T).hash_code();
	}
	template <typename T>
	static T& cast(Entry const& c) {
		return static_cast<Model<T>&>(*c).t;
	}
	template <typename T>
	static T* extract(Entry const& e) noexcept {
		return (e->typeHash == typeHash<T>()) ? &cast<T>(e) : nullptr;
	}
	template <typename T, typename... Args>
	static Entry make(Args&&... args) {
		return ktl::make_unique<Model<T>>(std::forward<Args>(args)...);
	}

	inline static Map s_map;
};

// impl

template <typename T>
DataObject<T>::DataObject(Hash id) : id(id) {
	t = DataStore::find<T>(id);
}

template <typename T>
void DataObject<T>::unset() noexcept {
	DataStore::erase(id);
	t = {};
}

template <typename T>
DataObject<T>& DataObject<T>::set(T t) {
	this->t = &DataStore::set(id, std::move(t));
	return *this;
}
} // namespace le

#pragma once
#include <type_traits>
#include <core/counter.hpp>
#include <core/tasks.hpp>
#include <engine/resources/resource_types.hpp>

namespace le::res {
using Semaphore = TCounter<s32>::Semaphore;
template <typename T>
using Result = kt::result_t<T>;

///
/// \brief Acquire a semaphore to block deinit/unload until (all have been) released
///
Semaphore acquire();

///
/// \brief RAII handle to a resource (unloads in destructor)
///
template <typename T>
struct TScoped final : NoCopy {
	static_assert(std::is_base_of_v<Resource<T>, T>, "T must derive from Resource!");
	T resource;

	constexpr TScoped(T t = T{}) noexcept : resource(t) {
	}
	constexpr TScoped(TScoped&&) noexcept;
	TScoped& operator=(TScoped&&);
	~TScoped();

	///
	/// \brief Implicitly cast to T
	///
	constexpr operator T const &() const noexcept {
		return resource;
	}
	///
	/// \brief Check if held resource is ready to use
	///
	bool ready() const;
};

template <typename T>
class Async final {
  public:
	constexpr Async() = default;
	Async(std::shared_ptr<tasks::Handle> task, Hash id) : m_task(std::move(task)), m_id(id) {
	}
	Async(Async&&) = default;
	Async& operator=(Async&&) = default;
	Async(Async const&) = default;
	Async& operator=(Async const&) = default;
	~Async();

	bool valid() const;
	bool loaded() const;
	Result<T> resource() const;

	bool reset();

  private:
	std::shared_ptr<tasks::Handle> m_task;
	Hash m_id;
};

template <typename T>
Result<T> find(Hash id);
template <typename T>
typename T::Info const& info(T resource);
template <typename T>
Status status(T resource);
template <typename T>
bool unload(T resource);
template <typename T>
bool unload(Hash id);

///
/// \brief Load new Shader
///
Shader load(io::Path const& id, Shader::CreateInfo createInfo);
///
/// \brief Find a loaded Shader
///
template <>
Result<Shader> find<Shader>(Hash id);
///
/// \brief Obtain Info for a loaded Shader
///
template <>
Shader::Info const& info<Shader>(Shader shader);
///
/// \brief Obtain Status for a loaded Shader
///
template <>
Status status<Shader>(Shader shader);
///
/// \brief Unload a loaded Shader
///
template <>
bool unload<Shader>(Shader shader);
///
/// \brief Unload a loaded Shader
///
template <>
bool unload<Shader>(Hash id);

///
/// \brief Load new Sampler
///
Sampler load(io::Path const& id, Sampler::CreateInfo createInfo);
///
/// \brief Find a loaded Sampler
///
template <>
Result<Sampler> find<Sampler>(Hash id);
///
/// \brief Obtain Info for a loaded Sampler
///
template <>
Sampler::Info const& info<Sampler>(Sampler sampler);
///
/// \brief Obtain Status for a loaded Sampler
///
template <>
Status status<Sampler>(Sampler sampler);
///
/// \brief Unload a loaded Sampler
///
template <>
bool unload<Sampler>(Sampler sampler);
///
/// \brief Unload a loaded Sampler
///
template <>
bool unload<Sampler>(Hash id);

///
/// \brief Load new Texture
///
Texture load(io::Path const& id, Texture::CreateInfo createInfo);
///
/// \brief Load new Texture on a separate thread
///
Async<Texture> loadAsync(io::Path const& id, Texture::LoadInfo loadInfo);
///
/// \brief Find a loaded Texture
///
template <>
Result<Texture> find<Texture>(Hash id);
///
/// \brief Obtain Info for a loaded Texture
///
template <>
Texture::Info const& info<Texture>(Texture texture);
///
/// \brief Obtain Status for a loaded Texture
///
template <>
Status status<Texture>(Texture texture);
///
/// \brief Unload a loaded Texture
///
template <>
bool unload<Texture>(Texture texture);
///
/// \brief Unload a loaded Texture
///
template <>
bool unload<Texture>(Hash id);

///
/// \brief Load new Material
///
Material load(io::Path const& id, Material::CreateInfo createInfo);
///
/// \brief Find a loaded Material
///
template <>
Result<Material> find<Material>(Hash id);
///
/// \brief Obtain Info for a loaded Material
///
template <>
Material::Info const& info<Material>(Material material);
///
/// \brief Obtain Status for a loaded Material
///
template <>
Status status<Material>(Material material);
///
/// \brief Unload a loaded Material
///
template <>
bool unload<Material>(Material material);
///
/// \brief Unload a loaded Material
///
template <>
bool unload<Material>(Hash id);

///
/// \brief Load new Mesh
///
Mesh load(io::Path const& id, Mesh::CreateInfo createInfo);
///
/// \brief Find a loaded Mesh
///
template <>
Result<Mesh> find<Mesh>(Hash id);
///
/// \brief Obtain Info for a loaded Mesh
///
template <>
Mesh::Info const& info<Mesh>(Mesh mesh);
///
/// \brief Obtain Status for a loaded Mesh
///
template <>
Status status<Mesh>(Mesh mesh);
///
/// \brief Unload a loaded Mesh
///
template <>
bool unload<Mesh>(Mesh mesh);
///
/// \brief Unload a loaded Mesh
///
template <>
bool unload<Mesh>(Hash id);

///
/// \brief Load new Font
///
Font load(io::Path const& id, Font::CreateInfo createInfo);
///
/// \brief Find a loaded Font
///
template <>
Result<Font> find<Font>(Hash id);
///
/// \brief Obtain Info for a loaded Font
///
template <>
Font::Info const& info<Font>(Font font);
///
/// \brief Obtain Status for a loaded Font
///
template <>
Status status<Font>(Font font);
///
/// \brief Unload a loaded Font
///
template <>
bool unload<Font>(Font font);
///
/// \brief Unload a loaded Font
///
template <>
bool unload<Font>(Hash id);

///
/// \brief Load new Model
///
Model load(io::Path const& id, Model::CreateInfo createInfo);
///
/// \brief Load new Model on a separate thread
///
Async<Model> loadAsync(io::Path const& id, Model::LoadInfo loadInfo);
///
/// \brief Find a loaded Model
///
template <>
Result<Model> find<Model>(Hash id);
///
/// \brief Obtain Info for a loaded Model
///
template <>
Model::Info const& info<Model>(Model model);
///
/// \brief Obtain Status for a loaded Model
///
template <>
Status status<Model>(Model model);
///
/// \brief Unload a loaded Model
///
template <>
bool unload<Model>(Model model);
///
/// \brief Unload a loaded Model
///
template <>
bool unload<Model>(Hash id);

///
/// \brief Unload a loaded Resource
///
bool unload(Hash id);

template <typename T>
Result<T> find(Hash) {
	static_assert(false_v<T>, "Invalid type!");
}

template <typename T>
constexpr TScoped<T>::TScoped(TScoped<T>&& rhs) noexcept : resource(std::move(rhs.resource)) {
}

template <typename T>
TScoped<T>& TScoped<T>::operator=(TScoped<T>&& rhs) {
	if (&rhs != this) {
		unload(resource);
		resource = std::move(rhs.resource);
	}
	return *this;
}

template <typename T>
TScoped<T>::~TScoped() {
	unload(resource);
}

template <typename T>
bool TScoped<T>::ready() const {
	return resource.guid.payload != GUID::null && resource.status() == Status::eReady;
}

template <typename T>
Async<T>::~Async() {
	if (m_task) {
		m_task->wait();
	}
}

template <typename T>
bool Async<T>::valid() const {
	return m_id != Hash() && m_task && m_task->status() != tasks::Handle::Status::eDiscarded;
}

template <typename T>
bool Async<T>::loaded() const {
	return resource().has_result();
}

template <typename T>
Result<T> Async<T>::resource() const {
	if (valid()) {
		return res::find<T>(m_id);
	}
	return {};
}

template <typename T>
bool Async<T>::reset() {
	if (m_task) {
		m_task.reset();
		return true;
	}
	return false;
}
} // namespace le::res

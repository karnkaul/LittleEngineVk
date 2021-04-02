#pragma once
#include <shared_mutex>
#include <unordered_map>
#include <variant>
#include <core/hash.hpp>
#include <core/io/file_monitor.hpp>
#include <core/io/reader.hpp>
#include <core/os.hpp>
#include <kt/async_queue/locker.hpp>

constexpr bool levk_resourceMonitor = levk_debug && levk_desktopOS;

namespace le {
class Resource {
  public:
	enum class Type { eText, eBinary };

	io::Path const& path() const {
		return m_path;
	}
	std::string_view string() const noexcept;
	View<std::byte> bytes() const noexcept;
	Type type() const noexcept;

	bool monitoring() const noexcept;
	std::optional<io::FileMonitor::Status> status() const;

  private:
	bool load(io::Reader const& reader, io::Path path, Type type, bool bMonitor);

	using Data = std::variant<bytearray, std::string>;

	Data m_data;
	io::Path m_path;
	Type m_type = Type::eBinary;
	std::optional<io::FileMonitor> m_monitor;

	friend class Resources;
};

class Resources {
  public:
	using FMode = io::FileMonitor::Mode;

	void reader(io::Reader const& reader);
	io::Reader const& reader() const;
	io::FileReader& fileReader();

	Resource const* find(Hash id) const noexcept;
	Resource const* load(io::Path path, Resource::Type type, bool bMonitor = false, bool bForceReload = false);
	bool loaded(Hash id) const noexcept;

	void update();
	void clear();

  protected:
	kt::locker_t<std::unordered_map<Hash, Resource>, std::shared_mutex> m_loaded;
	io::FileReader m_fileReader;
	io::Reader const* m_pReader = nullptr;
};
} // namespace le

#pragma once
#include <shared_mutex>
#include <unordered_map>
#include <core/hash.hpp>
#include <core/io/file_monitor.hpp>
#include <core/io/reader.hpp>
#include <core/not_null.hpp>
#include <core/os.hpp>
#include <ktl/either.hpp>
#include <ktl/shared_tmutex.hpp>

constexpr bool levk_resourceMonitor = levk_debug;

namespace le {
class Resource {
  public:
	enum class Type { eText, eBinary };

	io::Path const& path() const { return m_path; }
	std::string_view string() const noexcept;
	Span<std::byte const> bytes() const noexcept;
	Type type() const noexcept;

	bool monitoring() const noexcept;
	std::optional<io::FileMonitor::Status> status() const;

  private:
	bool load(io::Reader const& reader, io::Path path, Type type, bool bMonitor);

	using Data = ktl::either<bytearray, std::string>;

	Data m_data;
	io::Path m_path;
	Type m_type = Type::eBinary;
	std::optional<io::FileMonitor> m_monitor;

	friend class Resources;
};

class Resources {
  public:
	using FMode = io::FileMonitor::Mode;

	void reader(not_null<io::Reader const*> reader);
	io::Reader const& reader() const;
	io::FileReader& fileReader();

	Resource const* find(Hash id) const noexcept;
	Resource const* load(io::Path path, Resource::Type type, bool bMonitor = false, bool bForceReload = false);
	bool loaded(Hash id) const noexcept;

	void update();
	void clear();

  protected:
	using ResourceMap = std::unordered_map<Hash, Resource>;

	ktl::shared_strict_tmutex<ResourceMap> m_loaded;
	io::FileReader m_fileReader;
	io::Reader const* m_reader = nullptr;
};
} // namespace le

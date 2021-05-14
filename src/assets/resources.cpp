#include <core/utils/algo.hpp>
#include <engine/assets/resources.hpp>

namespace le {
std::string_view Resource::string() const noexcept {
	if (m_monitor) {
		return m_monitor->text();
	} else if (auto pStr = std::get_if<std::string>(&m_data)) {
		return *pStr;
	} else {
		return {};
	}
}

View<std::byte> Resource::bytes() const noexcept {
	if (m_monitor) {
		return m_monitor->bytes();
	} else if (auto pBytes = std::get_if<bytearray>(&m_data)) {
		return *pBytes;
	} else {
		return {};
	}
}

Resource::Type Resource::type() const noexcept { return m_type; }

bool Resource::monitoring() const noexcept { return m_monitor.has_value(); }

std::optional<io::FileMonitor::Status> Resource::status() const {
	if (m_monitor) {
		return m_monitor->lastStatus();
	}
	return std::nullopt;
}

bool Resource::load(io::Reader const& reader, io::Path path, Type type, bool bMonitor) {
	if (reader.checkPresence(path)) {
		io::FileReader const* pFR = bMonitor ? dynamic_cast<io::FileReader const*>(&reader) : nullptr;
		if (bMonitor && pFR) {
			using FMode = io::FileMonitor::Mode;
			m_monitor = io::FileMonitor(pFR->fullPath(path).generic_string(), type == Type::eText ? FMode::eTextContents : FMode::eBinaryContents);
			m_monitor->update();
			m_data = bytearray();
		} else {
			if (type == Type::eText) {
				m_data = *reader.string(path);
			} else {
				m_data = *reader.bytes(path);
			}
			m_monitor.reset();
		}
		m_path = std::move(path);
		m_type = type;
		return true;
	}
	return false;
}

void Resources::reader(not_null<io::Reader const*> reader) { m_reader = reader; }

io::Reader const& Resources::reader() const {
	if (m_reader) {
		return *m_reader;
	}
	return m_fileReader;
}

io::FileReader& Resources::fileReader() { return m_fileReader; }

Resource const* Resources::find(Hash id) const noexcept {
	auto lock = m_loaded.lock<std::shared_lock>();
	if (auto it = lock.get().find(id); it != lock.get().end()) {
		return &it->second;
	}
	return nullptr;
}

Resource const* Resources::load(io::Path path, Resource::Type type, bool bMonitor, bool bForceReload) {
	if (!bForceReload) {
		if (auto pRes = find(path)) {
			return pRes;
		}
	}
	auto lock = m_loaded.lock<std::unique_lock>();
	lock.get().erase(path);
	Resource resource;
	if (resource.load(reader(), path, type, bMonitor && levk_resourceMonitor)) {
		auto [it, bRes] = lock.get().emplace(std::move(path), std::move(resource));
		ENSURE(bRes, "Map insertion failure");
		return &it->second;
	}
	return nullptr;
}

bool Resources::loaded(Hash id) const noexcept {
	auto lock = m_loaded.lock<std::shared_lock>();
	return utils::contains(lock.get(), id);
}

void Resources::update() {
	auto lock = m_loaded.lock<std::shared_lock>();
	for (auto& [_, resource] : lock.get()) {
		if (resource.m_monitor) {
			resource.m_monitor->update();
		}
	}
}

void Resources::clear() {
	auto lock = m_loaded.lock<std::unique_lock>();
	lock.get().clear();
}
} // namespace le

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

Span<std::byte> Resource::bytes() const noexcept {
	if (m_monitor) {
		return m_monitor->bytes();
	} else if (auto pBytes = std::get_if<bytearray>(&m_data)) {
		return *pBytes;
	} else {
		return {};
	}
}

Resource::Type Resource::type() const noexcept {
	return m_type;
}

bool Resource::monitoring() const noexcept {
	return m_monitor.has_value();
}

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

void Resources::reader(io::Reader const& reader) {
	m_pReader = &reader;
}

io::Reader const& Resources::reader() const {
	if (m_pReader) {
		return *m_pReader;
	}
	return m_fileReader;
}

io::FileReader& Resources::fileReader() {
	return m_fileReader;
}

Resource const* Resources::find(Hash id) const noexcept {
	if (auto it = m_loaded.find(id); it != m_loaded.end()) {
		return &it->second;
	}
	return nullptr;
}

Resource const* Resources::load(io::Path path, Resource::Type type, bool bMonitor, bool bForceReload) {
	if (auto pRes = find(path)) {
		if (bForceReload) {
			m_loaded.erase(path);
		} else {
			return pRes;
		}
	}
	Resource resource;
	if (resource.load(reader(), path, type, bMonitor && levk_resourceMonitor)) {
		auto [it, res] = m_loaded.emplace(std::move(path), std::move(resource));
		ENSURE(res, "Map insertion failure");
		return &it->second;
	}
	return nullptr;
}

bool Resources::loaded(Hash id) const noexcept {
	return m_loaded.find(id) != m_loaded.end();
}

void Resources::update() {
	for (auto& [_, resource] : m_loaded) {
		if (resource.m_monitor) {
			resource.m_monitor->update();
		}
	}
}

void Resources::clear() {
	m_loaded.clear();
}
} // namespace le

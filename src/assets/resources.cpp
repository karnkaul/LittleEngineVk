#include <core/utils/algo.hpp>
#include <engine/assets/resources.hpp>

namespace le {
std::string_view Resource::string() const noexcept {
	if (m_monitor) {
		return m_monitor->text();
	} else if (auto pStr = m_data.get_if<std::string>()) {
		return *pStr;
	} else {
		return {};
	}
}

Span<std::byte const> Resource::bytes() const noexcept {
	if (m_monitor) {
		return m_monitor->bytes();
	} else if (auto pBytes = m_data.get_if<bytearray>()) {
		return *pBytes;
	} else {
		return {};
	}
}

Resource::Type Resource::type() const noexcept { return m_type; }

bool Resource::monitoring() const noexcept { return m_monitor.has_value(); }

std::optional<io::FileMonitor::Status> Resource::status() const {
	if (m_monitor) { return m_monitor->lastStatus(); }
	return std::nullopt;
}

bool Resource::load(io::Media const& media, io::Path uri, Type type, bool monitor, bool silent) {
	std::optional<dl::level> lvl;
	if (!silent) { lvl = dl::level::warn; }
	if (media.present(uri, lvl)) {
		io::FSMedia const* fm = monitor ? dynamic_cast<io::FSMedia const*>(&media) : nullptr;
		if (monitor && fm) {
			using FMode = io::FileMonitor::Mode;
			m_monitor = io::FileMonitor(fm->fullPath(uri).generic_string(), type == Type::eText ? FMode::eTextContents : FMode::eBinaryContents);
			m_monitor->update();
			m_data = bytearray();
		} else {
			if (type == Type::eText) {
				m_data = *media.string(uri);
			} else {
				m_data = *media.bytes(uri);
			}
			m_monitor.reset();
		}
		m_uri = std::move(uri);
		m_type = type;
		return true;
	}
	return false;
}

void Resources::media(not_null<io::Media const*> media) { m_media = media; }

io::Media const& Resources::media() const {
	if (m_media) { return *m_media; }
	return m_fsMedia;
}

io::FSMedia& Resources::fsMedia() { return m_fsMedia; }

Resource const* Resources::find(Hash uri) const noexcept {
	ktl::tlock lock(m_loaded);
	if (auto it = lock.get().find(uri); it != lock.get().end()) { return &it->second; }
	return nullptr;
}

Resource const* Resources::load(io::Path uri, Resource::Type type, Flags flags) {
	if (!flags.test(Flag::eReload)) {
		if (auto res = find(uri)) { return res; }
	}
	return loadImpl(std::move(uri), type, flags);
}

Resource const* Resources::loadFirst(Span<io::Path> uris, Resource::Type type, Flags flags) {
	for (auto& uri : uris) {
		if (!flags.test(Flag::eReload)) {
			if (auto res = find(uri)) { return res; }
		}
		if (media().present(uri, std::nullopt)) { return loadImpl(std::move(uri), type, flags); }
	}
	return nullptr;
}

bool Resources::loaded(Hash uri) const noexcept {
	ktl::tlock lock(m_loaded);
	return utils::contains(lock.get(), uri);
}

void Resources::update() {
	ktl::tlock lock(m_loaded);
	for (auto& [_, resource] : lock.get()) {
		if (resource.m_monitor) { resource.m_monitor->update(); }
	}
}

void Resources::clear() {
	ktl::unique_tlock<ResourceMap> lock(m_loaded);
	lock.get().clear();
}

Resource const* Resources::loadImpl(io::Path uri, Resource::Type type, Flags flags) {
	Resource resource;
	if (resource.load(media(), uri, type, flags.test(Flag::eMonitor) && levk_resourceMonitor, flags.test(Flag::eSilent))) {
		ktl::unique_tlock<ResourceMap> lock(m_loaded);
		lock->erase(uri);
		auto [it, _] = lock.get().emplace(std::move(uri), std::move(resource));
		return &it->second;
	}
	return nullptr;
}
} // namespace le

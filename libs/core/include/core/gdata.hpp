#pragma once
#include <unordered_map>
#include <vector>
#include <core/std_types.hpp>

namespace le
{
///
/// \brief Pseudo-JSON serialisable data container
///
class GData
{
protected:
	std::unordered_map<std::string, std::string> m_fieldMap;

public:
	GData();
	///
	/// \brief Marhshall `serialised` and load fields
	///
	explicit GData(std::string serialised);
	GData(GData&&);
	GData& operator=(GData&&);
	GData(GData const& rhs);
	GData& operator=(GData const&);
	virtual ~GData();

	///
	/// \brief Marhshall and load fields from serialised data
	///
	bool marshall(std::string serialised);
	///
	/// \brief Unmarshall all fields into a string
	/// \returns Original raw data, without whitespaces and enclosing braces
	///
	[[nodiscard]] std::string unmarshall() const;
	///
	/// \brief Clear raw data and fields
	///
	void clear();

	[[nodiscard]] std::string getString(std::string const& key, std::string defaultValue = "") const;
	[[nodiscard]] bool getBool(std::string const& key, bool defaultValue) const;
	[[nodiscard]] s32 getS32(std::string const& key, s32 defaultValue) const;
	[[nodiscard]] f64 getF64(std::string const& key, f64 defaultValue) const;
	[[nodiscard]] std::vector<std::string> getVecString(std::string const& key) const;

	[[nodiscard]] GData getGData(std::string const& key) const;
	[[nodiscard]] std::vector<GData> getGDatas(std::string const& key) const;

	[[nodiscard]] std::unordered_map<std::string, std::string> const& allFields() const;
	bool addField(std::string key, GData& gData);
	bool setString(std::string key, std::string value);

	[[nodiscard]] u32 fieldCount() const;
	[[nodiscard]] bool contains(std::string const& id) const;
};
} // namespace le

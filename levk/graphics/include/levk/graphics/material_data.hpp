#pragma once
#include <levk/graphics/rgba.hpp>

namespace le::graphics {
enum class MatTexType { eDiffuse, eSpecular, eAlpha, eBump, eMetalRough, eOcclusion, eNormal, eEmissive, eCOUNT_ };

template <typename T>
using TMatTexArray = EnumArray<MatTexType, T>;

struct BPMaterialData {
	RGBA Ka = colours::white;
	RGBA Kd = colours::white;
	RGBA Ks = colours::white;
	RGBA Tf = colours::white;
	f32 Ns = 42.0f;
	f32 d = 1.0f;
	s32 illum = 2;

	struct Std140;
	Std140 std140() const noexcept;
};

struct PBRMaterialData {
	enum class Mode : u8 { Opaque, Mask, Blend };

	RGBA baseColourFactor = colours::white;
	RGBA emissiveFactor = colours::white;
	f32 metallicFactor = 1.0f;
	f32 roughnessFactor = 1.0f;
	f32 alphaCutoff = 0.5f;
	Mode mode = Mode::Opaque;
};

struct BPMaterialData::Std140 {
	alignas(16) glm::vec4 tint{};
	alignas(16) glm::vec4 ambient{};
	alignas(16) glm::vec4 diffuse{};
	alignas(16) glm::vec4 specular{};
};

inline BPMaterialData::Std140 BPMaterialData::std140() const noexcept { return {{glm::vec3(Tf.toVec4()), d}, Ka.toVec4(), Kd.toVec4(), Ks.toVec4()}; }
} // namespace le::graphics

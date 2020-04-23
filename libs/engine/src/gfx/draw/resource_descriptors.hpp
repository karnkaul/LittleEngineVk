#pragma once
#include <vector>
#include "core/flags.hpp"
#include "core/utils.hpp"
#include "gfx/common.hpp"
#include "gfx/vram.hpp"

namespace le::gfx
{
class Texture;

namespace rd
{
struct View final
{
	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	alignas(16) glm::mat4 mat_vp = glm::mat4(1.0f);
	alignas(16) glm::mat4 mat_v = glm::mat4(1.0f);
	alignas(16) glm::mat4 mat_p = glm::mat4(1.0f);
	alignas(16) glm::mat4 mat_ui = glm::mat4(1.0f);
	alignas(16) glm::vec3 pos_v = glm::vec3(0.0f);
};

struct Locals final
{
	constexpr static u32 max = 1024;

	static vk::DescriptorSetLayoutBinding const s_setLayoutBinding;

	enum : u32
	{
		eTEXTURED = 1 << 0,
		eLIT = 1 << 1,
	};

	alignas(16) glm::mat4 mat_m = glm::mat4(1.0f);
	alignas(16) glm::mat4 mat_n = glm::mat4(1.0f);
	alignas(16) glm::vec4 tint = glm::vec4(1.0f);
	alignas(4) u32 flags = 0;
};

struct Textures final
{
	constexpr static u32 max = 1024;

	static vk::DescriptorSetLayoutBinding const s_diffuseLayoutBinding;
	static vk::DescriptorSetLayoutBinding const s_specularLayoutBinding;
};

struct Push final
{
	u32 localID = 0;
	u32 diffuseID = 0;
	u32 specularID = 0;
};

struct WriteInfo final
{
	vk::DescriptorSet set;
	vk::DescriptorType type;
	vk::DescriptorBufferInfo* pBuffer = nullptr;
	vk::DescriptorImageInfo* pImage = nullptr;
	u32 binding = 0;
	u32 arrayElement = 0;
	u32 count = 1;
};

struct ViewBuffer final
{
	Buffer buffer;

	void create();
	void release();

	bool write(View const& view);
};

struct LocalsBuffer final
{
	std::array<Buffer, Locals::max> buffers;

	void create();
	void release();

	bool write(Locals const& locals, u32 idx);
	Buffer& at(u32 idx);
};

struct ShaderWriter final
{
	vk::DescriptorType type;
	u32 binding = 0;

	void write(vk::DescriptorSet set, Buffer const& buffer, u32 idx) const;
	void write(vk::DescriptorSet set, Texture const& texture, u32 idx) const;
};

class Set final
{
public:
	vk::DescriptorSet m_descriptorSet;

private:
	ViewBuffer m_viewBuffer;
	LocalsBuffer m_localsBuffer;
	ShaderWriter m_view;
	ShaderWriter m_locals;
	ShaderWriter m_diffuse;
	ShaderWriter m_specular;

public:
	void create();
	void destroy();

public:
	void writeView(View const& view);
	void writeLocals(Locals const& locals, u32 idx);
	void writeDiffuse(Texture const& diffuse, u32 idx);
	void writeSpecular(Texture const& specular, u32 idx);

	void resetTextures();
};

struct SetLayouts final
{
	vk::DescriptorPool descriptorPool;
	std::vector<Set> set;
};

inline vk::DescriptorSetLayout g_setLayout;

void init();
void deinit();

SetLayouts allocateSets(u32 copies);
} // namespace rd
} // namespace le::gfx

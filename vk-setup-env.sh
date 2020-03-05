# Export `VULKAN_SDK` to the custom SDK path and
# source this script into the build and runtime environment shell(s)

if [[ -z $VULKAN_SDK ]]; then
	echo -e "  ERROR: VULKAN_SDK not defined!"
else
	export VULKAN_SDK
	LD_LIBRARY_PATH="$VULKAN_SDK/lib:${LD_LIBRARY_PATH:-}"
	PATH="$VULKAN_SDK:$VULKAN_SDK/bin:$LD_LIBRARY_PATH:$PATH"
	export PATH
	export LD_LIBRARY_PATH
	VK_LAYER_PATH="$VULKAN_SDK/etc/vulkan/explicit_layer.d"
	export VK_LAYER_PATH
	VK_INSTANCE_LAYERS="VK_LAYER_KHRONOS_validation"
	export VK_INSTANCE_LAYERS
fi

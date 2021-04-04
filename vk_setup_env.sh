# Export `VULKAN_SDK` to the custom SDK path and
# source this script into the build and runtime environment shell(s)

if [[ -z $VULKAN_SDK ]]; then
	echo -e "  ERROR: VULKAN_SDK not defined!"
else
	export PATH="$VULKAN_SDK/../source/Vulkan-Headers/include:$VULKAN_SDK/lib:$VULKAN_SDK/bin:$PATH"
	export VK_LAYER_PATH="$VULKAN_SDK/etc/vulkan/explicit_layer.d"
fi

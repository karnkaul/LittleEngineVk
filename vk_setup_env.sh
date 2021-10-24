# Export `VULKAN_SDK` to the custom SDK path and source this script into the runtime environment shell
# Replicate this environment in launch.json for VSCode debugging

if [[ -z $VULKAN_SDK ]]; then
  echo -e "  ERROR: VULKAN_SDK not defined!"
else
  export LD_LIBRARY_PATH="$VULKAN_SDK/lib"                          # vulkan so/dll
  export VK_LAYER_PATH="$VULKAN_SDK/etc/vulkan/explicit_layer.d"    # validation layers
fi

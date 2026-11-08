set(GLFW_PATH /opt/homebrew/lib/)
set(VULKAN_SDK_PATH /Users/dylanjayasundera/VulkanSDK/1.4.328.1/macOS)
# Xcode 26.6 ld cannot read the CommandLineTools 27.0 SDK; use the SDK bundled with Xcode
set(CMAKE_OSX_SYSROOT /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk)

workspace "CompGFX"
    configurations({"Debug", "Release"})
    platforms { "Win64" }
    includedirs { 
        "dependency/assimp/include/",
        "dependency/glfw/include/",
        "dependency/glm/",
        "dependency/imgui/",
        "dependency/imgui/backends/",
        "dependency/",
        "dependency/VulkanMemoryAllocator/include",
        "dependency/json/single_include/nlohmann",
        "src/CompGFX/",
        os.getenv('VULKAN_SDK') .. '/include'
    }
    cppdialect "C++20"
    
    filter { "configurations:Debug" }
    libdirs { "lib/Debug" }
    links { "assimp-vc143-mtd.lib", "zlibstaticd.lib", "Dependencies.lib" }
    filter { "configurations:Release" }
    libdirs { "lib/Release" }
    links { "assimp-vc143-mt.lib", "zlibstatic.lib", "Dependencies.lib" }
    filter {}
    
    libdirs { os.getenv('VULKAN_SDK') .. "/lib" }
    
    targetdir("bin/%{prj.name}/%{cfg.buildcfg}-%{cfg.architecture}")
    objdir("bin/int/%{prj.name}/%{cfg.buildcfg}-%{cfg.architecture}")
    flags { "MultiProcessorCompile" }
    
    filter { "platforms:Win64" }
        system "Windows"
        architecture "x86_64"
        
    filter "configurations:Debug"
    defines { "DEBUG" }
    symbols "On"
    runtime "Debug"
    buildoptions "/MDd"
            
    filter "configurations:Release"
    defines { "NDEBUG" }
    optimize "On"
    runtime "Release"
    buildoptions "/MD"

    project "CompGFX"
        -- SharedLib
        kind "SharedLib"
        language "C++"
        location "src/CompGFX"
        files { 
            "src/CompGFX/**.h",
            "src/CompGFX/**.hpp",
            "src/CompGFX/**.c",
            "src/CompGFX/**.cpp"
        }

        filter { "system:Windows" }
        links { "glfw3.lib", "vulkan-1.lib" }
        filter "configurations:Debug"
        links { "spirv-cross-cored.lib", "shaderc_sharedd.lib" }
        filter "configurations:Release"
        links { "spirv-cross-core.lib", "shaderc_shared.lib" }
        filter { }
        -- "BUILD_GRAPHICS_DLL=1"
        defines { "WINDOWS", "_CRT_SECURE_NO_WARNINGS", "NOMINMAX", "BUILD_GRAPHICS_DLL=1" }
        postbuildcommands { "cmd /c \"cd \"$(ProjectDir)\" && python postbuild.py\"" }
        
        
    project "Sandbox"
    kind "ConsoleApp"
    language "C++"
    location "src/Sandbox"
        links { "CompGFX.lib" }
        files { 
            "src/Sandbox/**.h",
            "src/Sandbox/**.hpp",
            "src/Sandbox/**.c",
            "src/Sandbox/**.cpp",
        }
        
        filter { "system:Windows" }
        links { "kernel32.lib", "user32.lib", "gdi32.lib", "glfw3.lib", "vulkan-1.lib", "shaderc_shared.lib" }
        defines { "WINDOWS", "_CRT_SECURE_NO_WARNINGS", "BUILD_GRAPHICS_DLL=1", "NOMINMAX" }
        postbuildcommands { "cmd /c \"cd \"$(ProjectDir)\" && python postbuild.py\"" }
        dependson { "CompGFX" }
        
newaction {
    trigger = "clean",
    description = "Remove vs files.",
    execute = function()
        print("Cleaning...")
        os.rmdir("**.vs")
        os.remove("*.sln")
        os.remove("**.vcxproj")
        os.remove("**.vcxproj.filters")
        os.remove("**.vcxproj.user")
        print("Done")
    end
}

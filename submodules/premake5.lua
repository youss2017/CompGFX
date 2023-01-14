workspace "GenericWorkspace"
    location "Generic"
    configurations({"Debug", "Release"})
    platforms { "x64" }
    cppdialect "C++17"

    includedirs { 
        os.getenv('VULKAN_SDK') .. '/include',
        "imgui/",
        "../include/"
    }

    objdir("../bin/int/%{prj.name}")

    objdir("../bin/int/%{prj.name}/%{cfg.buildcfg}-%{cfg.architecture}")    
    targetdir("../library/%{cfg.buildcfg}")

    filter {}
    flags { "MultiProcessorCompile" }

    filter { "platforms:x64" }
        system "Windows"
        architecture "x86_64"

    project "Generic"
        location "Generic"
        kind "StaticLib"
        language "C++"
        files { 
            "VulkanMemoryAllocator/src/*.cpp",
            "imgui/*.cpp",
            "imgui/backends/imgui_impl_vulkan.h",
            "imgui/backends/imgui_impl_vulkan.cpp",
            "imgui/backends/imgui_impl_glfw.h",
            "imgui/backends/imgui_impl_glfw.cpp"
        }
        removefiles { "*Test*", "*test*" }

        filter { "system:Windows" }
            links { "kernel32.lib", "user32.lib", "gdi32.lib"  }
            defines { "WINDOWS", "_CRT_SECURE_NO_WARNINGS" }

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

newaction {
    trigger = "clean",
    description = "Remove all binaries, intermediate binaries, and vs files.",
    execute = function()
        print("Cleaning...")
        os.rmdir("./.vs")
        os.remove("*.sln")
        os.remove("**.vcxproj")
        os.remove("**.vcxproj.filters")
        os.remove("**.vcxproj.user")
        print("Done")
    end
}
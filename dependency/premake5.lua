workspace "Dependencies"
    location "CompinedDependencies"
    configurations({"Debug", "Release"})
    platforms { "Win64" }
    includedirs { 
        "./",
        "imgui/",
        "glfw/include/",
        os.getenv('VULKAN_SDK') .. '/include',
        os.getenv('VULKAN_SDK') .. '/include/vma'
    }
    cppdialect "C++20"
    
    libdirs { os.getenv('VULKAN_SDK') .. "/lib" }
    
    targetdir("../bin/%{prj.name}/%{cfg.buildcfg}-%{cfg.architecture}")
    objdir("../bin/int/%{prj.name}/%{cfg.buildcfg}-%{cfg.architecture}")
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

    project "Dependencies"
        -- SharedLib
        kind "StaticLib"
        language "C++"
        location "CompinedDependencies"
        files { 
            "CompinedDependencies/*.cpp",
            "imgui/*.cpp",
            "imgui/backends/imgui_impl_vulkan.cpp",
            "imgui/backends/imgui_impl_glfw.cpp"
        }
        defines { "WINDOWS", "_CRT_SECURE_NO_WARNINGS", "NOMINMAX" }
        postbuildcommands { 
            "xcopy $(ProjectDir)..\\..\\bin\\Dependencies\\Debug-x86_64\\* $(ProjectDir)..\\..\\lib\\Debug\\ /K /D /H /Y /s /e"
            ..
            "&"
            ..
            "xcopy $(ProjectDir)..\\..\\bin\\Dependencies\\Release-x86_64\\* $(ProjectDir)..\\..\\lib\\Release\\ /K /D /H /Y /s /e"
        }
        
        
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

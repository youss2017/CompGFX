workspace "TemplateSolution"
    configurations({"Debug", "Release"})
    platforms { "Win64" }
    includedirs { 
        "include/", 
        "include/imgui/", 
        os.getenv('VULKAN_SDK') .. '/include' }
    cppdialect "C++20"
    
    filter { "configurations:Debug" }
    libdirs { "lib/Debug" }
    filter { "configurations:Release" }
    libdirs { "lib/Release" }
    filter {}
    links { "Dependencies.lib" }
    
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

    project "TemplateProject"
        kind "ConsoleApp"
        language "C++"
        location "TemplateProject"
        files { 
            "TemplateProject/**.h",
            "TemplateProject/**.hpp",
            "TemplateProject/**.c",
            "TemplateProject/**.cpp",
        }

        filter { "system:Windows" }
        links { "CompGFX.lib" }
        filter { }
        defines { "WINDOWS", "_CRT_SECURE_NO_WARNINGS", "NOMINMAX" }
        prebuildcommands { 
            "xcopy $(SolutionDir)lib\\%{cfg.buildcfg}\\*.dll $(SolutionDir)bin\\%{prj.name}\\%{cfg.buildcfg}-%{cfg.architecture} /K /D /H /Y /s /e"
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

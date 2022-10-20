workspace "Egx"
    configurations({"Debug", "Release"})
    platforms { "Win64" }
    includedirs { 
        "../include/", 
        "../include/imgui/", 
        os.getenv('VULKAN_SDK') .. '/include' }
    cppdialect "C++17"
    
    filter { "configurations:Debug" }
    libdirs { "../library/Debug" }
    links { "assimp-vc143-mtd.lib", "zlibstaticd.lib" }
    filter { "configurations:Release" }
    libdirs { "../library/Release" }
    links { "assimp-vc143-mt.lib", "zlibstatic.lib" }
    filter {}
    
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

    project "EgxEngine"
        kind "SharedLib"
        language "C++"
        location "EgxEngine"
        files { 
            "EgxEngine/**.h",
            "EgxEngine/**.hpp",
            "EgxEngine/**.c",
            "EgxEngine/**.cpp",
        }

        filter { "system:Windows" }
        links { "kernel32.lib", "user32.lib", "gdi32.lib", "glfw3.lib", "vulkan-1.lib", "Generic.lib", "shaderc_shared.lib" }
        defines { "WINDOWS", "_CRT_SECURE_NO_WARNINGS", "BUILD_GRAPHICS_DLL=1" }
        postbuildcommands { "cmd /c \"cd \"$(SolutionDir)\" && python postbuild.py\"" }

newaction {
    trigger = "clean",
    description = "Remove vs files.",
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

newaction {
    trigger = "build",
    description = "Builds the project, make sure to run using Visual Studio Developer Tools.",
    execute = function()
        print("Starting build.")
        os.system("msbuild Egx.sln /p:Configuration=Debug");
        os.system("msbuild Egx.sln /p:Configuration=Release");
        print("Copying include files.")
        copy("*.hpp", "./", "../include/")
        copy("*.h", "./", "../include/")
    end
}
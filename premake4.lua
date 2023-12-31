solution "VirtualGo"
    language "C++"
    includedirs { ".", "Source" }
    configurations { "Debug", "Release" }
    configuration "Debug"
        flags { "Symbols" }
        defines { "_DEBUG" }
    configuration "Release"
        flags { "Optimize" }
        defines { "NDEBUG" }

project "UnitTest++"
    kind "StaticLib"
    files { "UnitTest++/**.h", "UnitTest++/**.cpp" }
    configuration { "windows" }
        excludes { "**/Posix/**" }
    configuration { "not windows" }
        excludes { "**/Win32/**" }
    targetdir "lib"
    location "build"

project "Support"
    kind "ConsoleApp"
    files { "Source/*.h", "Source/Support.cpp", "Source/Platform.cpp" }
    configuration { "macosx" }
        links { "OpenGL.framework", "AGL.framework", "Carbon.framework" }

project "Tessellation"
    kind "ConsoleApp"
    files { "Source/*.h", "Source/Tessellation.cpp", "Source/Platform.cpp" }
    configuration { "macosx" }
        links { "OpenGL.framework", "AGL.framework", "Carbon.framework" }

project "Dynamics"
    kind "ConsoleApp"
    files { "Source/*.h", "Source/Dynamics.cpp", "Source/Platform.cpp" }
    configuration { "macosx" }
        links { "OpenGL.framework", "AGL.framework", "Carbon.framework" }

project "Collision"
    kind "ConsoleApp"
    files { "Source/*.h", "Source/Collision.cpp", "Source/Platform.cpp", "Source/stb_image.c" }
    configuration { "macosx" }
        links { "OpenGL.framework", "AGL.framework", "Carbon.framework" }

project "UnitTest"
    kind "ConsoleApp"
    files { "UnitTest.cpp" }
    links { "UnitTest++" }

if _ACTION == "clean" then
    os.rmdir "obj"
    os.rmdir "output"
end

if not os.is "windows" then
    
    newaction
    {
        trigger     = "support",
        description = "Build and run support demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            os.rmdir "output"
            os.mkdir "output"
            if os.execute "make -j32 Support" == 0 then
                os.execute "./Support"
            end
        end
    }

    newaction
    {
        trigger     = "tessellation",
        description = "Build and run tessellation demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 Tessellation" == 0 then
                os.execute "./Tessellation"
            end
        end
    }

    newaction
    {
        trigger     = "dynamics",
        description = "Build and run dynamics demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            os.rmdir "output"
            os.mkdir "output"
            if os.execute "make -j32 Dynamics" == 0 then
                os.execute "./Dynamics"
            end
        end
    }

    newaction
    {
        trigger     = "collision",
        description = "Build and run collision demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            os.rmdir "output"
            os.mkdir "output"
            if os.execute "make -j32 Collision" == 0 then
                os.execute "./Collision"
            end
        end
    }

    newaction
    {
        trigger     = "test",
        description = "Build and run unit tests",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 UnitTest" == 0 then
                os.execute "./UnitTest"
            end
        end
    }

end

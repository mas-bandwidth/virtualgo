solution "VirtualGo"
    language "C++"
    includedirs { ".", ".." }
    configuration { "not windows" }
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

project "Collision"
    kind "ConsoleApp"
    files { "*.h", "Collision.cpp", "Platform.cpp" }
    configuration { "macosx" }
        links { "OpenGL.framework", "AGL.framework", "Carbon.framework" }

project "Support"
    kind "ConsoleApp"
    files { "*.h", "Support.cpp", "Platform.cpp" }
    configuration { "macosx" }
        links { "OpenGL.framework", "AGL.framework", "Carbon.framework" }

project "Tessellation"
    kind "ConsoleApp"
    files { "*.h", "Tessellation.cpp", "Platform.cpp" }
    configuration { "macosx" }
        links { "OpenGL.framework", "AGL.framework", "Carbon.framework" }

project "VirtualGo"
    kind "ConsoleApp"
    files { "*.h", "VirtualGo.cpp", "Platform.cpp" }
    configuration { "macosx" }
        links { "OpenGL.framework", "AGL.framework", "Carbon.framework" }

project "UnitTest"
    kind "ConsoleApp"
    files { "UnitTest.cpp" }
    links { "UnitTest++" }

if _ACTION == "clean" then
    os.rmdir "obj"
end

if not os.is "windows" then
    
    newaction
    {
        trigger     = "collision",
        description = "Build and run angular motion demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 Collision" == 0 then
                os.execute "./Collision"
            end
        end
    }

    newaction
    {
        trigger     = "angular",
        description = "Build and run angular motion demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 Angular" == 0 then
                os.execute "./Angular"
            end
        end
    }

    newaction
    {
        trigger     = "linear",
        description = "Build and run linear motion demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 Linear" == 0 then
                os.execute "./Linear"
            end
        end
    }

    newaction
    {
        trigger     = "picking",
        description = "Build and run picking demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 Picking" == 0 then
                os.execute "./Picking"
            end
        end
    }

    newaction
    {
        trigger     = "support",
        description = "Build and run support demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 Support" == 0 then
                os.execute "./Support"
            end
        end
    }

    newaction
    {
        trigger     = "tess",
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
        trigger     = "go",
        description = "Build and run virtual go",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 VirtualGo" == 0 then
                os.execute "./VirtualGo"
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

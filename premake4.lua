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

project "Tesselation"
    kind "ConsoleApp"
    files { "*.h", "Tesselation.cpp", "Platform.cpp" }
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
        trigger     = "tess",
        description = "Build and run tesselation demo",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j32 Tesselation" == 0 then
                os.execute "./Tesselation"
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

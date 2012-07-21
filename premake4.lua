solution "VirtualGo"
    language "C++"
    includedirs { "." }
    configuration { "not windows" }
    configurations { "Debug", "Release" }
    configuration "Debug"
        flags { "Symbols" }
        defines { "_DEBUG" }
    configuration "Release"
        flags { "Optimize" }
        defines { "NDEBUG" }

project "VirtualGo"
    kind "ConsoleApp"
    files { "*.h", "*.cpp" }

if not os.is "windows" then

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

end

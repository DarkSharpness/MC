add_requires("antlr4-runtime 4.13.2")
add_rules("mode.debug", "mode.release")

local warnings = {
    "all",      -- turn on all warnings
    "extra",    -- turn on extra warnings
    "error",    -- treat warnings as errors
    -- "pedantic", -- be pedantic
}

local other_cxflags = {
    "-Wswitch-default",     -- warn if no default case in a switch statement
}

set_languages("c++23")
set_toolchains("gcc")

target("antlr-g4")
    set_kind("static")
    add_includedirs("csrc/antlr", {public = true})
    add_files("csrc/antlr/*.cpp")
    add_packages("antlr4-runtime")

-- set macro DEBUG for debug mode


target("error-handler")
    set_kind("static")
    set_warnings(warnings)
    add_cxflags(other_cxflags)
    add_includedirs("csrc/include/utils", {public = false})
    add_files("csrc/cpp/utils/*.cpp")

target("LTL")
    set_kind("binary")
    add_deps("antlr-g4", "error-handler")
    set_warnings(warnings)
    add_cxflags(other_cxflags)
    add_includedirs("csrc/include")
    add_files("csrc/cpp/*.cpp")
    add_packages("antlr4-runtime")
    if is_mode("debug") then
        add_defines("_DARK_DEBUG")
    end

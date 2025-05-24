add_requires("antlr4-runtime 4.13.2", "argparse 3.1")
add_rules("mode.debug", "mode.release")

local warnings = {
    "all",      -- turn on all warnings
    "extra",    -- turn on extra warnings
    "error",    -- treat warnings as errors
    "pedantic", -- be pedantic
}

local other_cxflags = {
    "-Wswitch-default",     -- warn if no default case in a switch statement
}

set_languages("c++23")

target("antlr-g4")
    set_kind("static")
    add_includedirs("csrc/antlr", {public = true})
    add_files("csrc/antlr/*.cpp")
    add_packages("antlr4-runtime")

target("error-handler")
    set_kind("static")
    set_warnings(warnings)
    add_cxflags(other_cxflags)
    add_includedirs("csrc/include")
    add_files("csrc/cpp/utils/*.cpp")

target("LTL")
    set_kind("binary")
    add_deps("antlr-g4", "error-handler")
    set_warnings(warnings)
    add_cxflags(other_cxflags)
    add_includedirs("csrc/include")
    add_files("csrc/cpp/*.cpp")
    add_packages("antlr4-runtime", "argparse")
    if is_mode("debug") then
        add_defines("_DARK_DEBUG")
    end
    set_rundir("$(projectdir)")

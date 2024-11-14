set_languages("cxx17")
set_arch("x86")
option("SAFETYHOOKPATH")

target("safetyhook")
    set_kind("static")
	add_files(
		"$(SAFETYHOOKPATH)/src/*.cpp",
		"$(SAFETYHOOKPATH)/zydis/Zydis.c"
	)

	add_includedirs(
		"$(SAFETYHOOKPATH)/include",
		"$(SAFETYHOOKPATH)/zydis"
	)

	add_defines("NDEBUG")

	if is_plat("windows") then
		set_toolchains("msvc")
		add_cxflags("/W3", "/w14640", "/wd4819", "/Ox", "/Oy-", "/EHsc", "/MT", "/Z7")
	else
		set_toolchains("clang")
		add_cxflags(
			"-Wall", "-Wextra",
			"-Wshadow",
			"-Wnon-virtual-dtor",
			"-Wno-unused-const-variable",
			"-Wno-unused-function",
			"-fvisibility=hidden", 
			"-fvisibility-inlines-hidden", 
			"-fPIC", "-O3", "-g3"
		)
	end

target("main")
	set_kind("binary")
	add_deps("safetyhook")

	add_files("src/*.cpp")
	add_includedirs(
		"src",
		"$(SAFETYHOOKPATH)/include",
		"$(SAFETYHOOKPATH)/zydis"
	)

	set_toolchains("clang")
	add_cxflags("-O0", "-g3")
	add_ldflags("-static-libstdc++", "-static-libgcc")

	after_build(function (target)
		os.cp(path.join(target:targetdir(), target:filename()), ".")
    end)


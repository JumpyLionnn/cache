local p = premake
p.modules.clangd = {}
local m = p.modules.clangd

newaction {
    trigger = "clangd",
    description = "Export project information for the clangd language server",

    onStart = function() 
        print("Starting clangd generation")
    end,

    onWorkspace = function(wks)
    end,

    onProject = function(prj)
    end,

    execute = function()
        p.eol("\n")
        p.indent("    ")
        local dir = {}
        dir.location = path.join(_MAIN_SCRIPT_DIR, ".clangd")
        p.generate(dir, "", m.generateFile)
    end,

    onEnd = function()
        print("Clangd genration complete")
    end,
}

function m.generateFile()
    local i = 0
    for wks in p.global.eachWorkspace() do
        printf("Generating clangd for workspace '%s'", wks.name)
        for prj in p.workspace.eachproject(wks) do
            printf("Generating clangd for project '%s'", prj.name)
            if i > 0 then
                p.w("---")
                p.w("")
            end
            m.onProject(prj, _MAIN_SCRIPT_DIR)
            i = i + 1
        end
    end
end

function m.onProject(prj, rootDir)
    if p.project.isc(prj) or p.project.iscpp(prj) then
        local cfg = p.project.getfirstconfig(prj)
        local toolset = m.getToolSet()
        local pchPath = m.getPchPath(cfg)
        local args = {}
        args = table.join(args, toolset.getdefines(cfg.defines))
        args = table.join(args, toolset.getundefines(cfg.undefines))
        args = table.join(args, m.getIncludeDirs(cfg.includedirs, cfg.sysincludedirs))

        if p.project.iscpp(prj) then
            args = table.join(args, toolset.getcppflags(cfg))
            args = table.join(args, toolset.getcxxflags(cfg))
            table.insert(args, "-xcpp")
        else
            args = table.join(args, toolset.getcflags(cfg))
            table.insert(args, "-xc")
        end
        args = table.join(args, cfg.buildoptions)

        if pchPath ~= nil then
            m.writeArgs({pchPath}, args, prj, cfg, toolset, rootDir);
            p.push("Diagnostics:")
            p.w("UnusedIncludes: None")
            p.pop("")
            p.w("---")
            p.w("")
        end
            
        local sourceFiles = {}
        for _, f in ipairs(cfg.files) do
            if path.iscfile(f) or path.iscppfile(f) then
                f = path.translate(f, "/")
                f = path.getabsolute(f)
                table.insert(sourceFiles, f)
            end
        end

        m.writeArgs(sourceFiles, args, prj, cfg, toolset, rootDir)
        p.push("Diagnostics:")
        p.push("Includes:")
        if pchPath ~= nil then
            p.w("IgnoreHeader: [%s]", path.getname(pchPath))
        end
        p.w("AnalyzeAngledIncludes: true") -- will only work once clangd 19 is released
        p.pop(2)
        p.w("---")
        p.w("")

        local headerFiles = {}
        for _, f in ipairs(cfg.files) do
            if path.iscppheader(f) then
                f = path.translate(f, "/")
                f = path.getabsolute(f)
                if f ~= pchPath then
                    table.insert(headerFiles, f)
                end
            end
        end
        if pchPath ~= nil then
            table.insert(args, "--include=" .. pchPath)
        end
        m.writeArgs(headerFiles, args, prj, cfg, toolset, rootDir)
        
    end

end


function m.writeArgs(files, args, prj, cfg, toolset, rootDir)
    p.push("If:")
    p.push("PathMatch: [")
    for i, file in ipairs(files) do
        file = path.getrelative(rootDir, file)
        file = string.gsub(file, "%.", "\\.")
        if i < #files then
            p.w("%s,", file)
        else
            p.w(file)
        end
    end
    p.pop("]")
    p.pop("")


    p.push("CompileFlags:")
    p.push("Add: [")
    for i, arg in ipairs(args) do
        if i < #args then
            p.w("%s,", arg)
        else
            p.w(arg)
        end
    end

    p.pop("]")
    local tool = iif(p.project.iscpp(prj), "cxx", "c")
    local toolname = iif(cfg.prefix, toolset.gettoolname(cfg, tool), toolset.tools[tool])
    if toolname == nil then
        toolname = "clang"
    end
    p.w("Compiler: %s", toolname)
    p.pop("")
end

function m.getIncludeDirs(dirs, extDirs)
    local flags = {}
    for _, dir in ipairs(dirs) do
        dir = path.getabsolute(dir)
        dir = path.translate(dir, "/")
        table.insert(flags, "-I" .. p.quoted(dir))
    end
    for _, dir in ipairs(extDirs) do
        dir = p.project.getabsolute(dir)
        dir = path.translate(dir, "/")
        table.insert(flags, "-isystem" .. p.quoted(dir))
    end
    return flags
end

function m.getPchPath(cfg)
    local pch = p.tools.gcc.getpch(cfg)
    if pch == nil then
        return nil
    else
        pch = path.join(cfg.location, pch)
        return path.getabsolute(pch)
    end
end

function m.getToolSet()
    local toolset = p.tools["clang"]
    if not toolset then
        error("Can't get the clang toolset")
    end
    return toolset
end

return m


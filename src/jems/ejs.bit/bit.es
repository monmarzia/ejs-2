#!/usr/bin/env ejs

/*
    bit.es -- Build It! -- Embedthis Build It Framework

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */
module embedthis.bit {

require ejs.unix

public class Bit {
    public var initialized: Boolean

    private static const VERSION: Number = 0.2
    private static const MAIN: Path = Path('main.bit')
    private static const START: Path = Path('start.bit')
    private static const supportedOS = ['freebsd', 'linux', 'macosx', 'solaris', 'vxworks', 'windows']
    private static const supportedArch = ['arm', 'i64', 'mips', 'sparc', 'x64', 'x86']
    private static const minimalCflags = [ 
        '-w', '-g', '-Wall', '-Wno-deprecated-declarations', '-Wno-unused-result', '-Wshorten-64-to-32']

    /*
        Filter for files that look like temp files and should not be installed
     */
    private const TempFilter = /\.makedep$|\.o$|\.pdb$|\.tmp$|\.save$|\.sav$|OLD|\/Archive\/|\/sav\/|\/save\/|oldFiles|\.libs\/|\.nc|\.orig|\.svn|\.git|^\.[a-zA-Z_]|\.swp$|\.new$|\.nc$|.DS_Store/

    private var appName: String = 'bit'
    private var args: Args
    private var currentBitFile: Path?
    private var currentPack: String?
    private var currentPlatform: String?
    private var envSettings: Object
    private var local: Object
    private var localPlatform: String
    private var localBin: Path
    private var missing = null
    private var options: Object = { control: {}}
    private var out: Stream
    private var platforms: Array
    private var rest: Array

    private var home: Path
    private var bareBit: Object = { platforms: [], platform: {}, dir: {}, settings: {
        required: [], optional: [],
    }, packs: {}, targets: {}, env: {}, globals: {}, }

    private var bit: Object = {}
    private var gen: Object
    private var platform: Object
    private var genout: TextStream
    private var generating: String?

    private var defaultTargets: Array
    private var originalTargets: Array
    private var selectedTargets: Array

    private var posix = ['macosx', 'linux', 'unix', 'freebsd', 'solaris']
    private var windows = ['windows', 'wince']
    private var start: Date
    private var targetsToBuildByDefault = { exe: true, file: true, lib: true, build: true }
    private var targetsToBlend = { exe: true, lib: true, obj: true, action: true, build: true, clean: true }
    private var targetsToClean = { exe: true, file: true, lib: true, obj: true, build: true }

    private var argTemplate = {
        options: {
            benchmark: { alias: 'b' },
            bit: { range: String },
            configure: { range: String },
            'continue': { alias: 'c' },
            debug: {},
            depth: { range: Number},
            diagnose: { alias: 'd' },
            dump: { },
            endian: { range: ['little', 'big'] },
            file: { range: String },
            force: {},
            gen: { range: String, separator: Array, commas: true },
            help: { },
            import: { alias: 'init' },
            keep: { alias: 'k' },
            log: { alias: 'l', range: String },
            out: { range: String },
            nocross: {},
            pre: { range: String, separator: Array },
            platform: { range: String, separator: Array },
            pre: { },
            prefix: { range: String, separator: Array },
            profile: { range: String },
            rebuild: { alias: 'r'},
            release: {},
            quiet: { alias: 'q' },
            'set': { range: String, separator: Array },
            show: { alias: 's'},
            static: { },
            unicode: {},
            unset: { range: String, separator: Array },
            verbose: { alias: 'v' },
            version: { alias: 'V' },
            why: { alias: 'w' },
            'with': { range: String, separator: Array },
            without: { range: String, separator: Array },
        },
        usage: usage
    }

    function usage(): Void {
        print('\nUsage: bit [options] [targets|actions] ...\n' +
            '  Options:\n' + 
            '    --benchmark                        # Measure elapsed time\n' +
            '    --configure path-to-source         # Configure for building\n' +
            '    --continue                         # Continue on errors\n' +
            '    --debug                            # Same as --profile debug\n' +
            '    --depth                            # Set utest depth\n' +
            '    --diagnose                         # Emit diagnostic trace \n' +
            '    --dump                             # Dump the full project bit file\n' +
            '    --endian [big|little]              # Define the CPU endianness\n' +
            '    --file file.bit                    # Use the specified bit file\n' +
            '    --force                            # Override warnings\n' +
            '    --gen [make|nmake|sh|vs|xcode]     # Generate project file\n' + 
            '    --help                             # Print help message\n' + 
            '    --import                           # Import standard bit configuration\n' + 
            '    --keep                             # Keep intermediate files\n' + 
            '    --log logSpec                      # Save errors to a log file\n' +
            '    --nocross                          # Build natively\n' +
            '    --out path                         # Save output to a file\n' +
            '    --platform os-arch[-cpu]           # Build for specified platform\n' +
            '    --pre                              # Pre-process a source file to stdout\n' +
            '    --profile [debug|release|...]      # Use the build profile\n' +
            '    --quiet                            # Quiet operation. Suppress trace \n' +
            '    --set [feature=value]              # Enable and a feature\n' +
            '    --show                             # Show commands executed\n' +
            '    --static                           # Make static without shared libraries\n' +
            '    --rebuild                          # Rebuild all specified targets\n' +
            '    --release                          # Same as --profile release\n' +
            '    --unicode                          # Set char size to wide (unicode)\n' +
            '    --unset feature                    # Unset a feature\n' +
            '    --version                          # Dispay the bit version\n' +
            '    --verbose                          # Trace operations\n' +
            '    --with PACK[=PATH]                 # Build with package at PATH\n' +
            '    --without PACK                     # Build without a package\n' +
            '')
        if (START.exists) {
            try {
                b.makeBit(Config.OS + '-' + Config.CPU, START)
                global.bit = bit = b.bit
                let bitfile: Path = START
                for (let [index,platform] in bit.platforms) {
                    bitfile = bitfile.dirname.join(platform).joinExt('bit')
                    if (bitfile.exists) {
                        b.makeBit(platform, bitfile)
                        b.prepBuild()
                    }
                    break
                }
                if (bit.usage) {
                    print('Feature Selection:')
                    for (let [item,msg] in bit.usage) {
                        print('    --set %-28s # %s' % [item + '=value', msg])
                    }
                    print('')
                }
            } catch (e) { print('CATCH: ' + e)}
        }
        App.exit(1)
    }

    function main() {
        let start = new Date
        global._b = this
        home = App.dir
        args = Args(argTemplate)
        options = args.options
        try {
            setup(args)
            if (options.import) {
                import()
                App.exit()
            } 
            if (options.configure) {
                configure()
            }
            if (options.gen) {
                generate()
            }
            if (!options.file) {
                let file = findStart()
                App.chdir(file.dirname)
                home = App.dir
                options.file = file.basename
            }
            process(options.file)
        } catch (e) {
            let msg: String
            if (e is String) {
                App.log.error('' + e + '\n')
            } else {
                App.log.error('' + ((options.diagnose) ? e : e.message) + '\n')
            }
            App.exit(2)
        }
        if (options.benchmark) {
            trace('Benchmark', 'Elapsed time %.2f' % ((start.elapsed / 1000)) + ' secs.')
        }
    }

    /*
        Parse arguments
     */
    function setup(args: Args) {
        options.control = {}
        if (options.version) {
            print(version)
            App.exit(0)
        }
        if (options.help) {
            usage()
            App.exit(0)
        }
        if (options.log) {
            App.log.redirect(options.log)
            App.mprLog.redirect(options.log)
        }
        out = (options.out) ? File(options.out, 'w') : stdout

        if (options.debug) {
            options.profile = 'debug'
        }
        if (options.release) {
            options.profile = 'release'
        }
        if (args.rest.contains('configure')) {
            options.configure = Path('.')
        } else if (options.configure) {
            args.rest.push('configure')
            options.configure = Path(options.configure)
        }
        if (args.rest.contains('generate')) {
            if (Config.OS == 'windows') {
                options.gen = ['sh', 'nmake', 'vs']
            } else if (Config.OS == 'macosx') {
                options.gen = ['sh', 'make', 'xcode']
            } else {
                options.gen = ['sh', 'make']
            }
        } else if (options.gen) {
            args.rest.push('generate')
        }
        if (args.rest.contains('dump')) {
            options.dump = true
        } else if (options.dump) {
            args.rest.push('dump')
            options.dump = true
        }
        if (args.rest.contains('rebuild')) {
            options.rebuild = true
        }
        if (args.rest.contains('import')) {
            options.import = true
        }
        if (options.profile && !(options.configure || options.gen)) {
            App.log.error('Can only set profile when configuring or generating')
            usage()
        }
        localPlatform =  Config.OS + '-' + Config.CPU
        platforms = options.platform || []
        if (platforms.length == 0) {
            platforms.insert(0, localPlatform)
        }
        platforms.transform(function(e) e == 'local' ? localPlatform : e).unique()

        if (options.gen) {
            if (platforms.length != 1) {
                App.log.error('Can only generate for one platform at a time')
                usage()
            }
            localPlatform = platforms[0]
            if (!Path(localPlatform + '.bit').exists) {
                trace('Generate', 'Create platform bit file: ' + localPlatform + '.bit')
                if (!options.configure) {
                    App.args.push('-without')
                    App.args.push('all')
                    options.configure = Path('.')
                }
            }
            /* Must continue if probe can't locate tools, but does know a default */
            options['continue'] = true
        }
        let [os, arch] = localPlatform.split('-') 
        validatePlatform(os, arch)
        local = {
            name: localPlatform,
            os: os,
            arch: arch,
            like: like(os),
        }

        /*
            The --set|unset|with|without switches apply to the previous --platform switch
         */
        let platform = localPlatform
        let poptions = options.control[platform] = {}
        for (i = 1; i < App.args.length; i++) {
            let arg = App.args[i]
            if (arg == '--platform' || arg == '-platform') {
                platform = App.args[++i]
                poptions = options.control[platform] = {}
            } else if (arg == '--with' || arg == '-with') {
                poptions['with'] ||= []
                poptions['with'].push(App.args[++i])
            } else if (arg == '--without' || arg == '-without') {
                poptions.without ||= []
                poptions.without.push(App.args[++i])
            } else if (arg == '--set' || arg == '-set') {
                /* Map set to enable */
                poptions.enable ||= []
                poptions.enable.push(App.args[++i])
            } else if (arg == '--unset' || arg == '-unset') {
                /* Map set to disable */
                poptions.disable ||= []
                poptions.disable.push(App.args[++i])
            }
        }
        if (options.depth) {
            poptions.enable ||= []
            poptions.enable.push('depth=' + options.depth)
        }
        if (options.static) {
            poptions.enable ||= []
            poptions.enable.push('static=true')
        }
        if (options.unicode) {
            poptions.enable ||= []
            poptions.enable.push(Config.OS == 'windows' ? 'charLen=2' : 'charLen=4')
        }
        originalTargets = selectedTargets = args.rest
        bareBit.options = options
    }

    /*  
        Configure and initialize for building. This generates platform specific bit files.
     */
    function configure() {
        vtrace('Load', 'Preload main.bit to determine required platforms')
        quickLoad(options.configure.join(MAIN))
        let settings = bit.settings
        if (settings.platforms && !options.gen && !options.nocross) {
            if (!(settings.platforms is Array)) {
                settings.platforms = [settings.platforms]
            }
            settings.platforms = settings.platforms.transform(function(e) e == 'local' ? localPlatform : e)
            platforms = (settings.platforms + platforms).unique()
        }
        for each (platform in platforms) {
            currentPlatform = platform
            trace('Configure', platform)
            makeBit(platform, options.configure.join(MAIN))
            findPacks()
            genPlatformBitFile(platform)
            makeOutDirs()
            makeBitHeader(platform)
            importPackFiles()
            bit.cross = true
        }
        bit.cross = false
        if (!options.gen) {
            genStartBitFile(platforms[0])
        }
    }

    function genStartBitFile(platform) {
        let nbit = { }
        nbit.platforms = platforms
        trace('Generate', START)
        let data = '/*\n    start.bit -- Startup Bit File for ' + bit.settings.title + 
            '\n */\n\nBit.load(' + 
            serialize(nbit, {pretty: true, indent: 4, commas: true, quotes: false}) + ')\n'
        START.write(data)
    }

    function genPlatformBitFile(platform) {
        let nbit = {}
        blend(nbit, {
            blend: [ 
                '${SRC}/main.bit',
            ],
            platform: bit.platform,
            dir: { 
                src: bit.dir.src.absolute.portable,
                top: bit.dir.top.portable,
            },
            settings: { configured: true },
            packs: bit.packs,
            env: bit.env,
        })
        nbit.platform.configuration = platform + '-' + '${platform.profile}'

        for (let [key, value] in bit.settings) {
            if (!bit.userSettings[key]) {
                nbit.settings[key] = value
            }
        }
        if (envSettings) {
            blend(nbit, envSettings, {combine: true})
        }
        if (bit.dir.bits != Config.Bin.join('bits')) {
            nbit.dir.bits = bit.dir.bits
        }

        /*
            Ejscript in appweb uses this to mark tagets as being prebuilt. See packs/ejscript.bit
         */
        for (let [tname,target] in bit.targets) {
            if (target.built) {
                nbit.targets ||= {}
                nbit.targets[tname] = { built: true}
            }
        }
        if (nbit.settings) {
            Object.sortProperties(nbit.settings);
        }
        if (options.configure) {
            let path: Path = Path(platform).joinExt('bit')
            trace('Generate', path)
            let data = '/*\n    ' + platform + '.bit -- Build ' + bit.settings.title + ' for ' + platform + 
                '\n */\n\nBit.load(' + 
                serialize(nbit, {pretty: true, indent: 4, commas: true, quotes: false}) + ')\n'
            path.write(data)
        }
        if (options.show && options.verbose) {
            trace('Configuration', bit.settings.title + ' for ' + platform + 
                '\nsettings = ' +
                serialize(bit.settings, {pretty: true, indent: 4, commas: true, quotes: false}) +
                '\npacks = ' +
                serialize(nbit.packs, {pretty: true, indent: 4, commas: true, quotes: false}))
        }
    }

    function makeBitHeader(platform) {
        let path = bit.dir.inc.join('bit.h')
        let f = TextStream(File(path, 'w'))
        f.writeLine('/*\n    bit.h -- Build It Configuration Header for ' + platform + '\n\n' +
                '    This header is generated by Bit during configuration. To change settings, re-run\n' +
                '    configure or define variables in your Makefile to override these default values.\n */\n')
        writeDefinitions(f, platform)
        f.close()
        for (let [tname, target] in bit.targets) {
            runTargetScript(target, 'postconfig')
        }
    }

    function def(f: TextStream, key, value) {
        f.writeLine('#ifndef ' + key)
        f.writeLine('    #define ' + key + ' ' + value)
        f.writeLine('#endif')
    }

    function writeDefinitions(f: TextStream, platform) {
        let settings = bit.settings.clone()
        if (options.endian) {
            settings.endian = options.endian == 'little' ? 1 : 2
        }
        Object.sortProperties(settings)
        f.writeLine('/* Settings */')
        for (let [key,value] in settings) {
            if (key.match(/[a-z]/)) {
                key = 'BIT_' + key.replace(/[A-Z]/g, '_$&').replace(/-/g, '_').toUpper()
            }
            if (value is Number) {
                def(f, key, value)
            } else if (value is Boolean) {
                def(f, key, value cast Number)
            } else {
                def(f, key, '"' + value + '"')
            }
        }

        f.writeLine('\n/* Prefixes */')
        let base = (settings.name == 'ejs') ? bit.prefixes.productver : bit.prefixes.product
        def(f, 'BIT_CFG_PREFIX', '"' + bit.prefixes.config + '"')
        def(f, 'BIT_BIN_PREFIX', '"' + bit.prefixes.bin + '"')
        def(f, 'BIT_INC_PREFIX', '"' + bit.prefixes.inc + '"')
        def(f, 'BIT_LOG_PREFIX', '"' + bit.prefixes.log + '"')
        def(f, 'BIT_PRD_PREFIX', '"' + bit.prefixes.product + '"')
        def(f, 'BIT_SPL_PREFIX', '"' + bit.prefixes.spool + '"')
        def(f, 'BIT_SRC_PREFIX', '"' + bit.prefixes.src + '"')
        def(f, 'BIT_VER_PREFIX', '"' + bit.prefixes.productver + '"')
        def(f, 'BIT_WEB_PREFIX', '"' + bit.prefixes.web + '"')

        /* Suffixes */
        f.writeLine('\n/* Suffixes */')
        def(f, 'BIT_EXE', '"' + bit.ext.dotexe + '"')
        def(f, 'BIT_SHLIB', '"' + bit.ext.dotshlib + '"')
        def(f, 'BIT_SHOBJ', '"' + bit.ext.dotshobj + '"')
        def(f, 'BIT_LIB', '"' + bit.ext.dotlib + '"')
        def(f, 'BIT_OBJ', '"' + bit.ext.doto + '"')

        /* Build profile */
        f.writeLine('\n/* Profile */')
        let args = 'bit ' + App.args.slice(1).join(' ')
        def(f, 'BIT_CONFIG_CMD', '"' + args + '"')
        def(f, 'BIT_' + settings.product.toUpper() + '_PRODUCT', '1')
        def(f, 'BIT_PROFILE', '"' + bit.platform.profile + '"')

        /* Architecture settings */
        f.writeLine('\n/* Miscellaneous */')
        if (settings.charlen) {
            def(f, 'BIT_CHAR_LEN', settings.charlen)
            if (settings.charlen == 1) {
                def(f, 'BIT_CHAR', 'char')
            } else if (settings.charlen == 2) {
                def(f, 'BIT_CHAR', 'short')
            } else if (settings.charlen == 4) {
                def(f, 'BIT_CHAR', 'int')
            }
        }
        let ver = settings.version.split('.')
        def(f, 'BIT_MAJOR_VERSION',  ver[0])
        def(f, 'BIT_MINOR_VERSION', ver[1])
        def(f, 'BIT_PATCH_VERSION', ver[2])
        def(f, 'BIT_VNUM',  ((((ver[0] * 1000) + ver[1]) * 1000) + ver[2]))

        f.writeLine('\n/* Packs */')
        let packs = bit.packs.clone()
        Object.sortProperties(packs)
        for (let [pname, pack] in packs) {
            if (pname == 'compiler') {
                pname = 'cc'
            }
            def(f, 'BIT_PACK_' + pname.toUpper(), pack.enable ? '1' : '0')
        }
        for (let [pname, pack] in packs) {
            if (pack.enable && pack.definitions) {
                for each (define in pack.definitions) {
                    if (define.match(/-D(.*)=(.*)/)) {
                        let [key,value] = define.match(/-D(.*)=(.*)/).slice(1)
                        def(f, key, value)
                    } else {
                        f.writeLine('#define ' + define.trimStart('-D'))
                    }
                }
            }
        }
    }

    /*
        Apply command line --with/--without --enable/--disable options
     */
    function applyCommandLineOptions(platform) {
        var poptions = options.control[platform]
        if (!poptions) {
            return
        }
        /* Disable/enable was originally --unset|--set */
        for each (field in poptions.disable) {
            bit.settings[field] = false
        }
        for each (field in poptions.enable) {
            let [field,value] = field.split('=')
            if (value == 'true') {
                value = true
            } else if (value == 'false') {
                value = false
            } else if (value.isDigit) {
                value = value cast Number
            }
            if (value == undefined) {
                value = true
            }
            bit.settings[field] = value
        }
        for each (field in poptions['with']) {
            let [field,value] = field.split('=')
            if (value) {
                bit.packs[field] = { enable: true, path: Path(value) }
            }
        }
        for each (field in poptions['without']) {
            if ((field == 'all' || field == 'own') && bit.settings['without-' + field]) {
                for each (f in bit.settings['without-' + field]) {
                    bit.packs[f] = { enable: false, diagnostic: 'configured --without ' + f }
                }
                continue
            }
            bit.packs[field] = { enable: false, diagnostic: 'configured --without ' + field }
        }
    }

    let envTools = {
        AR: '+lib',
        CC: '+compiler',
        LD: '+linker',
    }

    let envFlags = {
        CFLAGS:  '+compiler',
        DFLAGS:  '+defines',
        IFLAGS:  '+includes',
        LDFLAGS: '+linker',
    }

    /*
        Examine environment for flags and apply
        NOTE: this is for cross platforms ONLY
     */
    function applyEnv() {
        if (!bit.cross) {
            return
        }
        envSettings = { packs: {}, defaults: {} }
        for (let [key, tool] in envTools) {
            let path = App.getenv(key)
            if (path) {
                envSettings.packs[tool] ||= {}
                envSettings.packs[tool].path = path
                envSettings.packs[tool].enable = true
            }
        }
        for (let [flag, option] in envFlags) {
            let value = App.getenv(flag)
            if (value) {
                envSettings.defaults[option] ||= []
                envSettings.defaults[option] += value.replace(/^-I/, '').split(' ')
            }
        }
    }

    /*
        Import pack files
     */
    function importPackFiles() {
        for (let [pname, pack] in bit.packs) {
            for each (file in pack.imports) {
                vtrace('Import', file)
                if (file.extension == 'h') {
                    cp(file, bit.dir.inc)
                } else {
                    if (bit.platform.like == 'windows') {
                        let target = bit.dir.lib.join(file.basename).relative
                        let old = target.replaceExt('old')
                        vtrace('Preserve', 'Active library ' + target + ' as ' + old)
                        old.remove()
                        target.rename(old)
                    }
                    cp(file, bit.dir.lib)
                }
            }
        }
    }

    /*
        Apply the selected build profile
     */
    function applyProfile() {
        if (options.profile && bit.profiles) {
            blend(bit, bit.profiles[options.profile], {combine: true})
        }
    }

    /*
        Search for enabled packs in the system
     */
    function findPacks() {
        let settings = bit.settings
        if (!settings.required && !settings.optional) {
            return
        }
        trace('Search', 'For tools and extension packages')
        vtrace('Search', 'Packages: ' + [settings.required + settings.optional].join(' '))
        let packs = (settings.required + settings.optional).sort().unique()
        for each (pack in settings.required + settings.optional) {
            if (bit.packs[pack] && !bit.packs[pack].enable) {
                if (settings.required.contains(pack)) { 
                    throw 'Required pack ' + pack + ' is not enabled'
                }
                continue
            }
            let path = bit.dir.bits.join('packs', pack + '.pak')
            if (!path.exists) {
                for each (d in settings.packs) {
                    path = bit.dir.src.join(d, pack + '.pak')
                    if (path.exists) {
                        break
                    }
                }
            }
            if (path.exists) {
                try {
                    bit.packs[pack] ||= {enable: true}
                    currentPack = pack
                    loadBitFile(path)
                } catch (e) {
                    if (!(e is String)) {
                        App.log.debug(0, e)
                    }
                    let kind = settings.required.contains(pack) ? 'Required' : 'Optional'
                    whyMissing(kind + ' package "' + pack + '". ' + e)
                    let p = bit.packs[pack] ||= {}
                    p.enable = false
                    p.diagnostic = "" + e
                    if (kind == 'Required' && !options['continue']) {
                        throw e
                    }
                }
            }
            let p = bit.packs[pack]
            if (p) {
                let desc = p.description || pack
                if (p && p.enable && p.path) {
                    if (options.verbose) {
                        vtrace('Found', desc + ' at ' + p.path)
                    } else if (options.show) {
                        trace('Found', desc + ' at:\n                 ' + p.path)
                    } else {
                        trace('Found', desc)
                    }
                } else {
                    trace('Not Found', 'Optional: ' + desc)
                }
            } else {
                trace('Not Found', 'Optional: ' + pack)
            }
        }
        castDirTypes()
    }

    /*
        Probe for a file and locate
        Will throw an exception if the file is not found, unless {continue, default} specified in control options
     */
    public function probe(file: Path, control = {}): Path {
        let path: Path?
        let search = [], dir
        if (file.exists) {
            path = file
        } else {
            if (dir = bit.packs[currentPack].path) {
                search.push(dir)
            }
            if (control.search) {
                if (!(control.search is Array)) {
                    control.search = [control.search]
                }
                search += control.search
            }
            App.log.debug(2, "Probe for " + file + ' in search path: ' + search)
            for each (let s: Path in search) {
                App.log.debug(2, "Probe for " + s.join(file) + ' exists: ' + s.join(file).exists)
                if (s.join(file).exists) {
                    path = s.join(file)
                    break
                }
            }
            if (!control.nopath) {
                path ||= Cmd.locate(file)
            }
        }
        if (!path) {
            if (options.why) {
                trace('Probe', 'File ' + file)
                trace('Search', search.join(' '))
            }
            if (options['continue'] && control.default) {
                return control.default
            }
            throw 'File ' + file + ' not found for package ' + currentPack + '.'
        }
        App.log.debug(2, 'Probe for ' + file + ' found at ' + path)
        if (control.fullpath) {
            return path.portable
        }
        /*
            Trim the pattern we have been searching for and return the base prefix only
            Need to allow for both / and \ separators
         */
        let pat = RegExp('.' + file.toString().replace(/[\/\\]/g, '.') + '$')
        return path.portable.name.replace(pat, '')
    }

    function process(bitfile: Path) {
        if (!bitfile.exists) {
            throw 'Can\'t find ' + bitfile
        }
        quickLoad(bitfile)

        if (bit.platforms) {
            platforms = bit.platforms
            for (let [index,platform] in bit.platforms) {
                bitfile = bitfile.dirname.join(platform).joinExt('bit')
                makeBit(platform, bitfile)
                if (index == (bit.platforms.length - 1)) {
                    bit.platform.last = true
                }
                prepBuild()
                build()
                if (bit.platforms.length > 1 || bit.cross) {
                    trace('Complete', bit.platform.configuration)
                }
                bit.cross = true
            }
        } else {
            platforms = bit.platforms = [localPlatform]
            makeBit(localPlatform, bitfile)
            bit.platform.last = true
            prepBuild()
            build()
        }
    }

    public function loadModules() {
        App.log.debug(2, "Bit Modules: " + serialize(bit.modules, {pretty: true}))
        for each (let module in bit.modules) {
            App.log.debug(2, "Load bit module: " + module)
            try {
                global.load(module)
            } catch (e) {
                throw new Error('When loading: ' + module + '\n' + e)
            }
        }
    }

    public function loadBitFile(path) {
        let saveCurrent = currentBitFile
        try {
            currentBitFile = path.portable
            vtrace('Loading', currentBitFile)
            global.load(path)
        } finally {
            currentBitFile = saveCurrent
        }
    }

    function rebase(home: Path, o: Object, field: String) {
        if (o[field] is Array) {
            for (i in o[field]) {
                if (!o[field][i].startsWith('${')) {
                    o[field][i] = home.join(o[field][i])
                }
            }
        } else if (o[field]) {
            if (!o[field].startsWith('${')) {
                o[field] = home.join(o[field])
            }
        }
    }

    function fixup(o, ns) {
        let home = currentBitFile.dirname
        for (i in o.modules) {
            o.modules[i] = home.join(o.modules[i])
        }
        for (i in o['+modules']) {
            o['+modules'][i] = home.join(o['+modules'][i])
        }
        //  TODO Functionalize
        //  TODO add support for shell
        if (o.defaults) {
            rebase(home, o.defaults, 'includes')
            rebase(home, o.defaults, '+includes')
            for (let [when,item] in o.defaults.scripts) {
                if (item is String) {
                    o.defaults.scripts[when] = [{ home: home, shell: 'ejs', script: item }]
                } else {
                    item.home ||= home
                }
            }
        }
        if (o.internal) {
            rebase(home, o.internal, 'includes')
            rebase(home, o.internal, '+includes')
            for (let [when,item] in o.internal.scripts) {
                if (item is String) {
                    o.internal.scripts[when] = [{ home: home, shell: 'ejs', script: item }]
                } else {
                    item.home ||= home
                }
            }
        }
        for (let [tname,target] in o.targets) {
            target.name ||= tname
            target.home ||= home
            target.vars ||= {}
            if (target.path) {
                if (!target.path.startsWith('${')) {
                    target.path = target.home.join(target.path)
                }
            }
            //  TODO - what about other +fields
            rebase(home, target, 'includes')
            rebase(home, target, '+includes')
            rebase(home, target, 'headers')
            rebase(home, target, 'resources')
            rebase(home, target, 'sources')
            rebase(home, target, 'files')

            /* Convert strings scripts into an array of scripts structures */
            //  TODO - functionalize
            for (let [when,item] in target.scripts) {
                if (item is String) {
                    item = { shell: 'ejs', script: item  }
                    target.scripts[when] = [item]
                    item.home ||= home
                } else if (item is Array) {
                    item[0].home ||= home
                } else {
                    item.home ||= home
                }
            }
            if (target.build) {
                /*
                    Build scripts always run if doing a 'build'. Set the type to 'build'
                 */
                target.type ||= 'build'
                target.scripts ||= {}
                target.scripts['build'] ||= []
                target.scripts['build'] += [{ home: home, shell: 'ejs', script: target.build }]
                delete target.build
            }
            if (target.action) {
                /*
                    Actions do not run at 'build' time. They have a type of 'action' so they do not run by default
                    unless requested as an action on the command line AND they don't have the same type as a target. 
                 */
                target.type ||= 'action'
                target.scripts ||= {}
                target.scripts['build'] ||= []
                target.scripts['build'] += [{ home: home, shell: 'ejs', script: target.action, ns: ns }]
                delete target.action
            }
            if (target.shell) {
                target.type ||= 'action'
                target.scripts ||= {}
                target.scripts['build'] ||= []
                target.scripts['build'] += [{ home: home, shell: 'bash', script: target.shell }]
                delete target.shell
            }
            /*
                Blend internal for only the targets in this file
             */
            if (o.internal) {
                runTargetScript({scripts: o.internal.scripts}, 'preblend')
                blend(target, o.internal, {combine: true})
            }
            if (target.inherit) {
                blend(target, o[target.inherit], {combine: true})
            }
        }
    }

    /*
        Load a bit file object
     */
    public function loadBitObject(o, ns = null) {
        let home = currentBitFile.dirname
        fixup(o, ns)
        /* 
            Blending is depth-first -- blend this bit object after loading bit files referenced in blend[]
            Special case for the local plaform bit file to provide early definition of platform and dir properties
         */
        if (o.dir) {
            blend(bit.dir, o.dir, {combine: true})
        }
        if (o.platform) {
            blend(bit.platform, o.platform, {combine: true})
        }
        if (!bit.quickLoad) {
            for each (path in o.blend) {
                bit.globals.BITS = bit.dir.bits
                bit.globals.SRC = bit.dir.src
                if (path.startsWith('?')) {
                    path = home.join(expand(path.slice(1), {fill: null}))
                    if (path.exists) {
                        loadBitFile(path)
                    } else {
                        vtrace('SKIP', 'Skip blending optional ' + path.relative)
                    }
                } else {
                    path = home.join(expand(path, {fill: null}))
                    loadBitFile(path)
                }
            }
        }
        /*
            Delay combining targets until blendDefaults. This is because 'combine: true' erases the +/- property prefixes.
            These must be preserved until blendDefaults.
         */
        if (o.targets) {
            bit.targets ||= {}
            bit.targets = blend(bit.targets, o.targets)
            delete o.targets
        }
        bit = blend(bit, o, {combine: true})

        if (o.scripts && o.scripts.onload && !bit.quickLoad) {
            runScript(o.scripts.onload, home)
        }
    }

    function findStart(): Path? {
        let lp = START
        if (lp.exists) {
            return lp
        }
        let base: Path = options.configure || '.'
        for (let d: Path = base; d.parent != d; d = d.parent) {
            let f: Path = d.join(lp)
            if (f.exists) {
                vtrace('Info', 'Using bit file ' + f)
                return f
            }
        }
        throw 'Can\'t find suitable ' + START + '.\nRun "configure" or "bit configure" first.'
        return null
    }

    function generate() {
        platforms = bit.platforms = [localPlatform]
        makeBit(localPlatform, localPlatform + '.bit')
        bit.original = {
            dir: bit.dir.clone(),
            platform: bit.platform.clone(),
        }
        if (options.profile) {
            bit.platform.profile = options.profile
            bit.platform.configuration = bit.platform.name + '-' + bit.platform.profile
            for (d in bit.dir) {
                if (d == 'bits') continue
                bit.dir[d] = bit.dir[d].replace(bit.original.platform.configuration, bit.platform.configuration)
            }
        }
        bit.platform.last = true
        prepBuild()
        generateProjects()
    }

    function generateProjects() {
        selectedTargets = defaultTargets
        if (generating) return
        gen = {
            configuration:  bit.platform.configuration
            compiler:       bit.defaults.compiler.join(' '),
            defines:        bit.defaults.defines.join(' '),
            includes:       bit.defaults.includes.map(function(e) '-I' + e).join(' '),
            linker:         bit.defaults.linker.join(' '),
            libpaths:       mapLibPaths(bit.defaults.libpaths)
            libraries:      mapLibs(bit.defaults.libraries).join(' ')
        }
        /*
        if (Config.OS == 'windows') {
            trace('Copy', 'Export *.def files')
            for each (f in bit.dir.bin.files('*.def')) {
                f.copy(bit.dir.src.join('projects', bit.settings.product + '-windows', f.basename))
            }
        }
        */
        for each (item in options.gen) {
            generating = item
            let base = bit.dir.proj.join(bit.settings.product + '-' + bit.platform.os + '-' + bit.platform.profile)
            // base.makeDir()
            let path = bit.original.dir.inc.join('bit.h')
            let hfile = bit.dir.src.join('projects', 
                    bit.settings.product + '-' + bit.platform.os + '-' + bit.platform.profile + '-bit.h')
            path.copy(hfile)
            trace('Generate', 'project header: ' + hfile.relative)
            if (generating == 'sh') {
                generateShell(base)
            } else if (generating == 'make') {
                generateMake(base)
            } else if (generating == 'nmake') {
                generateNmake(base)
            } else if (generating == 'vstudio' || generating == 'vs') {
                generateVstudio(base)
            } else if (generating == 'xcode') {
                generateXcode(base)
            } else {
                throw 'Unknown generation format: ' + generating
            }
            for each (target in bit.targets) {
                target.built = false
            }
        }
        generating = null
    }

    function generateShell(base: Path) {
        trace('Generate', 'project file: ' + base.relative + '.sh')
        let path = base.joinExt('sh')
        genout = TextStream(File(path, 'w'))
        genout.writeLine('#\n#   ' + path.basename + ' -- Build It Shell Script to build ' + bit.settings.title + '\n#\n')
        genEnv()
        genout.writeLine('ARCH="' + bit.platform.arch + '"')
        genout.writeLine('ARCH="`uname -m | sed \'s/i.86/x86/;s/x86_64/x64/\'`"')
        genout.writeLine('OS="' + bit.platform.os + '"')
        genout.writeLine('PROFILE="' + bit.platform.profile + '"')
        genout.writeLine('CONFIG="${OS}-${ARCH}-${PROFILE}' + '"')
        genout.writeLine('CC="' + bit.packs.compiler.path + '"')
        if (bit.packs.link) {
            genout.writeLine('LD="' + bit.packs.link.path + '"')
        }
        genout.writeLine('CFLAGS="' + gen.compiler + '"')
        genout.writeLine('DFLAGS="' + gen.defines + '"')
        genout.writeLine('IFLAGS="' + 
            repvar(bit.defaults.includes.map(function(path) '-I' + path.relative).join(' ')) + '"')
        genout.writeLine('LDFLAGS="' + repvar(gen.linker).replace(/\$ORIGIN/g, '\\$$ORIGIN') + '"')
        genout.writeLine('LIBPATHS="' + repvar(gen.libpaths) + '"')
        genout.writeLine('LIBS="' + gen.libraries + '"\n')
        genout.writeLine('[ ! -x ${CONFIG}/inc ] && ' + 
            'mkdir -p ${CONFIG}/inc ${CONFIG}/obj ${CONFIG}/lib ${CONFIG}/bin\n')
        genout.writeLine('[ ! -f ${CONFIG}/inc/bit.h ] && ' + 
            'cp projects/' + bit.settings.product + '-${OS}-${PROFILE}-bit.h ${CONFIG}/inc/bit.h')
        genout.writeLine('if ! diff ${CONFIG}/inc/bit.h projects/' + bit.settings.product + 
            '-${OS}-${PROFILE}-bit.h >/dev/null ; then')
        genout.writeLine('\tcp projects/' + bit.settings.product + '-${OS}-${PROFILE}-bit.h ${CONFIG}/inc/bit.h')
        genout.writeLine('fi\n')
        build()
        genout.close()
        path.setAttributes({permissions: 0755})
    }

    function generateMake(base: Path) {
        trace('Generate', 'project file: ' + base.relative + '.mk')
        let path = base.joinExt('mk')
        genout = TextStream(File(path, 'w'))
        genout.writeLine('#\n#   ' + path.basename + ' -- Makefile to build ' + 
            bit.settings.title + ' for ' + bit.platform.os + '\n#\n')
        genEnv()
        genout.writeLine('ARCH     ?= $(shell uname -m | sed \'s/i.86/x86/;s/x86_64/x64/\')')
        genout.writeLine('OS       ?= ' + bit.platform.os)
        genout.writeLine('CC       ?= ' + bit.packs.compiler.path)
        if (bit.packs.link) {
            genout.writeLine('LD       ?= ' + bit.packs.link.path)
        }
        genout.writeLine('PROFILE  ?= ' + bit.platform.profile)
        genout.writeLine('CONFIG   ?= $(OS)-$(ARCH)-$(PROFILE)\n')

        let cflags = gen.compiler
        for each (word in minimalCflags) {
            cflags = cflags.replace(word, '')
        }
        cflags += ' -w'
        genout.writeLine('CFLAGS   += ' + cflags.trim())
        genout.writeLine('DFLAGS   += ' + gen.defines.replace(/-DBIT_DEBUG/, ''))
        genout.writeLine('IFLAGS   += ' + 
            repvar(bit.defaults.includes.map(function(path) '-I' + reppath(path.relative)).join(' ')))
        let linker = defaults.linker.map(function(s) "'" + s + "'").join(' ')
        let ldflags = repvar(linker).replace(/\$ORIGIN/g, '$$$$ORIGIN').replace(/ '-g'/, '')
        genout.writeLine('LDFLAGS  += ' + ldflags)
        genout.writeLine('LIBPATHS += ' + repvar(gen.libpaths))
        genout.writeLine('LIBS     += ' + gen.libraries + '\n')

        genout.writeLine('CFLAGS-debug    := -DBIT_DEBUG -g')
        genout.writeLine('CFLAGS-release  := -O2')
        genout.writeLine('LDFLAGS-debug   := -g')
        genout.writeLine('LDFLAGS-release := ')
        genout.writeLine('CFLAGS          += $(CFLAGS-$(PROFILE))')
        genout.writeLine('LDFLAGS         += $(LDFLAGS-$(PROFILE))\n')

        genout.writeLine('all: prep \\\n        ' + genAll())
        genout.writeLine('.PHONY: prep\n\nprep:')
        genout.writeLine('\t@if [ "$(CONFIG)" = "" ] ; then echo WARNING: CONFIG not set ; exit 255 ; fi')
        genout.writeLine('\t@[ ! -x $(CONFIG)/inc ] && ' + 
            'mkdir -p $(CONFIG)/inc $(CONFIG)/obj $(CONFIG)/lib $(CONFIG)/bin ; true')
        genout.writeLine('\t@[ ! -f $(CONFIG)/inc/bit.h ] && ' + 
            'cp projects/' + bit.settings.product + '-$(OS)-$(PROFILE)-bit.h $(CONFIG)/inc/bit.h ; true')
        genout.writeLine('\t@if ! diff $(CONFIG)/inc/bit.h projects/' + bit.settings.product + 
            '-$(OS)-$(PROFILE)-bit.h >/dev/null ; then\\')
        genout.writeLine('\t\techo cp projects/' + bit.settings.product + 
            '-$(OS)-$(PROFILE)-bit.h $(CONFIG)/inc/bit.h  ; \\')
        genout.writeLine('\t\tcp projects/' + bit.settings.product + '-$(OS)-$(PROFILE)-bit.h $(CONFIG)/inc/bit.h  ; \\')
        genout.writeLine('\tfi; true\n')
        genout.writeLine('clean:')
        action('cleanTargets')
        genout.writeLine('\nclobber: clean\n\trm -fr ./$(CONFIG)\n')
        build()
        genout.close()
    }

    function generateNmake(base: Path) {
        trace('Generate', 'project file: ' + base.relative + '.nmake')
        let path = base.joinExt('nmake')
        genout = TextStream(File(path, 'w'))
        genout.writeLine('#\n#   ' + path.basename + ' -- Makefile to build ' + bit.settings.title + 
            ' for ' + bit.platform.os + '\n#\n')
        genout.writeLine('PA        = $(PROCESSOR_ARCHITECTURE)')
        genout.writeLine('!IF "$(PA)" == "AMD64"')
            genout.writeLine('ARCH     = x64')
            genout.writeLine('ENTRY    = _DllMainCRTStartup')
        genout.writeLine('!ELSE')
            genout.writeLine('ARCH  = x86')
            genout.writeLine('ENTRY    = _DllMainCRTStartup@12')
        genout.writeLine('!ENDIF\n')
        genout.writeLine('OS       = ' + bit.platform.os)
        genout.writeLine('PROFILE  = ' + bit.platform.profile)
        genout.writeLine('CONFIG   = $(OS)-$(ARCH)-$(PROFILE)')
        genout.writeLine('CC       = cl')
        genout.writeLine('LD       = link')
        genout.writeLine('RC       = rc')
        genout.writeLine('CFLAGS   = ' + gen.compiler)
        genout.writeLine('DFLAGS   = ' + gen.defines)
        genout.writeLine('IFLAGS   = ' + 
            repvar(bit.defaults.includes.map(function(path) '-I' + reppath(path)).join(' ')))
        genout.writeLine('LDFLAGS  = ' + repvar(gen.linker).replace(/-machine:x86/, '-machine:$$(ARCH)'))
        genout.writeLine('LIBPATHS = ' + repvar(gen.libpaths).replace(/\//g, '\\'))
        genout.writeLine('LIBS     = ' + gen.libraries + '\n')
        genout.writeLine('all: prep \\\n        ' + genAll())
        genout.writeLine('.PHONY: prep\n\nprep:')
        genout.writeLine('!IF "$(VSINSTALLDIR)" == ""\n\techo "Visual Studio vars not set. Run vcvars.bat."\n\texit 255\n!ENDIF')
        genout.writeLine('\t@if not exist $(CONFIG)\\inc md $(CONFIG)\\inc')
        genout.writeLine('\t@if not exist $(CONFIG)\\obj md $(CONFIG)\\obj')
        genout.writeLine('\t@if not exist $(CONFIG)\\bin md $(CONFIG)\\bin')
        genout.writeLine('\t@if not exist $(CONFIG)\\inc\\bit.h ' +
            'copy projects\\' + bit.settings.product + '-$(OS)-$(PROFILE)-bit.h $(CONFIG)\\inc\\bit.h')
/* UNUSED
        genout.writeLine('\t@if not exist $(CONFIG)\\bin\\*.def ' +
            'xcopy /Y /S projects\\' + bit.settings.product + '-windows\\*.def $(CONFIG)\\bin\n')
 */
        genout.writeLine('clean:')
        action('cleanTargets')
        genout.writeLine('')
        build()
        genout.close()
    }

    function generateVstudio(base: Path) {
        trace('Generate', 'project file: ' + base.relative)
        mkdir(base)
        global.load(bit.dir.bits.join('vstudio.es'))
        vstudio(base)
    }

    function generateXcode(base: Path) {
        global.load(bit.dir.bits.join('xcode.es'))
        xcode(base)
    }

    function genEnv() {
        let found
        if (bit.platform.os == 'windows') {
            var winsdk = (bit.packs.winsdk && bit.packs.winsdk.path) ? 
                bit.packs.winsdk.path.windows.name.replace(/.*Program Files.*Microsoft/, '$$(PROGRAMFILES)\\Microsoft') :
                '$(PROGRAMFILES)\\Microsoft SDKs\\Windows\\v6.1'
            var vs = (bit.packs.compiler && bit.packs.compiler.dir) ? 
                bit.packs.compiler.dir.windows.name.replace(/.*Program Files.*Microsoft/, '$$(PROGRAMFILES)\\Microsoft') :
                '$(PROGRAMFILES)\\Microsoft Visual Studio 9.0'
            if (generating == 'make') {
                genout.writeLine('VS             := ' + '$(VSINSTALLDIR)')
                genout.writeLine('VS             ?= ' + vs)
                genout.writeLine('SDK            := ' + '$(WindowsSDKDir)')
                genout.writeLine('SDK            ?= ' + winsdk)
                genout.writeLine('\nexport         SDK VS')
            }
        }
        for (let [key,value] in bit.env) {
            if (bit.platform.os == 'windows') {
                value = value.map(function(item)
                    item.replace(bit.packs.compiler.dir, '$(VS)').replace(bit.packs.winsdk.path, '$(SDK)')
                )
            }
            if (value is Array) {
                value = value.join(App.SearchSeparator)
            }
            if (bit.platform.os == 'windows') {
                if (key == 'INCLUDE' || key == 'LIB') {
                    value = '$(' + key + ');' + value
                } else if (key == 'PATH') {
                    value = value + ';$(' + key + ')'
                } 
            }
            if (generating == 'make') {
                genout.writeLine('export %-7s := %s' % [key, value])

            } else if (generating == 'nmake') {
                value = value.replace(/\//g, '\\')
                genout.writeLine('%-9s = %s' % [key, value])

            } else if (generating == 'sh') {
                genout.writeLine('export ' + key + '="' + value + '"')
            }
            found = true
        }
        if (found) {
            genout.writeLine('')
        }
    }

    function genAll() {
        let all = []
        for each (tname in selectedTargets) {
            let target = bit.targets[tname]
            if (target.path && target.enable && !target.nogen) {
                all.push(reppath(target.path))
            }
        }
        return all.join(' \\\n        ') + '\n'
    }

    function import() {
        if (Path('bits').exists) {
            throw 'Current directory already contains a bits directory'
        }
        cp(Config.Bin.join('bits'), 'bits')
        if (!MAIN.exits) {
            mv('bits/sample.bit', MAIN)
        }
        print('Initialization complete.')
        print('Edit ' + MAIN + ' and run "bit configure" to prepare for building.')
        print('Then run "bit" to build.')
    }

    function prepBuild() {
        vtrace('Prepare', 'For building')
        if (bit.platforms.length > 1 || bit.cross) {
            trace('Build', bit.platform.configuration)
            vtrace('Targets', bit.platform.configuration + ': ' + 
                   ((selectedTargets != '') ? selectedTargets: 'nothing to do'))
        }
        /* 
            When cross generating, certain wild cards can't be resolved.
            Setting missing to empty will cause missing glob patterns to be replaced with the pattern itself 
         */
        if (options.gen || options.configure) {
            missing = ''
        }
        makeConstGlobals()
        makeDirGlobals()
        enableTargets()
        blendDefaults()
        resolveDependencies()
        expandWildcards()
        selectTargets()
        castTargetTypes()
        setDefaultTargetPath()
        inlineStatic()
        Object.sortProperties(bit)

        if (options.dump) {
            let o = bit.clone()
            delete o.blend
            let path = Path(currentPlatform + '.dmp')
            path.write(serialize(o, {pretty: true, commas: true, indent: 4, quotes: false}))
            trace('Dump', 'Combined Bit files to: ' + path)
        }
    }

    /*
        Determine which targets are enabled for building on this platform
     */
    function enableTargets() {
        for (let [tname, target] in bit.targets) {
            if (target.enable) {
                if (!(target.enable is Boolean)) {
                    let script = expand(target.enable)
                    try {
                        if (!eval(script)) {
                            vtrace('Skip', 'Target ' + tname + ' is disabled on this platform') 
                            target.enable = false
                        } else {
                            target.enable = true
                        }
                    } catch (e) {
                        vtrace('Enable', 'Can\'t run enable script for ' + target.name + '\n' + e)
                        target.enable = false
                    }
                }
                target.name ||= tname
            } else if (target.enable == undefined) {
                target.enable = true
            }
            if (target.platforms) {
                if (!target.platforms.contains(currentPlatform) &&
                    !(currentPlatform == localPlatform && target.platforms.contains('local')) &&
                    !(currentPlatform != localPlatform && target.platforms.contains('cross'))) {
                        target.enable = false
                }
            }
        }
    }

    /*
        Select the targets to build 
     */
    function selectTargets() {
        originalTargets ||= []
        selectedTargets = originalTargets
        defaultTargets = []
        for (let [tname,target] in bit.targets) {
            if (targetsToBuildByDefault[target.type]) {
                defaultTargets.push(tname)
            }
        }
        if (selectedTargets.length == 0) {
            /* No targets specified, so do a default 'build' */
            selectedTargets = defaultTargets

        } else {
            /* Targets specified. If 'build' is one of the targets|actions, expand it to explicit target names */
            let index = selectedTargets.indexOf('build')
            if (index < 0) {
                index = selectedTargets.indexOf('rebuild')
            }
            if (index >= 0) {
                let names = []
                for (let [tname,target] in bit.targets) {
                    if (targetsToBuildByDefault[target.type]) {
                        names.push(tname)
                    }
                }
                selectedTargets.splice(index, 1, ...names)
            }
        }
        for (let [index, name] in selectedTargets) {
            /* Select target by target type */
            let add = []
            for each (t in bit.targets) {
                if (t.type == name) {
                    if (!selectedTargets.contains(t.name)) {
                        add.push(t.name)
                    }
                    break
                }
            }
            if (!bit.targets[name] && add.length == 0) {
                for each (target in bit.targets) {
                    if (target.name.endsWith(name) || Path(target.name).trimExt().endsWith(name)) {
                        add.push(target.name)
                    }
                }
                if (add.length == 0) {
                    throw 'Unknown target ' + name
                }
            }
            selectedTargets += add
        }
        if (selectedTargets[0] == 'version') {
            print(bit.settings.version + '-' + bit.settings.buildNumber)
            App.exit()
        }
        vtrace('Targets', selectedTargets)
    }

    /*
        Set target output paths. Uses the default locations for libraries, executables and files
     */
    function setDefaultTargetPath() {
        for each (target in bit.targets) {
            if (!target.path) {
                if (target.type == 'lib') {
                    if (target.static) {
                        target.path = bit.dir.lib.join(target.name).joinExt(bit.ext.lib, true)
                    } else {
                        target.path = bit.dir.lib.join(target.name).joinExt(bit.ext.shobj, true)
                    }
                } else if (target.type == 'obj') {
                    target.path = bit.dir.obj.join(target.name).joinExt(bit.ext.o, true)
                } else if (target.type == 'exe') {
                    target.path = bit.dir.bin.join(target.name).joinExt(bit.ext.exe, true)
                } else if (target.type == 'file') {
                    target.path = bit.dir.lib.join(target.name)
                } else if (target.type == 'res') {
                    target.path = bit.dir.res.join(target.name).joinExt(bit.ext.res, true)
                } else if (target.type == 'build') {
                    target.path = target.name
                }
            }
            if (target.path) {
                target.path = Path(expand(target.path, {fill: '${}'}))
            }
        }
    }

    function getDepends(target): Array {
        let libs = []
        for each (dname in target.depends) {
            let dep = bit.targets[dname]
            if (dep && dep.type == 'lib' && dep.enable) {
                libs += getDepends(dep)
                libs.push(dname)
            }
        }
        return libs
    }

    /*
        Implement static linking by inlining all libraries
     */
    function inlineStatic() {
        for each (target in bit.targets) {
            if (target.static) {
                let resolved = []
                let includes = []
                let defines = []
                if (target.type == 'exe') {
                    for each (dname in getDepends(target).unique()) {
                        let dep = bit.targets[dname]
                        if (dep && dep.type == 'lib' && dep.enable) {
                            /* Add the dependent files to the target executables */
                            target.files += dep.files
                            includes += dep.includes
                            defines += dep.defines
                            if (dep.static) {
                                resolved.push(Path(dname).joinExt(bit.ext.lib, true))
                            } else if (dname.startsWith('lib')) {
                                resolved.push(dname.replace(/^lib/g, ''))
                            } else {
                                resolved.push(Path(dname).joinExt(bit.ext.shlib, true))
                            }
                        }
                    }
                }
                target.libraries -= resolved
                target.includes += includes
                target.defines += defines
                target.includes = target.includes.unique()
                target.defines = target.defines.unique()
            }
        }
    }

    /*
        Build a file list and apply include/exclude filters
     */
    function buildFileList(include, exclude = null) {
        let files
        if (include is RegExp) {
            files = Path(bit.dir.src).files('*', {include: include, missing: missing})
        } else {
            if (!(include is Array)) {
                include = [ include ]
            }
            files = []
            for each (pattern in include) {
                pattern = expand(pattern)
                /* If missing, use pattern */
                files += Path('.').files(pattern, {missing: ''})
            }
        }
        if (exclude) {
            if (exclude is RegExp) {
                files = files.reject(function (elt) elt.match(exclude)) 
            } else if (exclude is Array) {
                for each (pattern in exclude) {
                    files = files.reject(function (elt) { return elt.match(pattern); } ) 
                }
            } else {
                files = files.reject(function (elt) elt.match(exclude))
            }
        }
        return files
    }

    /*
        Resolve a target by inheriting dependent libraries
     */
    function resolve(target) {
        if (target.resolved) {
            return
        }
        runTargetScript(target, 'preresolve')
        target.resolved = true
        for each (dname in target.depends) {
            let dep = bit.targets[dname]
            if (dep) {
                if (!dep.enable) continue
                if (!dep.resolved) {
                    resolve(dep)
                }
                if (dep.type == 'lib') {
                    target.libraries
                    target.libraries ||= []
                    /* Put dependent libraries first so system libraries are last (matters on linux) */
                    if (dep.static) {
                        target.libraries = [Path(dname).joinExt(bit.ext.lib)] + target.libraries
                    } else {
                        if (dname.startsWith('lib')) {
                            target.libraries = [dname.replace(/^lib/, '')] + target.libraries
                        } else {
                            target.libraries = [Path(dname).joinExt(bit.ext.shlib, true)] + target.libraries
                        }
                    }
                    for each (lib in dep.libraries) {
                        if (!target.libraries.contains(lib)) {
                            target.libraries.push(lib)
                        }
                    }
                    for each (option in dep.linker) {
                        target.linker ||= []
                        if (!target.linker.contains(option)) {
                            target.linker.push(option)
                        }
                    }
                    for each (option in dep.libpaths) {
                        target.libpaths ||= []
                        if (!target.libpaths.contains(option)) {
                            target.libpaths.push(option)
                        }
                    }
                }
            } else {
                let pack = bit.packs[dname]
                if (pack) {
                    if (!pack.enable) continue
                    if (pack.includes) {
                        target.includes ||= []
                        target.includes += pack.includes
                    }
                    if (pack.defines) {
                        target.defines ||= []
                        target.defines += pack.defines
                    }
                    if (pack.libraries) {
                        target.libraries ||= []
                        target.libraries += pack.libraries
                    }
                    if (pack.linker) {
                        target.linker ||= []
                        target.linker += pack.linker
                    }
                    if (pack.libpaths) {
                        target.libpaths ||= []
                        target.libpaths += pack.libpaths
                    }
                }
            }
        }
        runTargetScript(target, 'postresolve')
    }

    function resolveDependencies() {
        for each (target in bit.targets) {
            resolve(target)
        }
        for each (target in bit.targets) {
            delete target.resolved
        }
    }

    /*
        Expand resources, sources and headers. Support include+exclude and create target.files[]
     */
    function expandWildcards() {
        let index
        for each (target in bit.targets) {
            runTargetScript(target, 'presource')
            if (target.files) {
                target.files = buildFileList(target.files)
            }
            if (target.headers) {
                /*
                    Create a target for each header file
                 */
                target.files ||= []
                let files = buildFileList(target.headers, target.exclude)
                for each (file in files) {
                    let header = bit.dir.inc.join(file.basename)
                    /* Always overwrite dynamically created targets created via makeDepends */
                    bit.targets[header] = { name: header, enable: true, path: header, type: 'header', files: [ file ],
                        vars: {} }
                    target.depends ||= []
                    target.depends.push(header)
                }
            }
            if (target.resources) {
                target.files ||= []
                let files = buildFileList(target.resources, target.exclude)
                for each (file in files) {
                    /*
                        Create a target for each resource file
                     */
                    let res = bit.dir.obj.join(file.replaceExt(bit.ext.res).basename)
                    let resTarget = { name : res, enable: true, path: res, type: 'resource', files: [ file ], 
                        includes: target.includes, vars: {} }
                    if (bit.targets[res]) {
                        resTarget = blend(bit.targets[resTarget.name], resTarget, {combined: true})
                    }
                    bit.targets[resTarget.name] = resTarget
                    target.files.push(res)
                    target.depends ||= []
                    target.depends.push(res)
                }
            }
            if (target.sources) {
                target.files ||= []
                let files = buildFileList(target.sources, target.exclude)
                for each (file in files) {
                    /*
                        Create a target for each source file
                     */
                    let obj = bit.dir.obj.join(file.replaceExt(bit.ext.o).basename)
                    let precompile = (target.scripts && target.scripts.precompile) ?
                        target.scripts.precompile : null
                    let objTarget = { name : obj, enable: true, path: obj, type: 'obj', files: [ file ], 
                        compiler: target.compiler, defines: target.defines, includes: target.includes,
                        scripts: { precompile: precompile }, vars: {}}
                    if (bit.targets[obj]) {
                        objTarget = blend(bit.targets[objTarget.name], objTarget, {combined: true})
                    }
                    bit.targets[objTarget.name] = objTarget
                    target.files.push(obj)
                    target.depends ||= []
                    target.depends.push(obj)

                    /*
                        Create targets for each header (if not already present)
                     */
                    objTarget.depends = makeDepends(objTarget)
                    for each (header in objTarget.depends) {
                        if (!bit.targets[header]) {
                            bit.targets[header] = { name: header, enable: true, path: header, 
                                type: 'header', files: [ header ], vars: {} }
                        }
                    }
                }
            }
        }
    }

    /*
        Blend bit.defaults into targets
     */
    function blendDefaults() {
        if (bit.defaults) {
            runTargetScript({scripts: bit.defaults.scripts}, 'preblend')
        }
        for (let [tname, target] in bit.targets) {
            if (targetsToBlend[target.type]) {
                let def = blend({}, bit.defaults, {combine: true})
                target = bit.targets[tname] = blend(def, target, {combine: true})
                runTargetScript(target, 'postblend')
                if (target.scripts && target.scripts.preblend) {
                    delete target.scripts.preblend
                }
                if (target.type == 'obj') { 
                    delete target.linker 
                    delete target.libpaths 
                    delete target.libraries 
                }
            }
            if (target.type == 'lib' && target.static == null) {
                target.static = bit.settings.static
            }
        }
    }

    function castDirTypes() {
        for (let [key,value] in bit.blend) {
            bit.blend[key] = Path(value).absolute.portable
        }
        for (let [key,value] in bit.dir) {
            bit.dir[key] = Path(value).absolute
        }
        let defaults = bit.defaults
        if (defaults) {
            for (let [key,value] in defaults.includes) {
                defaults.includes[key] = Path(value).absolute
            }
            for (let [key,value] in defaults.libpaths) {
                defaults.libpaths[key] = Path(value).absolute
            }
        }
        for (let [pname, prefix] in bit.prefixes) {
            bit.prefixes[pname] = Path(prefix)
            if (bit.platform.os == 'windows' && Config.OS == 'windows') {
                bit.prefixes[pname] = bit.prefixes[pname].absolute
            }
        }
        for each (pack in bit.packs) {
            if (pack.dir) {
                pack.dir = Path(pack.dir).absolute
            }
            if (pack.path) {
                /* Must not make absolute incase pack resolves using PATH at run-time. e.g. cc */
                pack.path = Path(pack.path)
            }
            for (let [key,value] in pack.includes) {
                pack.includes[key] = Path(value).absolute
            }
            for (let [key,value] in pack.libpaths) {
                pack.libpaths[key] = Path(value).absolute
            }
        }
    }

    function castTargetTypes() {
        for each (target in bit.targets) {
            if (target.path) {
                target.path = Path(target.path)
            }
            for (i in target.includes) {
                target.includes[i] = Path(target.includes[i])
            }
            for (i in target.libpaths) {
                target.libpaths[i] = Path(target.libpaths[i])
            }
            for (i in target.headers) {
                target.headers[i] = Path(target.headers[i])
            }
            for (i in target.files) {
                target.files[i] = Path(target.files[i])
            }
            for (i in target.resources) {
                target.resources[i] = Path(target.resources[i])
            }
        }
    }

    /*
        Build all selected targets
     */
    function build() {
        for each (name in selectedTargets) {
            /* Build named targets */
            let target = bit.targets[name]
            if (target && target.enable) {
                buildTarget(target)
            }
            if (name == 'generate') break

            /* Build targets with the same type as the action */
            for each (t in bit.targets) {
                if (t.type == name) {
                    if (t.enable) {
                        buildTarget(t)
                    }
                }
            }
        }
    }

    /*
        Build a target and all required dependencies (first)
     */
    function buildTarget(target) {
        if (target.built || !target.enable) {
            return
        }
        if (target.building) {
            throw 'Possible recursive dependancy: target ' + target.name + ' is already building'
        }
        target.building = true
        target.linker ||= []
        target.libpaths ||= []
        target.includes ||= []
        target.libraries ||= []

        runTargetScript(target, 'predependencies')
        for each (dname in target.depends) {
            let dep = bit.targets[dname]
            if (!dep) {
                if (dname == 'build') {
                    for each (tname in defaultTargets) {
                        buildTarget(bit.targets[tname])
                    }
                } else if (!Path(dname).exists) {
                    if (!bit.packs[dname]) {
                        print('Unknown dependency "' + dname + '" in target "' + target.name + '"')
                        return
                    }
                }
            } else {
                if (!dep.enable || dep.built) {
                    continue
                }
                if (dep.building) {
                    throw new Error('Possible recursive dependancy in target ' + target.name + 
                        ', dependancy ' + dep.name + ' is already building.')
                }
                buildTarget(dep)
            }
        }
        if (target.message) {
            trace('Info', target.message)
        }
        try {
            if (target.type == 'lib') {
                if (target.static) {
                    buildStaticLib(target)
                } else {
                    buildSharedLib(target)
                }
            } else if (target.type == 'exe') {
                buildExe(target)
            } else if (target.type == 'obj') {
                buildObj(target)
            } else if (target.type == 'file') {
                buildFile(target)
            } else if (target.type == 'header') {
                buildFile(target)
            } else if (target.type == 'resource') {
                buildResource(target)
            } else if (target.type == 'build' || (target.scripts && target.scripts['build'])) {
                buildScript(target)
            }
        } catch (e) {
            throw new Error('Building target ' + target.name + '\n' + e)
        }
        target.building = false
        target.built = true
    }

    /*
        Build an executable program
     */
    function buildExe(target) {
        if (!stale(target)) {
            whySkip(target.path, 'is up to date')
            return
        }
        if (options.diagnose) {
            App.log.debug(3, "Target => " + serialize(target, {pretty: true, commas: true, indent: 4, quotes: false}))
        }
        runTargetScript(target, 'prebuild')

        let transition = target.rule || 'exe'
        let rule = bit.rules[transition]
        if (!rule) {
            throw 'No rule to build target ' + target.path + ' for transition ' + transition
            return
        }
        let command = expandRule(target, rule)
        if (generating == 'sh') {
            command = repcmd(command)
            genout.writeLine(command + '\n')

        } else if (generating == 'make') {
            command = repcmd(command)
            genout.writeLine(reppath(target.path) + ': ' + repvar(getTargetDeps(target)) + '\n\t' + command + '\n')

        } else if (generating == 'nmake') {
            command = repcmd(command)
            genout.writeLine(reppath(target.path) + ': ' + repvar(getTargetDeps(target)) + '\n\t' + command + '\n')

        } else {
            trace('Link', target.name)
            if (target.active && bit.platform.like == 'windows') {
                let old = target.path.relative.replaceExt('old')
                trace('Preserve', 'Active target ' + target.path.relative + ' as ' + old)
                old.remove()
                target.path.rename(old)
            } else {
                safeRemove(target.path)
            }
            run(command, {excludeOutput: /Creating library /})
        }
    }

    function buildSharedLib(target) {
        if (!stale(target)) {
            whySkip(target.path, 'is up to date')
            return
        }
        if (options.diagnose) {
            App.log.debug(3, "Target => " + serialize(target, {pretty: true, commas: true, indent: 4, quotes: false}))
        }
        runTargetScript(target, 'prebuild')
        //UNUSED buildSym(target)
        let transition = target.rule || 'shlib'
        let rule = bit.rules[transition]
        if (!rule) {
            throw 'No rule to build target ' + target.path + ' for transition ' + transition
            return
        }
        let command = expandRule(target, rule)
        if (generating == 'sh') {
            command = repcmd(command)
            genout.writeLine(command + '\n')

        } else if (generating == 'make') {
            command = repcmd(command)
            genout.writeLine(reppath(target.path) + ': ' + repvar(getTargetDeps(target)) + '\n\t' + command + '\n')

        } else if (generating == 'nmake') {
            command = repcmd(command)
            genout.writeLine(reppath(target.path) + ': ' + repvar(getTargetDeps(target)) + '\n\t' + command + '\n')

        } else {
            trace('Link', target.name)
            if (target.active && bit.platform.like == 'windows') {
                let active = target.path.relative.replaceExt('old')
                trace('Preserve', 'Active target ' + target.path.relative + ' as ' + active)
                active.remove()
                target.path.rename(target.path.replaceExt('old'))
            } else {
                safeRemove(target.path)
            }
            run(command, {excludeOutput: /Creating library /})
        }
    }

    function buildStaticLib(target) {
        if (!stale(target)) {
            whySkip(target.path, 'is up to date')
            return
        }
        if (options.diagnose) {
            App.log.debug(3, "Target => " + serialize(target, {pretty: true, commas: true, indent: 4, quotes: false}))
        }
        runTargetScript(target, 'prebuild')
        let transition = target.rule || 'lib'
        let rule = bit.rules[transition]
        if (!rule) {
            throw 'No rule to build target ' + target.path + ' for transition ' + transition
            return
        }
        let command = expandRule(target, rule)
        if (generating == 'sh') {
            command = repcmd(command)
            genout.writeLine(command + '\n')

        } else if (generating == 'make') {
            command = repcmd(command)
            genout.writeLine(reppath(target.path) + ': ' + repvar(getTargetDeps(target)) + '\n\t' + command + '\n')

        } else if (generating == 'nmake') {
            command = repcmd(command)
            genout.writeLine(reppath(target.path) + ': ' + repvar(getTargetDeps(target)) + '\n\t' + command + '\n')

        } else {
            trace('Link', target.name)
            if (target.active && bit.platform.like == 'windows') {
                let active = target.path.relative.replaceExt('old')
                trace('Preserve', 'Active target ' + target.path.relative + ' as ' + active)
                active.remove()
                target.path.rename(target.path.replaceExt('old'))
            } else {
                safeRemove(target.path)
            }
            run(command, {excludeOutput: /has no symbols|Creating library /})
        }
    }

    /*
        Build symbols file for windows libraries
     */
    function buildSym(target) {
        let rule = bit.rules['sym']
        if (!rule || generating) {
            return
        }
        target.vars.IN = target.files.join(' ')
        let command = expandRule(target, rule)
        let data = run(command, {noshow: true})
        let result = []
        let lines = data.match(/SECT.*External *\| .*/gm)
        for each (l in lines) {
            if (l.contains('__real')) continue
            if (l.contains('??')) continue
            let sym
            if (bit.platform.arch == 'x64') {
                /* Win64 does not have "_" */
                sym = l.replace(/.*\| */, '').replace(/\r$/,'')
            } else {
                sym = l.replace(/.*\| _/, '').replace(/\r$/,'')
            }
            if (sym == 'MemoryBarrier' || sym.contains('_mask@@NegDouble@')) continue
            result.push(sym)
        }
        let def = Path(target.path.toString().replace(/dll$/, 'def'))
        def.write('LIBRARY ' + target.path.basename + '\nEXPORTS\n  ' + result.sort().join('\n  ') + '\n')
    }

    /*
        Build an object from source
     */
    function buildObj(target) {
        if (!stale(target)) {
            return
        }
        if (options.diagnose) {
            App.log.debug(3, "Target => " + serialize(target, {pretty: true, commas: true, indent: 4, quotes: false}))
        }
        runTargetScript(target, 'precompile')

        let ext = target.path.extension
        for each (file in target.files) {
            target.vars.IN = file.relative
            let transition = file.extension + '->' + target.path.extension
            if (options.pre) {
                transition = 'c->c'
            }
            let rule = target.rule || bit.rules[transition]
            if (!rule) {
                rule = bit.rules[target.path.extension]
                if (!rule) {
                    throw 'No rule to build target ' + target.path + ' for transition ' + transition
                    return
                }
            }
            let command = expandRule(target, rule)
            if (generating == 'sh') {
                command = repcmd(command)
                genout.writeLine(command + '\n')

            } else if (generating == 'make') {
                command = repcmd(command)
                genout.writeLine(reppath(target.path) + ': \\\n        ' + 
                    file.relative + repvar(getTargetDeps(target)) + '\n\t' + command + '\n')

            } else if (generating == 'nmake') {
                command = repcmd(command)
                genout.writeLine(reppath(target.path) + ': \\\n        ' + 
                    file.relative.windows + repvar(getTargetDeps(target)) + '\n\t' + command + '\n')

            } else {
                trace('Compile', file.relativeTo('.'))
                if (bit.platform.os == 'windows') {
                    run(command, {excludeOutput: /^[a-zA-Z0-9-]*.c\s*$/})
                } else {
                    run(command)
                }
            }
        }
    }

    function buildResource(target) {
        if (!stale(target)) {
            return
        }
        if (options.diagnose) {
            App.log.debug(3, "Target => " + serialize(target, {pretty: true, commas: true, indent: 4, quotes: false}))
        }
        runTargetScript(target, 'prebuild')

        let ext = target.path.extension
        for each (file in target.files) {
            target.vars.IN = file.relative
            let transition = file.extension + '->' + target.path.extension
            let rule = target.rule || bit.rules[transition]
            if (!rule) {
                rule = bit.rules[target.path.extension]
                if (!rule) {
                    throw 'No rule to build target ' + target.path + ' for transition ' + transition
                    return
                }
            }
            let command = expandRule(target, rule)
            if (generating == 'sh') {
                command = repcmd(command)
                genout.writeLine(command + '\n')

            } else if (generating == 'make') {
                command = repcmd(command)
                genout.writeLine(reppath(target.path) + ': \\\n        ' + 
                    file.relative + repvar(getTargetDeps(target)) + '\n\t' + command + '\n')

            } else if (generating == 'nmake') {
                command = repcmd(command)
                genout.writeLine(reppath(target.path) + ': \\\n        ' + 
                    file.relative.windows + repvar(getTargetDeps(target)) + '\n\t' + command + '\n')

            } else {
                trace('Compile', file.relativeTo('.'))
                run(command)
            }
        }
    }

    /*
        Copy files[] to path
     */
    function buildFile(target) {
        if (!stale(target)) {
            whySkip(target.path, 'is up to date')
            return
        }
        runTargetScript(target, 'prebuild')

        for each (let file: Path in target.files) {
            /* Auto-generated headers targets for includes have file == target.path */
            if (file == target.path) {
                continue
            }
            target.vars.IN = file.relative
            if (generating == 'sh') {
                genout.writeLine('rm -rf ' + reppath(target.path.relative))
                genout.writeLine('cp -r ' + reppath(file.relative) + ' ' + reppath(target.path.relative) + '\n')

            } else if (generating == 'make') {
                genout.writeLine(reppath(target.path) + ': ' + repvar(getTargetDeps(target)))
                genout.writeLine('\trm -fr ' + reppath(target.path.relative))
                genout.writeLine('\tcp -r ' + reppath(file.relative) + ' ' + reppath(target.path.relative) + '\n')

            } else if (generating == 'nmake') {
                genout.writeLine(reppath(target.path) + ': ' + repvar(getTargetDeps(target)))
                genout.writeLine('\t-if exist ' + reppath(target.path) + ' del /Q ' + reppath(target.path))
                if (file.isDir) {
                    genout.writeLine('\tif not exist ' + reppath(target.path) + ' md ' + reppath(target.path))
                    genout.writeLine('\txcopy /S /Y ' + reppath(file) + ' ' + reppath(target.path) + '\n')
                } else {
                    genout.writeLine('\tcopy /Y ' + reppath(file) + ' ' + reppath(target.path) + '\n')
                }

            } else {
                trace('Copy', target.path.portable.relativeTo('.'))
                if (target.active && bit.platform.like == 'windows') {
                    let active = target.path.relative.replaceExt('old')
                    trace('Preserve', 'Active target ' + target.path.relative + ' as ' + active)
                    active.remove()
                    target.path.rename(target.path.replaceExt('old'))
                } else {
                    safeRemove(target.path)
                }
                cp(file, target.path)
            }
        }
    }

    function buildScript(target) {
        if (!stale(target)) {
            whySkip(target.path, 'is up to date')
            return
        }
        if (options.diagnose) {
            App.log.debug(3, "Target => " + serialize(target, {pretty: true, commas: true, indent: 4, quotes: false}))
        }
        runTargetScript(target, 'prebuild')
        setRuleVars(target, target.home)

        let prefix, suffix
        if (generating == 'sh' || generating == 'make') {
            prefix = 'cd ' + target.home.relative + ' >/dev/null\n'
            suffix = '\ncd - >/dev/null'
        } else if (generating == 'nmake') {
            prefix = 'cd ' + target.home.relative.windows + '\n'
            suffix = '\ncd ' + bit.dir.src.relativeTo(target.home).windows
        } else {
            prefix = suffix = ''
        }
        if (generating == 'sh') {
            let cmd = target['generate-sh'] || target.shell
            if (cmd) {
                cmd = (prefix + cmd.trim() + suffix).replace(/^[ \t]*/mg, '')
                cmd = cmd.replace(/$/mg, ';\\').replace(/;\\;\\/g, ' ;\\').trim(';\\')
                cmd = expand(cmd, {fill: null}).expand(target.vars, {fill: ''})
                cmd = repvar2(cmd, target.home)
                genWrite(cmd + '\n')
            } else {
                genout.writeLine('#  Omit build script ' + target.path)
            }

        } else if (generating == 'make') {
            genWrite(target.path.relative + ': ' + getTargetDeps(target))
            let cmd = target['generate-make'] || target['generate-sh'] || target.generate
            if (cmd) {
                cmd = (prefix + cmd.trim() + suffix).replace(/^[ \t]*/mg, '\t')
                cmd = cmd.replace(/$/mg, ';\\').replace(/;\\;\\/g, ' ;\\').trim(';\\')
                cmd = expand(cmd, {fill: null}).expand(target.vars, {fill: ''})
                cmd = repvar2(cmd, target.home)
                genWrite(cmd + '\n')
            }

        } else if (generating == 'nmake') {
            genWrite(target.path.relative.windows + ': ' + getTargetDeps(target))
            let cmd = target['generate-nmake'] || target['generate-make'] || target['generate']
            if (cmd) {
                cmd = cmd.trim().replace(/^cp /, 'copy ')
                cmd = (prefix + cmd + suffix).replace(/^[ \t]*/mg, '\t')
                    let saveDir = []
                if (bit.platform.os == 'windows') {
                    for (n in bit.globals) {
                        if (bit.globals[n] is Path) {
                            saveDir[n] = bit.globals[n]
                            bit[n] = bit.globals[n].windows
                        }
                    }
                }
                try {
                    cmd = expand(cmd, {fill: null}).expand(target.vars, {fill: ''})
                } catch (e) {
                    print('Target', target.name)
                    print('Script:', cmd)
                    throw e
                }
                if (bit.platform.os == 'windows') {
                    for (n in saveDir) {
                        bit.globals[n] = saveDir[n]
                    }
                }
                cmd = repvar2(cmd, target.home)
                genWrite(cmd + '\n')
            } else {
                genout.writeLine('#  Omit build script ' + target.path + '\n')
            }

        } else if (target.scripts) {
            vtrace(target.type.toPascal(), target.name)
            runTargetScript(target, 'build')
        }
    }

    /*
        Replace default defines, includes, libraries etc with token equivalents. This allows
        Makefiles and script to be use variables to control various flag settings.
     */
    function repcmd(command: String): String {
        if (generating == 'make' || generating == 'nmake') {
            /* Twice because ldflags are repeated and replace only changes the first occurrence */
            command = command.replace(gen.linker, '$(LDFLAGS)')
            command = command.replace(gen.linker, '$(LDFLAGS)')
            command = command.replace(gen.libpaths, '$(LIBPATHS)')
            command = command.replace(gen.compiler, '$(CFLAGS)')
            command = command.replace(gen.defines, '$(DFLAGS)')
            command = command.replace(gen.includes, '$(IFLAGS)')
            command = command.replace(gen.libraries, '$(LIBS)')
            command = command.replace(RegExp(gen.configuration, 'g'), '$$(CONFIG)')
            command = command.replace(bit.packs.compiler.path, '$(CC)')
            command = command.replace(bit.packs.link.path, '$(LD)')
            if (bit.packs.rc) {
                command = command.replace(bit.packs.rc.path, '$(RC)')
            }
            for each (word in minimalCflags) {
                command = command.replace(word, '')
            }

        } else if (generating == 'sh') {
            command = command.replace(gen.linker, '${LDFLAGS}')
            command = command.replace(gen.linker, '${LDFLAGS}')
            command = command.replace(gen.libpaths, '${LIBPATHS}')
            command = command.replace(gen.compiler, '${CFLAGS}')
            command = command.replace(gen.defines, '${DFLAGS}')
            command = command.replace(gen.includes, '${IFLAGS}')
            command = command.replace(gen.libraries, '${LIBS}')
            command = command.replace(RegExp(gen.configuration, 'g'), '$${CONFIG}')
            command = command.replace(bit.packs.compiler.path, '${CC}')
            command = command.replace(bit.packs.link.path, '${LD}')
        }
        if (generating == 'nmake') {
            command = command.replace('_DllMainCRTStartup@12', '$(ENTRY)')
        }
        command = command.replace(RegExp(bit.dir.top + '/', 'g'), '')
        command = command.replace(/  */g, ' ')
        if (generating == 'nmake') {
            command = command.replace(/\//g, '\\')
        }
        return command
    }

    /*
        Replace with variables where possible.
        Replaces the top directory and the CONFIGURATION
     */
    function repvar(command: String): String {
        command = command.replace(RegExp(bit.dir.top + '/', 'g'), '')
        if (generating == 'make') {
            command = command.replace(RegExp(gen.configuration, 'g'), '$$(CONFIG)')
        } else if (generating == 'nmake') {
            command = command.replace(RegExp(gen.configuration, 'g'), '$$(CONFIG)')
        } else if (generating == 'sh') {
            command = command.replace(RegExp(gen.configuration, 'g'), '$${CONFIG}')
        }
        return command
    }

    function repvar2(command: String, home: Path): String {
        command = command.replace(RegExp(bit.dir.top, 'g'), bit.dir.top.relativeTo(home))
        if (bit.platform.like == 'windows' && generating == 'nmake') {
            let re = RegExp(bit.dir.top.windows.name.replace(/\\/g, '\\\\'), 'g')
            command = command.replace(re, bit.dir.top.relativeTo(home).windows)
        }
        if (generating == 'make') {
            command = command.replace(RegExp(gen.configuration, 'g'), '$$(CONFIG)')
        } else if (generating == 'nmake') {
            command = command.replace(RegExp(gen.configuration + '\\\\bin/', 'g'), '$$(CONFIG)\\bin\\')
            command = command.replace(RegExp(gen.configuration, 'g'), '$$(CONFIG)')
        } else if (generating == 'sh') {
            command = command.replace(RegExp(gen.configuration, 'g'), '$${CONFIG}')
        }
        return command
    }

    function reppath(path: Path): String {
        path = path.relative
        if (bit.platform.like == 'windows') {
            path = (generating == 'nmake') ? path.windows : path.portable
        } else if (Config.OS == 'windows' && generating && generating != 'nmake')  {
            path = path.portable 
        }
        return repvar(path)
    }

    /*
        Get the dependencies of a target as a string
     */
    function getTargetDeps(target): String {
        let deps = []
        if (!target.depends || target.depends.length == 0) {
            return ''
        } else {
            for each (let dname in target.depends) {
                let dep = bit.targets[dname]
                if (dep && dep.enable) {
                    deps.push(reppath(dep.path))
                }
            }
            return ' \\\n        ' + deps.join(' \\\n        ')
        }
    }

    /*
        Set top level constant variables. This enables them to be used in token expansion
     */
    public function makeConstGlobals() {
        let g = bit.globals
        g.PLATFORM = bit.platform.name
        g.OS = bit.platform.os
        g.CPU = bit.platform.cpu || 'generic'
        g.ARCH = bit.platform.arch
        /* Apple gcc only */
        if (bit.ccArch) {
            g.CC_ARCH = bit.ccArch[bit.platform.arch] || bit.platform.arch
        }
        g.CONFIG = bit.platform.configuration
        g.EXE = bit.ext.dotexe
        g.LIKE = bit.platform.like
        g.O = bit.ext.doto
        g.SHOBJ = bit.ext.dotshobj
        g.SHLIB = bit.ext.dotshlib
    }

    /*
        Called in this file and in xcode.es during project generation
     */
    public function makeDirGlobals(base: Path? = null) {
        for each (n in ['BIN', 'CFG', 'BITS', 'FLAT', 'INC', 'LIB', 'OBJ', 'PACKS', 'PKG', 'REL', 'SRC', 'TOP']) {
            /* 
                These globals are always in portable format so they can be used in build scripts. Windows back-slashes
                require quoting! 
             */ 
            let dir = bit.dir[n.toLower()]
            if (!dir) continue
            dir = dir.portable
            if (base) {
                dir = dir.relativeTo(base)
            }
            global[n] = bit.globals[n] = dir
        }
        bit.globals.LBIN = localBin
    }

    public function setRuleVars(target, base: Path = App.dir) {
        let tv = target.vars || {}
        if (target.home) {
            tv.HOME = target.home.relativeTo(base)
        }
        if (target.path) {
            tv.OUT = target.path.relativeTo(base)
        }
        if (target.libpaths) {
            tv.LIBPATHS = mapLibPaths(target.libpaths, base)
        }
        if (target.entry) {
            tv.ENTRY = target.entry[target.rule || target.type]
        }
        if (target.type == 'exe') {
            if (!target.files) {
                throw 'Target ' + target.name + ' has no input files or sources'
            }
            tv.IN = target.files.map(function(p) p.relativeTo(base)).join(' ')
            tv.LIBS = mapLibs(target.libraries, target.static)
            tv.LDFLAGS = (target.linker) ? target.linker.join(' ') : ''

        } else if (target.type == 'lib') {
            if (!target.files) {
                throw 'Target ' + target.name + ' has no input files or sources'
            }
            tv.IN = target.files.map(function(p) p.relativeTo(base)).join(' ')
            tv.LIBNAME = target.path.basename
            //  MOB unused
            tv.DEF = Path(target.path.relativeTo(base).toString().replace(/dll$/, 'def'))
            tv.LIBS = mapLibs(target.libraries, target.static)
            tv.LDFLAGS = (target.linker) ? target.linker.join(' ') : ''

        } else if (target.type == 'obj') {
            tv.CFLAGS = (target.compiler) ? target.compiler.join(' ') : ''
            tv.DEFINES = (target.defines) ? target.defines.join(' ') : ''
            tv.INCLUDES = (target.includes) ? target.includes.map(function(p) '-I' + p.relativeTo(base)) : ''
            tv.PDB = tv.OUT.replaceExt('pdb')

        } else if (target.type == 'resource') {
            tv.OUT = target.path.relative
            tv.CFLAGS = (target.compiler) ? target.compiler.join(' ') : ''
            tv.DEFINES = (target.defines) ? target.defines.join(' ') : ''
            tv.INCLUDES = (target.includes) ? target.includes.map(function(path) '-I' + path.relative) : ''
        }
        target.vars = tv
    }

    /*
        Set the PATH and LD_LIBRARY_PATH environment variables
     */
    function setPathEnvVar(bit) {
        let outbin = Path('.').join(bit.platform.configuration, 'bin').absolute
        let sep = App.SearchSeparator
        if (generating) {
            outbin = outbin.relative
        }
        App.putenv('PATH', outbin + sep + App.getenv('PATH'))
        App.log.debug(2, "PATH=" + App.getenv('PATH'))
    }

    /*
        Run an event script in the directory of the bit file
        When values used are: build, postblend, postresolve, presource, prebuild, action
     */
    public function runTargetScript(target, when) {
        if (!target.scripts) return
        for each (item in target.scripts[when]) {
            let pwd = App.dir
            if (item.home && item.home != pwd) {
                App.chdir(item.home)
            }
            global.TARGET = bit.target = target
            try {
                if (item.shell != 'ejs') {
                    runShell(target, item.shell, item.script)
                } else {
                    let script = expand(item.script).expand(target.vars, {fill: ''})
/*
print('ITEM.NS', typeOf(item.ns))
print('ITEM.NS', item.ns)
dump(item)
global.NN = item.ns
*/
                    script = 'require ejs.unix\n' + script
                    eval(script)
                }
            } finally {
                App.chdir(pwd)
                global.TARGET = null
                delete bit.target
            }
        }
    }

    public function runScript(script: String, home: Path) {
        let pwd = App.dir
        if (home && home != pwd) {
            App.chdir(home)
        }
        try {
            script = 'require ejs.unix\n' + expand(script)
            eval(script)
        } finally {
            App.chdir(pwd)
        }
    }

    function setShellEnv(target, script) {
    }

    function runShell(target, shell, script) {
        let lines = script.match(/^.*$/mg).filter(function(l) l.length)
        let command = lines.join(';')
        strace('Run', command)
        let shell = Cmd.locate(shell)
        let cmd = new Cmd
        setShellEnv(target, cmd)
        cmd.start([shell, "-c", command.toString().trimEnd('\n')], {noio: true})
        if (cmd.status != 0 && !options['continue']) {
            throw 'Command failure: ' + command + '\nError: ' + cmd.error
        }
    }

    function mapLibPaths(libpaths: Array, base: Path = App.dir): String {
        if (bit.platform.os == 'windows') {
            return libpaths.map(function(p) '-libpath:' + p.relativeTo(base)).join(' ')
        } else {
            return libpaths.map(function(p) '-L' + p.relativeTo(base)).join(' ')
        }
    }

    /*
        Map libraries into the appropriate O/S dependant format
     */
    public function mapLibs(libs: Array, static = null): Array {
        if (bit.platform.os == 'windows') {
            libs = libs.clone()
            for (let [i,name] in libs) {
                let libname = Path('lib' + name).joinExt(bit.ext.shlib)
                if (bit.targets['lib' + name] || bit.dir.lib.join(libname).exists) {
                    libs[i] = libname
                }
            }
        } else if (bit.platform.os == 'vxworks') {
            libs = libs.clone()
            for (i = 0; i < libs.length; i++) {
                if (libs.contains(libs[i])) {
                    libs.remove(i)
                    i--
                }
            }
            for (i in libs) {
                let llib = bit.dir.lib.join("lib" + libs[i]).joinExt(bit.ext.shlib).relative
                if (llib.exists) {
                    libs[i] = llib
                } else {
                    libs[i] = '-l' + Path(libs[i]).trimExt().toString().replace(/^lib/, '')
                }
            }
        } else {
            let mapped = []
            for each (let lib:Path in libs) {
                /* MOB UNUSED
                if (lib.extension) {
                    mapped.push(bit.dir.lib.relativeTo(App.dir).join(lib))
                } else
                */
                    mapped.push('-l' + lib.trimExt().relative.toString().replace(/^lib/, ''))
            }
            libs = mapped
        }
        return libs
    }

    /*
        Test if a target is stale vs the inputs AND dependencies
     */
    function stale(target) {
        if (target.built) {
            return false
        }
        if (generating) {
            return !target.nogen
        }
        if (options.rebuild) {
            return true
        }
        if (!target.path) {
            return true
        }
        let path = target.path
        if (!path.modified) {
            whyRebuild(target.name, 'Rebuild', target.path + ' is missing.')
            return true
        }
        for each (file in target.files) {
            if (file.modified > path.modified) {
                whyRebuild(path, 'Rebuild', 'input ' + file + ' has been modified.')
                return true
            }
        }
        for each (let dname: Path in target.depends) {
            let file
            if (!bit.targets[dname]) {
                let pack = bit.packs[dname]
                if (pack) {
                    if (!pack.enable) {
                        continue
                    }
                    file = pack.path
                    if (!file) {
                        whyRebuild(path, 'Rebuild', 'missing ' + file + ' for package ' + dname)
                        return true
                    }
                } else {
                    /* If dependency is not a target, then treat as a file */
                    if (!dname.modified) {
                        whyRebuild(path, 'Rebuild', 'missing dependency ' + dname)
                        return true
                    }
                    if (dname.modified > path.modified) {
                        whyRebuild(path, 'Rebuild', 'dependency ' + dname + ' has been modified.')
                        return true
                    }
                    return false
                }
            } else {
                file = bit.targets[dname].path
            }
            if (file.modified > path.modified) {
                whyRebuild(path, 'Rebuild', 'dependent ' + file + ' has been modified.')
                return true
            }
        }
        return false
    }

    /*
        Create an array of dependencies for a target
     */
    function makeDepends(target): Array {
        let includes: Array = []
        for each (path in target.files) {
            let str = path.readString()
            let more = str.match(/^#include.*"$/gm)
            if (more) {
                includes += more
            }
        }
        let depends = [ bit.dir.inc.join('bit.h') ]

        /*
            Resolve includes 
         */
        for each (item in includes) {
            let ifile = item.replace(/#include.*"(.*)"/, '$1')
            let path
            for each (dir in target.includes) {
                path = Path(dir).join(ifile)
                if (path.exists && !path.isDir) {
                    break
                }
                if (options.why) {
                    trace('Warn', 'Can\'t resolve include: ' + path.relative + ' for ' + target.name)
                }
                path = null
            }
            if (path && !depends.contains(path)) {
                depends.push(path)
            }
        }
        return depends
    }

    /*
        Expand tokens in all fields in an object hash. This is used to expand tokens in bit file objects.
     */
    function expandTokens(o) {
        for (let [key,value] in o) {
            if (value is String) {
                o[key] = expand(value, {fill: '${}'})
            } else if (value is Path) {

                o[key] = Path(expand(value, {fill: '${}'}))
            } else if (Object.getOwnPropertyCount(value) > 0) {
                o[key] = expandTokens(value)
            }
        }
        return o
    }

    /*
        Run a command and trace output if cmdOptions.true or options.show
     */
    public function run(command, cmdOptions = {}): String {
        if (options.show || cmdOptions.show) {
            if (command is Array) {
                trace('Run', command.join(' '))
            } else {
                trace('Run', command)
            }
        }
        let cmd = new Cmd
        if (bit.env) {
            let env = App.env.clone()
            for (let [key,value] in bit.env) {
                if (value is Array) {
                    value = value.join(App.SearchSeparator)
                }
                if (bit.platform.os == 'windows') {
                    /* Replacement may contain $(VS) */
                    if (!bit.packs.compiler.dir.contains('$'))
                        value = value.replace(/\$\(VS\)/g, bit.packs.compiler.dir)
                    if (!bit.packs.winsdk.path.contains('$'))
                        value = value.replace(/\$\(SDK\)/g, bit.packs.winsdk.path)
                }
                if (env[key] && (key == 'PATH' || key == 'INCLUDE' || key == 'LIB')) {
                    env[key] = value + App.SearchSeparator + env[key]
                } else {
                    env[key] = value
                }
            }
            cmd.env = env
        }
        App.log.debug(2, "Command " + command)
        App.log.debug(3, "Env " + serialize(cmd.env, {pretty: true, indent: 4, commas: true, quotes: false}))
        cmd.start(command, cmdOptions)
        if (cmd.status != 0) {
            let msg
            if (!cmd.error || cmd.error == '') {
                msg = 'Command failure: ' + cmd.response + '\nCommand: ' + command
            } else {
                msg = 'Command failure: ' + cmd.error + '\n' + cmd.response + '\nCommand: ' + command
            }
            if (cmdOptions.continueOnErrors || options['continue']) {
                trace('Error', msg)
            } else {
                throw msg
            }
        } else if (!cmdOptions.noshow) {
            if (!cmdOptions.filter || !cmdOptions.filter.test(command)) {
                if (cmd.error) {
                    if (!cmdOptions.excludeOutput || !cmdOptions.excludeOutput.test(cmd.error)) {
                        print(cmd.error)
                    }
                }
                if (cmd.response) {
                    if (!cmdOptions.excludeOutput || !cmdOptions.excludeOutput.test(cmd.response)) {
                        print(cmd.response)
                    }
                }
            }
        }
        return cmd.response
    }

    /*
        Make required output directories (carefully). Only make dirs inside the 'src' or 'top' directories.
     */
    function makeOutDirs() {
        for each (d in bit.dir) {
            if (d.startsWith(bit.dir.top) || d.startsWith(bit.dir.src)) {
                d.makeDir()
            }
        }
    }

    public function trace(tag: String, ...args): Void {
        if (!options.quiet) {
            let msg = args.join(" ")
            let msg = "%12s %s" % (["[" + tag + "]"] + [msg]) + "\n"
            out.write(msg)
        }
    }

    public function strace(tag, msg) {
        if (options.show) {
            trace(tag, msg)
        }
    }

    public function vtrace(tag, msg) {
        if (options.verbose) {
            trace(tag, msg)
        }
    }

    public function whyRebuild(path, tag, msg) {
        if (options.why) {
            trace(tag, path + ' because ' + msg)
        }
    }

    function whySkip(path, msg) {
        if (options.why) {
            trace('Target', path + ' ' + msg)
        }
    }

    function whyMissing(msg) {
        if (options.why) {
            trace('Missing', msg)
        }
    }

    function diagnose(msg) {
        if (options.diagnose) {
            trace('Debug', msg)
        }
    }

    /*
        Run an action
     */
    public function action(cmd: String, actionOptions: Object = {}) {
        switch (cmd) {
        case 'cleanTargets':
            for each (target in bit.targets) {
                if (target.enable && target.path && targetsToClean[target.type]) {
                    if (!target.built && !target.precious && !target.nogen) {
                        if (generating == 'make') {
                            genWrite('\trm -rf ' + reppath(target.path))

                        } else if (generating == 'nmake') {
                            genout.writeLine('\t-if exist ' + reppath(target.path) + ' del /Q ' + reppath(target.path))

                        } else if (generating == 'sh') {
                            genWrite('rm -rf ' + target.path.relative)

                        } else {
                            if (target.path.exists) {
                                if (options.show) {
                                    trace('Clean', target.path.relative)
                                }
                                safeRemove(target.path)
                            }
                            if (Config.OS == 'windows') {
                                let ext = target.path.extension
                                if (ext == bit.ext.shobj || ext == bit.ext.exe) {
                                    target.path.replaceExt('lib').remove()
                                    target.path.replaceExt('pdb').remove()
                                    target.path.replaceExt('exp').remove()
                                }
                            }
                        }
                    }
                }
            }
            break
        }
    }

    function genWrite(str) {
        genout.writeLine(repvar(str))
    }

    function like(os) {
        if (posix.contains(os)) {
            return "posix"
        } else if (windows.contains(os)) {
            return "windows"
        }
        return ""
    }

    function programFiles(): Path {
        /*
            If we are a 32 bit program, we don't get to see /Program Files (x86)
         */
        let programs: Path
        if (Config.OS == 'windows') {
            let pvar = App.getenv('PROGRAMFILES')
            let pf64 = Path(pvar + ' (x86)')
            programs = Path(pf64.exists ? pf64 : pvar)
            if (!programs) {
                for each (drive in (FileSystem.drives() - ['A', 'B'])) {
                    let pf = Path(drive + ':\\').files('Program Files*')
                    if (pf.length > 0) {
                        return pf[0].portable
                    }
                }
            }
        } else {
            programs = Path("/usr/local/bin")
        }
        return programs.portable
    }

    function dist(os) {
        let dist = { macosx: 'apple', windows: 'ms', 'linux': 'ubuntu', 'vxworks': 'WindRiver' }[os]
        if (os == 'linux') {
            let relfile = Path('/etc/redhat-release')
            if (relfile.exists) {
                let rver = relfile.readString()
                if (rver.contains('Fedora')) {
                    dist = 'fedora'
                } else if (rver.contains('Red Hat Enterprise')) {
                    dist = 'rhl'
                } else {
                    dist = 'fedora'
                }
            } else if (Path('/etc/SuSE-release').exists) {
                dist = 'suse'
            } else if (Path('/etc/gentoo-release').exists) {
                dist = 'gentoo'
            }
        }
        return dist
    }

    public static function load(o: Object, ns = null) {
        b.loadBitObject(o, ns)
    }

    public function safeRemove(dir: Path) {
/* UNUSED MOB
        if (dir.isAbsolute)  {
            //  Comparison with top doesn't handle C: vs c:
            if (bit.dir.top.same('/') || !dir.startsWith(bit.dir.top)) {
                if (!options.force) {
                    throw new Error('Unsafe attempt to remove ' + dir + ' expected parent ' + bit.dir.top)
                }
            }
        }
*/
        dir.removeAll()
    }

    /*
        Make a bit object. This may optionally load a bit file over the initialized object
     */
    function makeBit(platform: String, bitfile: Path) {
        let [os, arch, cpu] = platform.split('-') 
        let kind = like(os)
        global.bit = bit = makeBareBit()
        bit.dir.src = options.configure || Path('.')
        bit.dir.bits = bit.dir.src.join('bits/standard.bit').exists ? 
            bit.dir.src.join('bits') : Config.Bin.join('bits').portable
        bit.dir.top = '.'
        if (kind == 'windows') {
            bit.dir.programs = programFiles()
            bit.dir.programFiles = Path(bit.dir.programs.name.replace(' (x86)', ''))
        } else {
            bit.dir.programs = Path('/usr/local/bin')
        }
        let profile = options.profile || 'debug'
        bit.platform = { 
            name: platform, 
            os: os,
            arch: arch,
            like: kind, 
            dist: dist(os),
            profile: profile,
            configuration: platform + '-' + profile,
            dev: localPlatform,
        }
        if (cpu) {
            bit.platform.cpu = cpu
        }
        loadBitFile(bit.dir.bits.join('standard.bit'))
        loadBitFile(bit.dir.bits.join('os/' + bit.platform.os + '.bit'))
        bit.globals.PLATFORM = currentPlatform = platform
        if (bitfile) {
            loadBitFile(bitfile)
            /*
                Customize bit files must be applied after the enclosing bit file is fully loaded so they
                can override anything.
             */
            for each (path in bit.customize) {
                let path = home.join(expand(path, {fill: '.'}))
                if (path.exists) {
                    loadBitFile(path)
                }
            }
        }
        for (let [key,value] in bit.ext.clone()) {
            if (value) {
                bit.ext['dot' + key] = '.' + value
            } else {
                bit.ext['dot' + key] = value
            }
        }
        if (!bit.settings.configured && !options.configure) {
            loadBitFile(bit.dir.bits.join('simple.bit'))
        }
        expandTokens(bit)
        loadModules()
        applyProfile()
        bit.userSettings = bit.settings.clone(true)
        applyCommandLineOptions(platform)
        applyEnv()
        setPathEnvVar(bit)
        castDirTypes()
        if (platform == localPlatform) {
            bit.globals.LBIN = localBin = bit.dir.bin.portable
        }
    }


    function quickLoad(bitfile: Path) {
        global.bit = bit = makeBareBit()
        bit.quickLoad = true
        loadBitFile(bitfile)
    }

    function validatePlatform(os, arch) {
        if (!supportedOS.contains(os)) {
            trace('WARN', 'Unsupported or unknown operating system: ' + os + '. Select from: ' + supportedOS.join(' '))
        }
        if (!supportedArch.contains(arch)) {
            trace('WARN', 'Unsupported or unknown architecture: ' + arch + '. Select from: ' + supportedArch.join(' '))
        }
    }

    function makeBareBit() {
        let old = bit
        bit = bareBit.clone(true)
        bit.platforms = old.platforms
        bit.cross = old.cross || false
        return bit
    }

    public function expand(s: String, options = {fill: ''}) : String {
        /* 
            Do twice to allow tokens to use ${vars} 
         */
        let eo = {fill: '${}'}
        s = s.expand(bit, eo)
        s = s.expand(bit.globals, eo)
        s = s.expand(bit, eo)
        return s.expand(bit.globals, eo)
    }

    function expandRule(target, rule) {
        setRuleVars(target)
        let result = expand(rule).expand(target.vars, {fill: ''})
        target.vars = {}
        return result
    }
}

} /* bit module */


/*
    Global functions for bit files
 */
require embedthis.bit

public var b: Bit = new Bit
b.main()

public function pack(name: String, description: String) {
    let packs = {}
    packs[name] = {description: description}
    Bit.load({packs: packs})
}

public function probe(file: Path, options = {}): Path {
    return b.probe(file, options)
}

/*
    Resolve a program. Name can be either a path or a basename with optional extension
 */
public function program(name: Path, description = null): Path {
    let path = bit.packs[name.trimExt()].path || name
    let packs = {}
    let cfg = {description: description}
    packs[name] = cfg
    try {
        cfg.path = probe(name, {fullpath: true})
    } catch (e) {
        throw e
    }
    Bit.load({packs: packs})
    return cfg.path
}

public function action(command: String, options = null)
    b.action(command, options)

public function trace(tag, msg)
    b.trace(tag, msg)

public function vtrace(tag, msg)
    b.vtrace(tag, msg)

public function install(src, dest: Path, options = {})
    b.install(src, dest, options)

public function package(formats)
    b.package(formats)

public function run(command, options = {})
    b.run(command, options)

public function safeRemove(dir: Path)
    b.safeRemove(dir)

public function mapLibs(libs: Array, static = null)
    b.mapLibs(libs, static)

public function setRuleVars(target, dir = App.dir)
    b.setRuleVars(target, dir)

public function makeDirGlobals(base: Path? = null)
    b.makeDirGlobals(base)

public function runTargetScript(target, when)
    b.runTargetScript(target, when)

public function whyRebuild(path, tag, msg)
    b.whyRebuild(path, tag, msg)

public function expand(rule)
    b.expand(rule)

/*
    @copy   default
  
    Copyright (c) Embedthis Software LLC, 2003-2012. All Rights Reserved.
    Copyright (c) Michael O'Brien, 1993-2012. All Rights Reserved.
  
    This software is distributed under commercial and open source licenses.
    You may use the GPL open source license described below or you may acquire
    a commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details.
  
    This software is open source; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by the
    Free Software Foundation; either version 2 of the License, or (at your
    option) any later version. See the GNU General Public License for more
    details at: http://www.embedthis.com/downloads/gplLicense.html
  
    This program is distributed WITHOUT ANY WARRANTY; without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
    This GPL license does NOT permit incorporating this software into
    proprietary programs. If you are unable to comply with the GPL, you must
    acquire a commercial license to use this software. Commercial licenses
    for this software and support services are available from Embedthis
    Software at http://www.embedthis.com
  
    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */

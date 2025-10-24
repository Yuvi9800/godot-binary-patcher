from SCons.Script import ARGUMENTS, Environment, Mkdir, Default, File, CacheDir
import os
import sys

# Resolve platform/target/arch from args (matches CI defaults)
platform = ARGUMENTS.get('platform')
if not platform:
    p = sys.platform
    if p.startswith('win'):
        platform = 'windows'
    elif p == 'darwin':
        platform = 'macos'
    else:
        platform = 'linux'

target = ARGUMENTS.get('target', 'template_release')
arch = ARGUMENTS.get('arch', 'universal' if platform == 'macos' else 'x86_64')
if arch not in ['x86_64', 'x86_32', 'arm64', 'universal']:
    print(f"ERROR: Invalid architecture '{arch}'. Supported architectures are: x86_64, x86_32, arm64, universal.")
    Exit(1)

# Set up the environment based on the platform
use_mingw_arg = ARGUMENTS.get('use_mingw', 'no')
use_mingw = use_mingw_arg.lower() in ['yes', 'true', '1']

if platform == 'windows' and not use_mingw:
    # Use the MSVC compiler on Windows (native build)
    env = Environment(tools=['default', 'msvc'])
elif platform == 'windows' and use_mingw:
    # Force SCons to use the MinGW toolchain for cross-compilation
    env = Environment(tools=['gcc', 'g++', 'gnulink', 'ar', 'gas'])
    
    # Explicitly override compiler settings after environment creation
    cc_cmd = os.environ.get('CC', 'x86_64-w64-mingw32-gcc')
    cxx_cmd = os.environ.get('CXX', 'x86_64-w64-mingw32-g++')
    
    env.Replace(CC=cc_cmd)
    env.Replace(CXX=cxx_cmd)
    env.Replace(LINK=cxx_cmd)  # Use C++ compiler for linking (g++) so libstdc++ and C++ EH symbols are pulled in

else:
    # Use the default compiler on other platforms
    env = Environment()

# Optional: enable SCons cache if SCONS_CACHE or SCONS_CACHE_DIR is provided (local or CI)
cache_dir = os.environ.get('SCONS_CACHE') or os.environ.get('SCONS_CACHE_DIR')
if cache_dir:
    CacheDir(cache_dir)
    print(f"SCons cache enabled at: {cache_dir}")

# Add include paths for godot-cpp
env.Append(CPPPATH=[
    'src',
    '.',
    'godot-cpp/include',
    'godot-cpp/gen/include',
    'godot-cpp/gdextension',
    'HDiffPatch',
    'libdeflate',
    'lzma/C',
    'libmd5',
    'HDiffPatch/lzma/C',
    'zstd/lib'
])

# Platform-specific include paths - only add system paths for native builds
if not use_mingw and platform != 'windows':
    # Only add Linux/macOS system paths for native builds
    env.Append(CPPPATH=[
        '/usr/include',
        '/usr/local/include',
    ])
    if platform == 'linux':
        env.Append(CPPPATH=[
            '/usr/include/dbus-1.0',
            '/usr/lib/x86_64-linux-gnu/dbus-1.0/include',
        ])
elif use_mingw or platform == 'windows':
    # For Windows cross-compilation, add MinGW-specific paths
    env.Append(CPPPATH=[
        '/usr/x86_64-w64-mingw32/include',
    ])
    # Ensure MinGW uses its own sysroot and headers
    env.Append(CCFLAGS=['--sysroot=/usr/x86_64-w64-mingw32'])
    env.Append(LINKFLAGS=['--sysroot=/usr/x86_64-w64-mingw32'])

# Platform-specific library paths
env.Append(LIBPATH=['godot-cpp/bin'])

is_windows = platform == 'windows'
if is_windows and not use_mingw:
    # MSVC flags
    env.Append(CXXFLAGS=['/std:c++17'])
else:
    # Linux/macOS/MinGW flags
    env.Append(CCFLAGS=['-fPIC'])
    env.Append(CXXFLAGS=['-std=c++17'])
    env.Append(CPPDEFINES=['_HDIFFPATCH_DLL_BUILD_', '_LARGEFILE_SOURCE', '_FILE_OFFSET_BITS=64'])

    # macOS: Add architecture flags for cross-compilation
    if platform == 'macos' and arch != 'universal':
        # Split -arch and the architecture value into separate list items
        env.Append(CCFLAGS=['-arch', arch])
        env.Append(LINKFLAGS=['-arch', arch])

    # macOS: Ensure assembler uses the correct architecture for .S sources (e.g., zstd huf_decompress_amd64.S)
    if platform == 'macos':
        # Use the C compiler as the assembler/preprocessor driver so -arch is honored
        env['AS'] = env.get('CC')
        env['ASPP'] = env.get('CC')
        # Flags for plain .s (ASFLAGS) and preprocessed .S (ASPPFLAGS)
        env.Append(ASFLAGS=['-x', 'assembler-with-cpp'])
        env.Append(ASPPFLAGS=['-x', 'assembler-with-cpp'])
        if arch != 'universal':
            env.Append(ASFLAGS=['-arch', arch])
            env.Append(ASPPFLAGS=['-arch', arch])

if is_windows and use_mingw:
    # Add Windows-specific defines for MinGW
    env.Append(CPPDEFINES=['WIN32', '_WIN32', 'WINDOWS_ENABLED'])
    # Match godot-cpp's MinGW linking configuration for compatibility
    env.Append(CCFLAGS=['-Wwrite-strings'])
    env.Append(LINKFLAGS=['-Wl,--no-undefined'])
    # Use static linking to match godot-cpp's default use_static_cpp=True behavior
    env.Append(LINKFLAGS=['-static', '-static-libgcc', '-static-libstdc++'])

# Enable debug logging for media keys (can be disabled for production)
# Uncomment the line below to enable debug logging:
# env.Append(CPPDEFINES=['MEDIA_KEYS_DEBUG'])

# When using MinGW for cross-compilation, we get .a files with lib prefix
# godot-cpp-builds (NodotProject action) uses lib prefix even for MSVC
if is_windows and not use_mingw:
    lib_ext = '.lib'
    lib_prefix = 'lib'  # godot-cpp-builds uses lib prefix even for MSVC
else:
    lib_ext = '.a'
    lib_prefix = 'lib'

# Add godot-cpp library
# For macOS, try arch-specific first, then fall back to universal
if platform == 'macos' and arch != 'universal':
    arch_specific = f"{lib_prefix}godot-cpp.{platform}.{target}.{arch}{lib_ext}"
    universal = f"{lib_prefix}godot-cpp.{platform}.{target}.universal{lib_ext}"

    arch_specific_path = os.path.join('godot-cpp', 'bin', arch_specific)
    universal_path = os.path.join('godot-cpp', 'bin', universal)

    if os.path.exists(arch_specific_path):
        godot_cpp_lib = arch_specific
        print(f"Using arch-specific godot-cpp library: {arch_specific}")
    elif os.path.exists(universal_path):
        godot_cpp_lib = universal
        print(f"Using universal godot-cpp library: {universal}")
    else:
        print(f"ERROR: No suitable godot-cpp library found!")
        print(f"Tried: {arch_specific_path}")
        print(f"Tried: {universal_path}")
        Exit(1)
else:
    godot_cpp_lib = f"{lib_prefix}godot-cpp.{platform}.{target}.{arch}{lib_ext}"

env.Append(LIBS=[File(os.path.join('godot-cpp', 'bin', godot_cpp_lib))])

if is_windows:
    # ws2_32 and bcrypt are needed by godot-cpp
    # user32 is needed for Windows message handling (CreateWindowEx, GetMessage, etc.)
    # pthread is needed by godot-cpp (winpthreads for MinGW)
    env.Append(LIBS=['ws2_32', 'bcrypt', 'user32', 'pthread'])
elif platform == 'linux':
    env.Append(LIBS=['pthread', 'dl', 'dbus-1', 'z', 'bz2'])
    env.Append(LINKFLAGS=['-Wl,--no-undefined'])
elif platform == 'macos':
    env.Append(LIBS=['pthread'])
    # CoreFoundation and CoreGraphics are needed for CGEventTap
    # AppKit/Foundation are needed for NSEvent handling
    # Carbon is needed for media key codes (NX_KEYTYPE_*)
    # IOKit is needed for ev_keymap.h
    env.Append(FRAMEWORKS=['CoreFoundation', 'CoreGraphics', 'AppKit', 'Carbon', 'IOKit', 'SystemConfiguration', 'Security'])


# Debug logging for CI: print resolved names and compiler locations
print("=== SCons debug: resolved build variables ===")
print("platform:", platform)
print("target:", target)
print("arch:", arch)
print("use_mingw:", ARGUMENTS.get('use_mingw', 'no'))
print("expected godot_cpp_lib:", godot_cpp_lib)
print("godot-cpp bin path:", os.path.join('godot-cpp', 'bin'))
print("ENV CC:", os.environ.get('CC'))
print("ENV CXX:", os.environ.get('CXX'))
print("env['CC']:", env.get('CC'))
print("env['CXX']:", env.get('CXX'))
print("============================================")

src_files = [
    'src/register_types.cpp',
    'src/godot_binary_patcher.cpp',
    'src/hdiff_wrapper.cpp',
    'HDiffPatch/libHDiffPatch/HPatch/patch.c',
    'HDiffPatch/file_for_patch.c',
    'HDiffPatch/libHDiffPatch/HDiff/diff.cpp',
    'HDiffPatch/libHDiffPatch/HDiff/private_diff/suffix_string.cpp',
    'HDiffPatch/libHDiffPatch/HDiff/match_block.cpp',
    'HDiffPatch/libHDiffPatch/HDiff/private_diff/limit_mem_diff/stream_serialize.cpp',
    'HDiffPatch/libHDiffPatch/HDiff/private_diff/limit_mem_diff/adler_roll.c',
    'HDiffPatch/libHDiffPatch/HDiff/private_diff/limit_mem_diff/digest_matcher.cpp',
    'HDiffPatch/libHDiffPatch/HPatch/hpatch_mt/_hpatch_mt.c',
    'HDiffPatch/libHDiffPatch/HDiff/private_diff/bytes_rle.cpp',
    'HDiffPatch/libHDiffPatch/HDiff/private_diff/compress_detect.cpp',
    'HDiffPatch/libHDiffPatch/HPatchLite/hpatch_lite.c',
    'HDiffPatch/dirDiffPatch/dir_diff/dir_diff_tools.cpp',
    'HDiffPatch/dirDiffPatch/dir_diff/dir_diff.cpp',
    'HDiffPatch/dirDiffPatch/dir_diff/dir_manifest.cpp',
    'HDiffPatch/dirDiffPatch/dir_patch/dir_patch_tools.c',
    'HDiffPatch/dirDiffPatch/dir_patch/dir_patch.c',
    'HDiffPatch/compress_parallel.cpp',
    'libmd5/md5.c',
    'HDiffPatch/libHDiffPatch/HDiff/private_diff/libdivsufsort/divsufsort.cpp',
    'HDiffPatch/libParallel/parallel_import_c.c',
    'HDiffPatch/bsdiff_wrapper/bsdiff_wrapper.cpp',
    'HDiffPatch/bsdiff_wrapper/bspatch_wrapper.c',
    'HDiffPatch/vcdiff_wrapper/vcdiff_wrapper.cpp',
    'HDiffPatch/vcdiff_wrapper/vcpatch_wrapper.c',
    'HDiffPatch/libHDiffPatch/HPatch/hpatch_mt/hpatch_mt.c',
    'HDiffPatch/dirDiffPatch/dir_patch/ref_stream.c',
    'HDiffPatch/dirDiffPatch/dir_patch/res_handle_limit.c',
    'HDiffPatch/dirDiffPatch/dir_patch/new_dir_output.c',
    'HDiffPatch/libParallel/parallel_channel.cpp',
    'HDiffPatch/libHDiffPatch/HPatch/hpatch_mt/_hinput_mt.c',
    'HDiffPatch/libHDiffPatch/HPatch/hpatch_mt/_hcache_old_mt.c',
    'HDiffPatch/libHDiffPatch/HPatch/hpatch_mt/_houtput_mt.c',
    'lzma/C/LzmaDec.c',
    'lzma/C/Lzma2Dec.c',
    'lzma/C/XzDec.c',
    'HDiffPatch/dirDiffPatch/dir_patch/new_stream.c',
    'HDiffPatch/libHDiffPatch/HDiff/private_diff/libdivsufsort/divsufsort64.cpp',
    'lzma/C/7zCrc.c',
    'lzma/C/Delta.c',
    'lzma/C/Sha256.c',
    'zstd/lib/common/debug.c',
    'zstd/lib/common/entropy_common.c',
    'zstd/lib/common/error_private.c',
    'zstd/lib/common/fse_decompress.c',
    'zstd/lib/common/xxhash.c',
    'zstd/lib/common/zstd_common.c',
    'zstd/lib/decompress/huf_decompress.c',
    'zstd/lib/decompress/zstd_decompress.c',
    'lzma/C/Bra86.c',
    'lzma/C/XzEnc.c',
    'lzma/C/XzCrc64.c',
    'lzma/C/7zCrcOpt.c',
    'lzma/C/CpuArch.c',
    'lzma/C/Xz.c',
    'lzma/C/XzCrc64Opt.c',
    'zstd/lib/decompress/zstd_ddict.c',
    'zstd/lib/decompress/zstd_decompress_block.c',
    'lzma/C/Bra.c',
    'lzma/C/Sha256Opt.c',
    'lzma/C/MtDec.c',
    'lzma/C/MtCoder.c',
    'lzma/C/Threads.c',
    'lzma/C/LzmaEnc.c',
    'lzma/C/LzmaLib.c',
    'lzma/C/Lzma2Enc.c',
    'lzma/C/Alloc.c',
    'lzma/C/7zStream.c',
    'lzma/C/LzFind.c',
    'lzma/C/LzFindMt.c',
    'lzma/C/LzFindOpt.c'
]

# Only build x86_64-optimized Zstd asm on x86_64 builds
if arch == 'x86_64':
    src_files.append('zstd/lib/decompress/huf_decompress_amd64.S')


env.Execute(Mkdir('addons/godot-binary-patcher/bin'))

# Set the correct library suffix and prefix based on platform
if is_windows:
    env['SHLIBPREFIX'] = 'lib'
    env['SHLIBSUFFIX'] = '.dll'
elif platform == 'macos':
    env['SHLIBPREFIX'] = 'lib'
    env['SHLIBSUFFIX'] = '.dylib'
else:
    env['SHLIBPREFIX'] = 'lib'
    env['SHLIBSUFFIX'] = '.so'

# Debug logging: shared lib name details
print("SHLIBPREFIX:", env.get('SHLIBPREFIX'))
print("SHLIBSUFFIX:", env.get('SHLIBSUFFIX'))
print("Target shared lib will be created as:", env.get('SHLIBPREFIX') + 'godot_binary_patcher' + env.get('SHLIBSUFFIX'))

# Create the library directly in the addon directory
# For macOS with specific arch, include arch in the filename (not for universal)
# NOTE: SCons adds SHLIBPREFIX (lib) and SHLIBSUFFIX (.dylib/.so/.dll) automatically
# BUT: If the target contains a dot, SCons treats everything after the last dot as an extension
# So we must override SHLIBSUFFIX to include the arch to work around this issue
if platform == 'macos' and arch != 'universal':
    # For arch-specific builds, we want: libgodot_binary_patcher.{arch}.dylib
    # We override SHLIBSUFFIX to include the arch to work around SCons' extension detection
    env['SHLIBSUFFIX'] = f'.{arch}.dylib'
    library_target = 'addons/godot-binary-patcher/bin/godot_binary_patcher'
else:
    library_target = 'addons/godot-binary-patcher/bin/godot_binary_patcher'

library = env.SharedLibrary(target=library_target, source=src_files)
Default(library)
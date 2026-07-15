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

# Establish a sensible architecture default per platform
if platform == 'macos':
    default_arch = 'universal'
elif platform == 'android':
    default_arch = 'arm64-v8a'
else:
    default_arch = 'x86_64'

arch = ARGUMENTS.get('arch', default_arch)

supported_architectures = ['x86_64', 'x86_32', 'arm64-v8a', 'armeabi-v7a']
if arch not in supported_architectures:
    print(f"ERROR: Invalid architecture '{arch}'. Supported architectures are: {', '.join(supported_architectures)}.")
    Exit(1)

# Set up the environment based on the platform
use_mingw_arg = ARGUMENTS.get('use_mingw', 'no')
use_mingw = use_mingw_arg.lower() in ['yes', 'true', '1']

# Initialize SCons Environment with platform overrides
if platform == 'android':
    # Ensure SCons utilizes the Android NDK cross-compilers
    env = Environment(tools=['default'])
    
    ndk_root = os.environ.get('ANDROID_NDK_ROOT') or os.environ.get('ANDROID_NDK_HOME')
    if not ndk_root:
        print("ERROR: ANDROID_NDK_ROOT environment variable must be set to compile for Android.")
        Exit(1)

    # Resolve compiler tool paths based on host machine OS
    host_os = 'linux-x86_64'
    if sys.platform.startswith('win'):
        host_os = 'windows-x86_64'
    elif sys.platform == 'darwin':
        host_os = 'darwin-x86_64'

    toolchain_path = os.path.join(ndk_root, 'toolchains', 'llvm', 'prebuilt', host_os, 'bin')
    
    # API 21 is minimum for Godot 4, API 24-29 is standard. Suffix aligns compiler output.
    api_version = "24" 
    
    if arch == 'arm64-v8a':
        cc_name = f"aarch64-linux-android{api_version}-clang"
        cxx_name = f"aarch64-linux-android{api_version}-clang++"
        ar_name = "llvm-ar"
    elif arch == 'armeabi-v7a':
        cc_name = f"armv7a-linux-androideabi{api_version}-clang"
        cxx_name = f"armv7a-linux-androideabi{api_version}-clang++"
        ar_name = "llvm-ar"
    elif arch == 'x86_64':
        cc_name = f"x86_64-linux-android{api_version}-clang"
        cxx_name = f"x86_64-linux-android{api_version}-clang++"
        ar_name = "llvm-ar"
    else:
        print(f"ERROR: Android target does not support '{arch}' architecture.")
        Exit(1)

    # Override SCons binaries dynamically
    env.Replace(CC=os.path.join(toolchain_path, cc_name))
    env.Replace(CXX=os.path.join(toolchain_path, cxx_name))
    env.Replace(LINK=os.path.join(toolchain_path, cxx_name))
    env.Replace(AR=os.path.join(toolchain_path, ar_name))

elif platform == 'windows' and not use_mingw:
    env = Environment(tools=['default', 'msvc'])
elif platform == 'windows' and use_mingw:
    env = Environment(tools=['gcc', 'g++', 'gnulink', 'ar', 'gas'])
    cc_cmd = os.environ.get('CC', 'x86_64-w64-mingw32-gcc')
    cxx_cmd = os.environ.get('CXX', 'x86_64-w64-mingw32-g++')
    env.Replace(CC=cc_cmd)
    env.Replace(CXX=cxx_cmd)
    env.Replace(LINK=cxx_cmd)
else:
    env = Environment()

# Optional: enable SCons cache if SCONS_CACHE or SCONS_CACHE_DIR is provided
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

# Platform-specific include paths
if platform == 'android':
    # NDK toolchains handle system includes automatically; avoid adding host system directories
    pass
elif not use_mingw and platform != 'windows':
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
    env.Append(CPPPATH=['/usr/x86_64-w64-mingw32/include'])
    env.Append(CCFLAGS=['--sysroot=/usr/x86_64-w64-mingw32'])
    env.Append(LINKFLAGS=['--sysroot=/usr/x86_64-w64-mingw32'])

# Platform-specific library paths
env.Append(LIBPATH=['godot-cpp/bin'])

is_windows = platform == 'windows'
if is_windows and not use_mingw:
    env.Append(CXXFLAGS=['/std:c++17'])
else:
    env.Append(CCFLAGS=['-fPIC'])
    env.Append(CXXFLAGS=['-std=c++17'])
    env.Append(CPPDEFINES=['_HDIFFPATCH_DLL_BUILD_', '_LARGEFILE_SOURCE', '_FILE_OFFSET_BITS=64'])

    if platform == 'macos' and arch != 'universal':
        env.Append(CCFLAGS=['-arch', arch])
        env.Append(LINKFLAGS=['-arch', arch])

    if platform == 'macos':
        env['AS'] = env.get('CC')
        env['ASPP'] = env.get('CC')
        env.Append(ASFLAGS=['-x', 'assembler-with-cpp'])
        env.Append(ASPPFLAGS=['-x', 'assembler-with-cpp'])
        if arch != 'universal':
            env.Append(ASFLAGS=['-arch', arch])
            env.Append(ASPPFLAGS=['-arch', arch])

if is_windows and use_mingw:
    env.Append(CPPDEFINES=['WIN32', '_WIN32', 'WINDOWS_ENABLED'])
    env.Append(CCFLAGS=['-Wwrite-strings'])
    env.Append(LINKFLAGS=['-Wl,--no-undefined'])
    env.Append(LINKFLAGS=['-static', '-static-libgcc', '-static-libstdc++'])

if platform == 'android':
    env.Append(CPPDEFINES=['ANDROID_ENABLED', 'UNIX_ENABLED'])
    env.Append(LINKFLAGS=['-shared', '-Wl,--no-undefined'])

# Configure static binding output libraries
if is_windows and not use_mingw:
    lib_ext = '.lib'
    lib_prefix = 'lib'
else:
    lib_ext = '.a'
    lib_prefix = 'lib'

# Resolve native static binding dependency names
if platform == 'macos' and arch != 'universal':
    arch_specific = f"{lib_prefix}godot-cpp.{platform}.{target}.{arch}{lib_ext}"
    universal = f"{lib_prefix}godot-cpp.{platform}.{target}.universal{lib_ext}"

    arch_specific_path = os.path.join('godot-cpp', 'bin', arch_specific)
    universal_path = os.path.join('godot-cpp', 'bin', universal)

    if os.path.exists(arch_specific_path):
        godot_cpp_lib = arch_specific
    elif os.path.exists(universal_path):
        godot_cpp_lib = universal
    else:
        print(f"ERROR: No suitable godot-cpp library found!")
        Exit(1)
else:
    godot_cpp_lib = f"{lib_prefix}godot-cpp.{platform}.{target}.{arch}{lib_ext}"

env.Append(LIBS=[File(os.path.join('godot-cpp', 'bin', godot_cpp_lib))])

# Link dynamic system libraries
if is_windows:
    env.Append(LIBS=['ws2_32', 'bcrypt', 'user32', 'pthread'])
elif platform == 'linux':
    env.Append(LIBS=['pthread', 'dl', 'dbus-1', 'z', 'bz2'])
    env.Append(LINKFLAGS=['-Wl,--no-undefined'])
elif platform == 'macos':
    env.Append(LIBS=['pthread'])
    env.Append(FRAMEWORKS=['CoreFoundation', 'CoreGraphics', 'AppKit', 'Carbon', 'IOKit', 'SystemConfiguration', 'Security'])
elif platform == 'android':
    # Android handles logging, math, and runtime libraries natively
    env.Append(LIBS=['log', 'android', 'm'])

# Debug output log
print("=== SCons debug: resolved build variables ===")
print("platform:", platform)
print("target:", target)
print("arch:", arch)
print("expected godot_cpp_lib:", godot_cpp_lib)
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
    'HDiffPatch/dirDiffPatch/dir_manifest.cpp',
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

if arch == 'x86_64' and platform != 'android':
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
    # Handles both Linux and Android
    env['SHLIBPREFIX'] = 'lib'
    env['SHLIBSUFFIX'] = '.so'

if platform == 'macos' and arch != 'universal':
    env['SHLIBSUFFIX'] = f'.{arch}.dylib'

# Explicitly override the shared library target name format for Android architectures
if platform == 'android':
    library_target = f'addons/godot-binary-patcher/bin/godot_binary_patcher.android.{target}.{arch}'
else:
    library_target = 'addons/godot-binary-patcher/bin/godot_binary_patcher'

library = env.SharedLibrary(target=library_target, source=src_files)
Default(library)

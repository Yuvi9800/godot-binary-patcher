#!/usr/bin/env python
from SCons.Script import Default, Glob
from pathlib import Path
import os

# 1. Delegate environment setup to godot-cpp.
# This automatically configures Android NDK, MSVC/MinGW, macOS frameworks, and architectures.
env = SConscript("godot-cpp/SConstruct")

if env["platform"] == "android":
    env.Append(CXXFLAGS=["-fexceptions"])

# 2. Add all your custom include paths to the inherited environment
env.Append(CPPPATH=[
    'src',
    '.',
    'HDiffPatch',
    'libdeflate',
    'lzma/C',
    'libmd5',
    'HDiffPatch/lzma/C',
    'zstd/lib'
])

# 3. Define macro definitions needed for your patches
env.Append(CPPDEFINES=[
    '_HDIFFPATCH_DLL_BUILD_', 
    '_LARGEFILE_SOURCE', 
    '_FILE_OFFSET_BITS=64'
])

# 4. Gather all your source files
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

# Append assembly code for x86_64 desktop platforms
if env["arch"] == 'x86_64' and env["platform"] != 'android':
    src_files.append('zstd/lib/decompress/huf_decompress_amd64.S')

# 5. Resolve build target path dynamically using the inherited parameters
# This mirrors the naming structure of standard GDExtensions
suffix = env["SHLIBSUFFIX"]
target_name = f"godot_binary_patcher.{env['platform']}.{env['target']}.{env['arch']}{suffix}"
library_target = os.path.join("addons", "godot-binary-patcher", "bin", target_name)

# 6. Compile the Shared Library using inherited flags and toolchains
library = env.SharedLibrary(target=library_target, source=src_files)
Default(library)

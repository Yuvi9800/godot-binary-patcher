#include "hdiff_wrapper.h"
#include "HDiffPatch/libHDiffPatch/HPatch/patch.h"
#include "HDiffPatch/libHDiffPatch/HDiff/diff.h"
#include "HDiffPatch/file_for_patch.h"
#include <stdexcept>
#include <vector>
#include <atomic>
#include <thread>
#include <chrono>
#include <algorithm>
#if defined(_WIN32)
  #include <windows.h>
  static uint64_t _get_file_size(const char* path) {
      WIN32_FILE_ATTRIBUTE_DATA fad;
      if (GetFileAttributesExA(path, GetFileExInfoStandard, &fad)) {
          ULARGE_INTEGER li;
          li.HighPart = fad.nFileSizeHigh;
          li.LowPart = fad.nFileSizeLow;
          return (uint64_t)li.QuadPart;
      }
      return 0;
  }
#else
  #include <sys/stat.h>
  static uint64_t _get_file_size(const char* path) {
      struct stat st;
      if (stat(path, &st) == 0) return (uint64_t)st.st_size;
      return 0;
  }
#endif

static void _monitor_file_progress(const char* path, PatchStatus* status, std::atomic<bool>* stop_flag) {
    using namespace std::chrono_literals;
    while (!stop_flag->load(std::memory_order_relaxed) && !status->finished.load(std::memory_order_relaxed)) {
        const uint64_t done = _get_file_size(path);
        status->bytes_done.store(done, std::memory_order_relaxed);
        const uint64_t total = status->bytes_total.load(std::memory_order_relaxed);
        double ratio = 0.0;
        if (total > 0) {
            ratio = std::min(1.0, (double)done / (double)total);
        }
        status->progress.store(ratio, std::memory_order_relaxed);
        std::this_thread::sleep_for(25ms);
    }
}

void apply_patch(const char* old_filename, const char* patch_filename, const char* new_filename, PatchStatus* status) {
    try {
        hpatch_TFileStreamInput old_file;
        hpatch_TFileStreamInput patch_file;
        hpatch_TFileStreamOutput new_file;

        hpatch_TFileStreamInput_init(&old_file);
        hpatch_TFileStreamInput_init(&patch_file);
        hpatch_TFileStreamOutput_init(&new_file);

        if (!hpatch_TFileStreamInput_open(&old_file, old_filename))
            throw std::runtime_error("Failed to open old file");
        if (!hpatch_TFileStreamInput_open(&patch_file, patch_filename))
            throw std::runtime_error("Failed to open patch file");

        // Determine diff type and target size (for progress total)
        hpatch_StreamPos_t target_size = (hpatch_StreamPos_t)-1;
        bool is_single = false;
        hpatch_compressedDiffInfo cinfo;
        hpatch_singleCompressedDiffInfo sinfo;
        if (getCompressedDiffInfo(&cinfo, &patch_file.base)) {
            target_size = cinfo.newDataSize;
        } else if (getSingleCompressedDiffInfo(&sinfo, &patch_file.base, 0)) {
            target_size = sinfo.newDataSize;
            is_single = true;
        }

        if (!hpatch_TFileStreamOutput_open(&new_file, new_filename, target_size))
            throw std::runtime_error("Failed to open new file for writing");

        // Initialize progress tracking
        status->bytes_done.store(0);
        status->bytes_total.store((uint64_t)((target_size==(hpatch_StreamPos_t)-1)?0:target_size));
        status->progress.store(0.0);

        // Allow non-sequential writes if patcher performs random seeks (safe for all types)
        hpatch_TFileStreamOutput_setRandomOut(&new_file, hpatch_TRUE);

        // Start a lightweight monitor thread that polls the growing output file size
        std::atomic<bool> stop_flag(false);
        std::thread monitor(_monitor_file_progress, new_filename, status, &stop_flag);

        // Apply according to detected diff type
        if (is_single) {
            const size_t step_mem = (size_t)sinfo.stepMemSize;
            // Allocate a modest cache: stepMem + 1 MiB margin
            const size_t cache_size = step_mem + (1u << 20);
            std::vector<unsigned char> cache(cache_size);
            status->success = patch_single_compressed_diff(
                &new_file.base, &old_file.base, &patch_file.base,
                sinfo.diffDataPos, sinfo.uncompressedSize, sinfo.compressedSize,
                0,                // decompressPlugin (none needed if uncompressed)
                sinfo.coverCount, // coverCount
                (size_t)sinfo.stepMemSize,
                cache.data(), cache.data() + cache.size(),
                0,                // coversListener
                1                 // threadNum
            );
        } else {
            // Standard compressed diff (possibly uncompressed), plugin null when not required
            status->success = patch_decompress(&new_file.base, &old_file.base, &patch_file.base, 0);
        }

        // Finalize progress
        if (status->success.load()) {
            const uint64_t total = status->bytes_total.load();
            if (total > 0) {
                status->bytes_done.store(total);
                status->progress.store(1.0);
            } else {
                // If total is unknown, at least emit progress as done
                status->bytes_done.store(_get_file_size(new_filename));
                status->progress.store(1.0);
            }
        }
        stop_flag.store(true);
        if (monitor.joinable()) monitor.join();

        hpatch_TFileStreamInput_close(&old_file);
        hpatch_TFileStreamInput_close(&patch_file);
        hpatch_TFileStreamOutput_close(&new_file);
    } catch (const std::exception& e) {
        status->success = false;
    }
    status->finished = true;
}

void create_patch(const char* old_filename, const char* new_filename, const char* diff_filename, PatchStatus* status) {
    try {
        hpatch_TFileStreamInput old_file;
        hpatch_TFileStreamInput new_file;
        hpatch_TFileStreamOutput diff_file;

        hpatch_TFileStreamInput_init(&old_file);
        hpatch_TFileStreamInput_init(&new_file);
        hpatch_TFileStreamOutput_init(&diff_file);

        if (!hpatch_TFileStreamInput_open(&old_file, old_filename))
            throw std::runtime_error("Failed to open old file");
        if (!hpatch_TFileStreamInput_open(&new_file, new_filename))
            throw std::runtime_error("Failed to open new file");
        if (!hpatch_TFileStreamOutput_open(&diff_file, diff_filename, hpatch_kNullStreamPos))
            throw std::runtime_error("Failed to open diff file for writing");
        // Allow non-sequential writes during diff serialization
        hpatch_TFileStreamOutput_setRandomOut(&diff_file, hpatch_TRUE);

        // Initialize progress tracking. Use new file size as a rough total budget.
        const uint64_t new_size = (uint64_t)new_file.base.streamSize;
        status->bytes_total.store(new_size);
        status->bytes_done.store(0);
        status->progress.store(0.0);

        // Monitor the diff file growth as progress proxy
        std::atomic<bool> stop_flag(false);
        std::thread monitor(_monitor_file_progress, diff_filename, status, &stop_flag);

        // Create compressed-diff format without compression to avoid plugin requirements
        create_compressed_diff_stream(&new_file.base, &old_file.base, &diff_file.base, 0);
        status->success = true;

        // Finalize progress
        const uint64_t total = status->bytes_total.load();
        const uint64_t done  = _get_file_size(diff_filename);
        status->bytes_done.store(std::max(done, total)); // clamp up so done==total if possible
        if (total > 0) {
            status->progress.store(1.0);
        } else {
            // Unknown total, still mark as finished
            status->progress.store(1.0);
        }

        stop_flag.store(true);
        if (monitor.joinable()) monitor.join();

        hpatch_TFileStreamInput_close(&old_file);
        hpatch_TFileStreamInput_close(&new_file);
        hpatch_TFileStreamOutput_close(&diff_file);
    } catch (const std::exception& e) {
        status->success = false;
    }
    status->finished = true;
}
#ifndef HDIFF_WRAPPER_H
#define HDIFF_WRAPPER_H

#include <atomic>

struct PatchStatus {
    std::atomic<double> progress;
    std::atomic<uint64_t> bytes_done;
    std::atomic<uint64_t> bytes_total;
    std::atomic<bool> finished;
    std::atomic<bool> success;

    PatchStatus() {
        reset();
    }

    void reset() {
        progress.store(0.0);
        bytes_done.store(0);
        bytes_total.store(0);
        finished.store(false);
        success.store(false);
    }
};

void apply_patch(const char* old_file, const char* patch_file, const char* new_file, PatchStatus* status);
void create_patch(const char* old_file, const char* new_file, const char* diff_file, PatchStatus* status);

#endif // HDIFF_WRAPPER_H
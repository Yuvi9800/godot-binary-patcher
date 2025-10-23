#ifndef HDIFF_WRAPPER_H
#define HDIFF_WRAPPER_H

#include <atomic>

struct PatchStatus {
    std::atomic<double> progress;
    std::atomic<bool> finished;
    std::atomic<bool> success;
    std::atomic<bool> cancel_requested;

    PatchStatus() : progress(0.0), finished(false), success(false), cancel_requested(false) {}
};

void apply_patch(const char* old_file, const char* patch_file, const char* new_file, PatchStatus* status);

#endif // HDIFF_WRAPPER_H
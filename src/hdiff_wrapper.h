#ifndef HDIFF_WRAPPER_H
#define HDIFF_WRAPPER_H

#include <atomic>
#include <functional>
#include <string>

namespace hdiff_patch_wrapper {

struct PatchStatus {
    std::atomic<long long> current_size;
    std::atomic<long long> total_size;
    std::atomic<bool> cancel_flag;

    PatchStatus() : current_size(0), total_size(0), cancel_flag(false) {}
};

bool apply_patch(
    const std::string &old_file_path,
    const std::string &diff_file_path,
    const std::string &out_new_file_path,
    PatchStatus &status);

} // namespace hdiff_patch_wrapper

#endif // HDIFF_WRAPPER_H
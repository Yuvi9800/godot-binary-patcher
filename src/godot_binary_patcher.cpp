#include "godot_binary_patcher.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>

using namespace godot;

void BinaryPatcher::_bind_methods() {
    ClassDB::bind_method(D_METHOD("apply_patch_async", "old_file", "patch_file", "new_file"), &BinaryPatcher::apply_patch_async);
    ClassDB::bind_method(D_METHOD("cancel"), &BinaryPatcher::cancel);

    ADD_SIGNAL(MethodInfo("progress", PropertyInfo(Variant::FLOAT, "ratio"), PropertyInfo(Variant::INT, "bytes_done"), PropertyInfo(Variant::INT, "bytes_total")));
    ADD_SIGNAL(MethodInfo("finished", PropertyInfo(Variant::BOOL, "success")));
}

BinaryPatcher::BinaryPatcher() {
    // Constructor
}

BinaryPatcher::~BinaryPatcher() {
    if (patch_thread.joinable()) {
        patch_status.cancel_requested.store(true);
        patch_thread.join();
    }
}

void BinaryPatcher::_process(double delta) {
    if (patch_status.finished.load()) {
        if (patch_thread.joinable()) {
            patch_thread.join();
        }
        emit_signal("finished", patch_status.success.load());
        set_process(false);
    } else {
        // This is a simplified progress report. A more accurate report would need total bytes.
        emit_signal("progress", patch_status.progress.load(), 0, 0);
    }
}

void BinaryPatcher::apply_patch_async(const String& old_file, const String& patch_file, const String& new_file) {
    if (patch_thread.joinable()) {
        // Another patch is already in progress
        return;
    }

    patch_status.progress.store(0.0);
    patch_status.finished.store(false);
    patch_status.success.store(false);
    patch_status.cancel_requested.store(false);

    patch_thread = std::thread(apply_patch,
        old_file.utf8().get_data(),
        patch_file.utf8().get_data(),
        new_file.utf8().get_data(),
        &patch_status);

    set_process(true);
}

void BinaryPatcher::cancel() {
    if (patch_thread.joinable()) {
        patch_status.cancel_requested.store(true);
    }
}
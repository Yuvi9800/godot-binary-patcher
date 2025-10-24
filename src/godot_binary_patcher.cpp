#include "godot_binary_patcher.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/classes/engine.hpp>
#include <godot_cpp/classes/project_settings.hpp>
#include <godot_cpp/classes/os.hpp>

using namespace godot;

void BinaryPatcher::_bind_methods() {
    // Methods
    ClassDB::bind_method(D_METHOD("apply_patch_async", "old_file", "patch_file", "new_file"), &BinaryPatcher::apply_patch_async);
    ClassDB::bind_method(D_METHOD("create_patch_async", "old_file", "new_file", "diff_file"), &BinaryPatcher::create_patch_async);

    // Signals
    ADD_SIGNAL(MethodInfo("progress", PropertyInfo(Variant::FLOAT, "ratio"), PropertyInfo(Variant::INT, "bytes_done"), PropertyInfo(Variant::INT, "bytes_total")));
    ADD_SIGNAL(MethodInfo("finished", PropertyInfo(Variant::BOOL, "success")));
}

BinaryPatcher::BinaryPatcher() {
    // Ensure _process is called so we can emit progress/finished signals from the main thread
    set_process(true);
}

BinaryPatcher::~BinaryPatcher() {
    if (patch_thread.joinable()) {
        patch_thread.join();
    }
}

void BinaryPatcher::_enter_tree() {
    // Ensure _process() runs so we can emit "progress" and "finished" from the main thread.
    set_process(true);
}

void BinaryPatcher::_process(double delta) {
    if (Engine::get_singleton()->is_editor_hint()) {
        return;
    }

    if (patch_thread.joinable()) {
        if (patch_status.finished.load()) {
            patch_thread.join();
            emit_signal("finished", patch_status.success.load());
        } else {
            emit_signal("progress", patch_status.progress.load(), (int64_t)patch_status.bytes_done.load(), (int64_t)patch_status.bytes_total.load());
        }
    }
}

void BinaryPatcher::apply_patch_async(const String& old_file, const String& patch_file, const String& new_file) {
    if (patch_thread.joinable()) {
        return;
    }

    patch_status.reset();

    String user_data_dir = OS::get_singleton()->get_user_data_dir();
    String old_file_abs = old_file.replace("user://", user_data_dir + "/");
    String patch_file_abs = patch_file.replace("user://", user_data_dir + "/");
    String new_file_abs = new_file.replace("user://", user_data_dir + "/");

    patch_thread = std::thread([this, old_file_abs, patch_file_abs, new_file_abs]() {
        apply_patch(
            old_file_abs.utf8().get_data(),
            patch_file_abs.utf8().get_data(),
            new_file_abs.utf8().get_data(),
            &this->patch_status
        );
    });
}

void BinaryPatcher::create_patch_async(const String& old_file, const String& new_file, const String& diff_file) {
    if (patch_thread.joinable()) {
        return;
    }

    patch_status.reset();

    String user_data_dir = OS::get_singleton()->get_user_data_dir();
    String old_file_abs = old_file.replace("user://", user_data_dir + "/");
    String new_file_abs = new_file.replace("user://", user_data_dir + "/");
    String diff_file_abs = diff_file.replace("user://", user_data_dir + "/");

    patch_thread = std::thread([this, old_file_abs, new_file_abs, diff_file_abs]() {
        create_patch(
            old_file_abs.utf8().get_data(),
            new_file_abs.utf8().get_data(),
            diff_file_abs.utf8().get_data(),
            &this->patch_status
        );
    });
}

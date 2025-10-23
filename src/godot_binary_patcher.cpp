#include "hdiff_wrapper.h"

#include <godot_cpp/classes/os.hpp>
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/memory.hpp>
#include <godot_cpp/templates/vector.hpp>
#include <godot_cpp/variant/utility_functions.hpp>

#include <thread>

using namespace godot;

class BinaryPatcher : public RefCounted {
    GDCLASS(BinaryPatcher, RefCounted);

private:
    struct PatchThreadArgs {
        String old_path;
        String diff_path;
        String out_path;
        hdiff_patch_wrapper::PatchStatus *status;
        std::atomic<bool> *finished_flag;
        std::atomic<bool> *success_flag;
    };

    std::thread patch_thread;
    hdiff_patch_wrapper::PatchStatus patch_status;
    std::atomic<bool> thread_finished;
    std::atomic<bool> thread_success;

    void poll_patch_progress() {
        if (!thread_finished) {
            return;
        }

        if (patch_thread.joinable()) {
            patch_thread.join();
        }

        emit_signal("finished", thread_success.load());
        set_process(false);
    }

    static void patch_thread_func(void *p_userdata) {
        PatchThreadArgs *args = static_cast<PatchThreadArgs *>(p_userdata);

        bool success = hdiff_patch_wrapper::apply_patch(
                args->old_path.utf8().get_data(),
                args->diff_path.utf8().get_data(),
                args->out_path.utf8().get_data(),
                *args->status);

        args->success_flag->store(success);
        args->finished_flag->store(true);

        memdelete(args);
    }

protected:
    static void _bind_methods() {
        ClassDB::bind_method(D_METHOD("apply_patch_async", "old_path", "diff_path", "out_path"), &BinaryPatcher::apply_patch_async);
        ClassDB::bind_method(D_METHOD("cancel"), &BinaryPatcher::cancel);

        ADD_SIGNAL(MethodInfo("progress", PropertyInfo(Variant::FLOAT, "ratio"), PropertyInfo(Variant::INT, "bytes_done"), PropertyInfo(Variant::INT, "bytes_total")));
        ADD_SIGNAL(MethodInfo("finished", PropertyInfo(Variant::BOOL, "success")));
    }

public:
    BinaryPatcher() {
        thread_finished.store(false);
        thread_success.store(false);
    }

    ~BinaryPatcher() {
        if (patch_thread.joinable()) {
            patch_status.cancel_flag.store(true);
            patch_thread.join();
        }
    }

    void _process(double delta) {
        if (thread_finished) {
            poll_patch_progress();
            return;
        }

        long long current = patch_status.current_size.load();
        long long total = patch_status.total_size.load();
        if (total > 0) {
            double ratio = static_cast<double>(current) / total;
            emit_signal("progress", ratio, current, total);
        }
    }

    void apply_patch_async(const String &old_path, const String &diff_path, const String &out_path) {
        if (patch_thread.joinable()) {
            UtilityFunctions::push_warning("BinaryPatcher is already busy.");
            return;
        }

        patch_status = hdiff_patch_wrapper::PatchStatus();
        thread_finished.store(false);
        thread_success.store(false);

        PatchThreadArgs *args = memnew(PatchThreadArgs{
                old_path,
                diff_path,
                out_path,
                &patch_status,
                &thread_finished,
                &thread_success,
        });

        patch_thread = std::thread(patch_thread_func, args);

        set_process(true);
    }

    void cancel() {
        if (patch_thread.joinable() && !thread_finished) {
            patch_status.cancel_flag.store(true);
        }
    }
};

// GDExtension registration
#include "register_types.h"
#include <gdextension_interface.h>

void initialize_godot_binary_patcher_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
    ClassDB::register_class<BinaryPatcher>();
}

void uninitialize_godot_binary_patcher_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
GDExtensionBool GDE_EXPORT godot_binary_patcher_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization *r_initialization) {
    godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);
    init_obj.register_initializer(initialize_godot_binary_patcher_module);
    init_obj.register_terminator(uninitialize_godot_binary_patcher_module);
    init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);
    return init_obj.init();
}
}
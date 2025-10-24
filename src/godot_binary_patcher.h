#ifndef GODOT_BINARY_PATCHER_H
#define GODOT_BINARY_PATCHER_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/core/binder_common.hpp>

#include <thread>
#include "hdiff_wrapper.h"

using namespace godot;

class BinaryPatcher : public Node {
    GDCLASS(BinaryPatcher, Node);

private:
    std::thread patch_thread;
    PatchStatus patch_status;

protected:
    static void _bind_methods();

public:
    BinaryPatcher();
    ~BinaryPatcher();

    void _enter_tree() override;
    void _process(double delta);
    void apply_patch_async(const String& old_file, const String& patch_file, const String& new_file);
    void create_patch_async(const String& old_file, const String& new_file, const String& diff_file);
};

#endif // GODOT_BINARY_PATCHER_H
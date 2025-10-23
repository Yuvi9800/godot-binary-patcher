#include "godot_cpp/classes/ref.hpp"

// Add necessary headers here

class BinaryPatcher : public RefCounted {
    GDCLASS(BinaryPatcher, RefCounted);

protected:
    static void _bind_methods();

public:
    BinaryPatcher();
    ~BinaryPatcher();

    void apply_patch_async(const String &old_path, const String &diff_path, const String &out_path);
    void cancel();
};
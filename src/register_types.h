#ifndef REGISTER_TYPES_H
#define REGISTER_TYPES_H

#include <godot_cpp/core/class_db.hpp>

void initialize_godot_binary_patcher_module(godot::ModuleInitializationLevel p_level);
void uninitialize_godot_binary_patcher_module(godot::ModuleInitializationLevel p_level);

#endif // REGISTER_TYPES_H
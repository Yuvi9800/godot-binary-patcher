extends Control

const OLD_FILE = "user://old_file.bin"
const PATCH_FILE = "user://patch_file.hdiff"
const NEW_FILE = "user://new_file.bin"

@onready var progress_bar = $VBoxContainer/ProgressBar
@onready var status_label = $VBoxContainer/StatusLabel
@onready var patch_button = $VBoxContainer/PatchButton

var patcher_create: BinaryPatcher
var patcher_apply: BinaryPatcher

func _ready():
	patcher_create = BinaryPatcher.new()
	patcher_create.progress.connect(_on_create_progress)
	patcher_create.finished.connect(_on_create_finished)
	add_child(patcher_create)

	patcher_apply = BinaryPatcher.new()
	patcher_apply.progress.connect(_on_apply_progress)
	patcher_apply.finished.connect(_on_apply_finished)
	add_child(patcher_apply)

func _on_patch_button_pressed():
	_cleanup_files()
	patch_button.disabled = true
	progress_bar.value = 0
	status_label.text = "Status: Preparing files..."
	
	_create_files_fast()
	_start_patching()

func _create_files_fast():
	# Instantly create large files by seeking and writing one byte
	var file_size = 1024 * 1024 * 100 # 100MB
	
	var old_file = FileAccess.open(OLD_FILE, FileAccess.WRITE)
	old_file.seek(file_size - 1)
	old_file.store_8(0)
	old_file.close()
	
	var new_file_temp = FileAccess.open(NEW_FILE + ".tmp", FileAccess.WRITE)
	new_file_temp.seek(file_size - 1)
	new_file_temp.store_8(0)
	# Make a small change to the new file
	new_file_temp.seek(file_size / 2) # Change something in the middle
	new_file_temp.store_string("change")
	new_file_temp.close()
	
	progress_bar.value = 5 # Show a small amount of progress for file creation
	status_label.text = "Status: Files prepared."

func _start_patching():
	status_label.text = "Status: Creating patch..."
	var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
	var new_file_tmp_abs = ProjectSettings.globalize_path(NEW_FILE + ".tmp")
	var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)
	patcher_create.create_patch_async(old_file_abs, new_file_tmp_abs, patch_file_abs)

func _on_create_progress(ratio, bytes_done, bytes_total):
	progress_bar.value = ratio * 50 # Creation is first 50%
	status_label.text = "Status: Creating... (%d%%)" % [ratio * 100]

func _on_create_finished(success):
	if success:
		status_label.text = "Status: Applying patch..."
		var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
		var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)
		var new_file_abs = ProjectSettings.globalize_path(NEW_FILE)
		patcher_apply.apply_patch_async(old_file_abs, patch_file_abs, new_file_abs)
	else:
		status_label.text = "Status: Create patch failed."
		patch_button.disabled = false
		
	_cleanup_files()
	
func _cleanup_files():
	var dir = DirAccess.open("user://")
	if dir:
		dir.remove(OLD_FILE.get_file())
		dir.remove(NEW_FILE.get_file())
		dir.remove(PATCH_FILE.get_file())
		dir.remove((NEW_FILE + ".tmp").get_file())

func _on_apply_progress(ratio, bytes_done, bytes_total):
	progress_bar.value = 50 + (ratio * 50) # Application is second 50%
	status_label.text = "Status: Applying... (%d%%)" % [ratio * 100]

func _on_apply_finished(success):
	if success:
		status_label.text = "Status: Patch successful!"
		progress_bar.value = 100
	else:
		status_label.text = "Status: Apply patch failed."
	patch_button.disabled = false

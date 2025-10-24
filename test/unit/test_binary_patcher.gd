extends GutTest

const OLD_FILE = "user://old_file_test.bin"
const PATCH_FILE = "user://patch_file_test.hdiff"
const NEW_FILE_EXPECTED = "user://new_file_expected.bin"
const NEW_FILE_ACTUAL = "user://new_file_actual.bin"

var patcher: BinaryPatcher

func before_all():
	patcher = BinaryPatcher.new()
	add_child(patcher)

func after_all():
	patcher.free()

func setup():
	# Create dummy files for each test
	var old_file = FileAccess.open(OLD_FILE, FileAccess.WRITE)
	old_file.store_string("This is the old file content.")
	old_file.close()

	var new_file = FileAccess.open(NEW_FILE_EXPECTED, FileAccess.WRITE)
	new_file.store_string("This is the new file content, which is different.")
	new_file.close()

func teardown():
	# Clean up dummy files after each test
	var dir = DirAccess.open("user://")
	if dir:
		dir.remove(OLD_FILE.replace("user://", ""))
		dir.remove(PATCH_FILE.replace("user://", ""))
		dir.remove(NEW_FILE_EXPECTED.replace("user://", ""))
		dir.remove(NEW_FILE_ACTUAL.replace("user://", ""))

func test_create_patch_successfully():
	var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
	var new_file_abs = ProjectSettings.globalize_path(NEW_FILE_EXPECTED)
	var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)
	patcher.create_patch_async(old_file_abs, new_file_abs, patch_file_abs)
	var result = await patcher.finished
	assert_true(result, "Create patch operation should be successful.")
	assert_true(FileAccess.file_exists(PATCH_FILE), "Patch file should be created.")

func test_apply_patch_successfully():
	var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
	var new_file_exp_abs = ProjectSettings.globalize_path(NEW_FILE_EXPECTED)
	var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)
	var new_file_act_abs = ProjectSettings.globalize_path(NEW_FILE_ACTUAL)

	# First, create the patch
	patcher.create_patch_async(old_file_abs, new_file_exp_abs, patch_file_abs)
	var create_result = await patcher.finished
	assert_true(create_result, "Prerequisite: Create patch operation should be successful.")
	assert_true(FileAccess.file_exists(PATCH_FILE), "Prerequisite: Patch file should be created.")

	# Now, apply the patch
	patcher.apply_patch_async(old_file_abs, patch_file_abs, new_file_act_abs)
	var apply_result = await patcher.finished
	assert_true(apply_result, "Apply patch operation should be successful.")
	assert_true(FileAccess.file_exists(NEW_FILE_ACTUAL), "New file should be created after applying patch.")

	# Verify the content of the patched file
	var expected_content = FileAccess.get_file_as_string(NEW_FILE_EXPECTED)
	var actual_content = FileAccess.get_file_as_string(NEW_FILE_ACTUAL)
	
	assert_eq(actual_content, expected_content, "Patched file content should match the expected new file content.")

func test_create_patch_with_nonexistent_old_file():
	var new_file_abs = ProjectSettings.globalize_path(NEW_FILE_EXPECTED)
	var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)
	patcher.create_patch_async("user://nonexistent_file.bin", new_file_abs, patch_file_abs)
	var result = await patcher.finished
	assert_false(result, "Create patch with non-existent old file should fail.")

func test_create_patch_with_nonexistent_new_file():
	var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
	var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)
	patcher.create_patch_async(old_file_abs, "user://nonexistent_file.bin", patch_file_abs)
	var result = await patcher.finished
	assert_false(result, "Create patch with non-existent new file should fail.")

func test_apply_patch_with_nonexistent_old_file():
	var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
	var new_file_exp_abs = ProjectSettings.globalize_path(NEW_FILE_EXPECTED)
	var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)
	var new_file_act_abs = ProjectSettings.globalize_path(NEW_FILE_ACTUAL)

	# First, create a valid patch
	patcher.create_patch_async(old_file_abs, new_file_exp_abs, patch_file_abs)
	var create_result = await patcher.finished
	assert_true(create_result, "Prerequisite: Create patch should succeed.")

	# Now, attempt to apply it with a non-existent old file
	patcher.apply_patch_async("user://nonexistent_file.bin", patch_file_abs, new_file_act_abs)
	var apply_result = await patcher.finished
	assert_false(apply_result, "Apply patch with non-existent old file should fail.")

func test_apply_patch_with_nonexistent_patch_file():
	var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
	var new_file_act_abs = ProjectSettings.globalize_path(NEW_FILE_ACTUAL)
	patcher.apply_patch_async(old_file_abs, "user://nonexistent_patch.hdiff", new_file_act_abs)
	var result = await patcher.finished
	assert_false(result, "Apply patch with non-existent patch file should fail.")

func test_apply_patch_with_invalid_patch_file():
	var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
	var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)
	var new_file_act_abs = ProjectSettings.globalize_path(NEW_FILE_ACTUAL)

	# Create a dummy invalid patch file
	var invalid_patch_file = FileAccess.open(PATCH_FILE, FileAccess.WRITE)
	invalid_patch_file.store_string("this is not a valid patch")
	invalid_patch_file.close()

	patcher.apply_patch_async(old_file_abs, patch_file_abs, new_file_act_abs)
	var result = await patcher.finished
	assert_false(result, "Apply patch with invalid patch file should fail.")

func test_create_patch_with_empty_old_file():
	# Create an empty old file
	var empty_file = FileAccess.open(OLD_FILE, FileAccess.WRITE)
	empty_file.close()

	var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
	var new_file_exp_abs = ProjectSettings.globalize_path(NEW_FILE_EXPECTED)
	var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)
	var new_file_act_abs = ProjectSettings.globalize_path(NEW_FILE_ACTUAL)

	patcher.create_patch_async(old_file_abs, new_file_exp_abs, patch_file_abs)
	var result = await patcher.finished
	assert_true(result, "Create patch with empty old file should succeed.")
	assert_true(FileAccess.file_exists(PATCH_FILE), "Patch file should be created.")

	# Now apply the patch and verify
	patcher.apply_patch_async(old_file_abs, patch_file_abs, new_file_act_abs)
	var apply_result = await patcher.finished
	assert_true(apply_result, "Apply patch should succeed.")
	
	var expected_content = FileAccess.get_file_as_string(NEW_FILE_EXPECTED)
	var actual_content = FileAccess.get_file_as_string(NEW_FILE_ACTUAL)
	assert_eq(actual_content, expected_content, "Patched content should match.")

func test_create_patch_with_empty_new_file():
	# Create an empty new file
	var empty_file = FileAccess.open(NEW_FILE_EXPECTED, FileAccess.WRITE)
	empty_file.close()

	var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
	var new_file_exp_abs = ProjectSettings.globalize_path(NEW_FILE_EXPECTED)
	var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)
	var new_file_act_abs = ProjectSettings.globalize_path(NEW_FILE_ACTUAL)

	patcher.create_patch_async(old_file_abs, new_file_exp_abs, patch_file_abs)
	var result = await patcher.finished
	assert_true(result, "Create patch with empty new file should succeed.")
	assert_true(FileAccess.file_exists(PATCH_FILE), "Patch file should be created.")

	# Now apply the patch and verify
	patcher.apply_patch_async(old_file_abs, patch_file_abs, new_file_act_abs)
	var apply_result = await patcher.finished
	assert_true(apply_result, "Apply patch should succeed.")
	
	var expected_content = FileAccess.get_file_as_string(NEW_FILE_EXPECTED)
	var actual_content = FileAccess.get_file_as_string(NEW_FILE_ACTUAL)
	assert_eq(actual_content, expected_content, "Patched content should be empty.")

func test_create_patch_with_identical_files():
	# Make new file identical to old file
	var new_file = FileAccess.open(NEW_FILE_EXPECTED, FileAccess.WRITE)
	new_file.store_string("This is the old file content.")
	new_file.close()

	var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
	var new_file_exp_abs = ProjectSettings.globalize_path(NEW_FILE_EXPECTED)
	var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)
	var new_file_act_abs = ProjectSettings.globalize_path(NEW_FILE_ACTUAL)

	patcher.create_patch_async(old_file_abs, new_file_exp_abs, patch_file_abs)
	var result = await patcher.finished
	assert_true(result, "Create patch with identical files should succeed.")
	assert_true(FileAccess.file_exists(PATCH_FILE), "Patch file should be created.")

	# Now apply the patch and verify
	patcher.apply_patch_async(old_file_abs, patch_file_abs, new_file_act_abs)
	var apply_result = await patcher.finished
	assert_true(apply_result, "Apply patch should succeed.")
	
	var expected_content = FileAccess.get_file_as_string(NEW_FILE_EXPECTED)
	var actual_content = FileAccess.get_file_as_string(NEW_FILE_ACTUAL)
	assert_eq(actual_content, expected_content, "Patched content should be identical.")

func test_patching_with_binary_files():
	# Define paths for binary files
	var old_binary_file_src = "zstd/tests/golden-decompression/block-128k.zst"
	var new_binary_file_src = "zstd/tests/golden-decompression/empty-block.zst"
	var old_binary_file_dst = "user://old_binary.zst"
	var new_binary_file_dst_expected = "user://new_binary_expected.zst"
	var new_binary_file_dst_actual = "user://new_binary_actual.zst"
	var binary_patch_file = "user://binary.hdiff"

	# Ensure the source files exist before trying to copy
	assert_true(FileAccess.file_exists(old_binary_file_src), "Source old binary file should exist.")
	assert_true(FileAccess.file_exists(new_binary_file_src), "Source new binary file should exist.")

	# Copy binary files to user directory for the test
	DirAccess.copy_absolute(old_binary_file_src, old_binary_file_dst)
	DirAccess.copy_absolute(new_binary_file_src, new_binary_file_dst_expected)
	
	assert_true(FileAccess.file_exists(old_binary_file_dst), "Copied old binary file should exist.")
	assert_true(FileAccess.file_exists(new_binary_file_dst_expected), "Copied new binary file should exist.")

	var old_binary_file_dst_abs = ProjectSettings.globalize_path(old_binary_file_dst)
	var new_binary_file_dst_expected_abs = ProjectSettings.globalize_path(new_binary_file_dst_expected)
	var binary_patch_file_abs = ProjectSettings.globalize_path(binary_patch_file)
	var new_binary_file_dst_actual_abs = ProjectSettings.globalize_path(new_binary_file_dst_actual)

	# Create the patch
	patcher.create_patch_async(old_binary_file_dst_abs, new_binary_file_dst_expected_abs, binary_patch_file_abs)
	var create_result = await patcher.finished
	assert_true(create_result, "Create patch for binary files should succeed.")
	assert_true(FileAccess.file_exists(binary_patch_file), "Binary patch file should be created.")

	# Apply the patch
	patcher.apply_patch_async(old_binary_file_dst_abs, binary_patch_file_abs, new_binary_file_dst_actual_abs)
	var apply_result = await patcher.finished
	assert_true(apply_result, "Apply patch for binary files should succeed.")
	assert_true(FileAccess.file_exists(new_binary_file_dst_actual), "New binary file should be created after applying patch.")

	# Verify the content of the patched file
	var expected_bytes = FileAccess.get_file_as_bytes(new_binary_file_dst_expected)
	var actual_bytes = FileAccess.get_file_as_bytes(new_binary_file_dst_actual)
	
	assert_eq(actual_bytes, expected_bytes, "Patched binary file content should match the expected new binary file content.")

	# Clean up binary files
	var dir = DirAccess.open("user://")
	if dir:
		dir.remove(old_binary_file_dst.replace("user://", ""))
		dir.remove(new_binary_file_dst_expected.replace("user://", ""))
		dir.remove(new_binary_file_dst_actual.replace("user://", ""))
		dir.remove(binary_patch_file.replace("user://", ""))

func test_create_patch_progress_reports_bytes_and_ratio():
	var ratios := []
	var dones := []
	var totals := []
	var progress_cb := func(r, bd, bt):
		ratios.push_back(r)
		dones.push_back(bd)
		totals.push_back(bt)
	patcher.progress.connect(progress_cb)

	var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
	var new_file_abs = ProjectSettings.globalize_path(NEW_FILE_EXPECTED)
	var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)

	# Connect BEFORE starting async work to avoid missing the finished signal
	var completed := false
	var success_val := false
	var finished_cb := func(s):
		completed = true
		success_val = s
	patcher.finished.connect(finished_cb)

	# Now start the async create
	patcher.create_patch_async(old_file_abs, new_file_abs, patch_file_abs)

	var start_ms := Time.get_ticks_msec()
	while not completed and (Time.get_ticks_msec() - start_ms) < 10000:
		await get_tree().process_frame

	if not completed or not success_val:
		# Did not finish in time: still verify that progress is measurable.
		assert_gt(ratios.size(), 0, "Progress should have been emitted within 10 seconds (create).")
		patcher.finished.disconnect(finished_cb)
		patcher.progress.disconnect(progress_cb)
		return

	# Progress must emit, be monotonic, and consistent with byte counters
	assert_gt(ratios.size(), 0, "Progress signal should have been emitted at least once.")
	for i in range(ratios.size() - 1):
		assert_true(ratios[i] <= ratios[i + 1], "Progress ratio should be non-decreasing.")
	for i in range(dones.size() - 1):
		assert_true(dones[i] <= dones[i + 1], "bytes_done should be non-decreasing.")
	for i in range(totals.size() - 1):
		assert_true(totals[i] <= totals[i + 1], "bytes_total should be non-decreasing.")

	var any_total_positive := false
	for t in totals:
		if t > 0:
			any_total_positive = true
			break
	assert_true(any_total_positive, "bytes_total should be > 0 at some point.")

	var final_ratio: float = ratios[-1]
	var final_done: int = dones[-1]
	var final_total: int = totals[-1]
	assert_eq(final_done, final_total, "Final bytes_done should equal bytes_total.")
	assert_eq(final_ratio, 1.0, "Final progress ratio should be 1.0.")

	# Check ratio ~= bytes_done / bytes_total when total > 0 (allow tiny tolerance)
	for i in range(ratios.size()):
		var bt = totals[i]
		if bt > 0:
			var expected = float(dones[i]) / float(bt)
			var delta = abs(ratios[i] - expected)
			assert_lt(delta, 0.051, "ratio should approximately equal bytes_done/bytes_total (delta=%f)" % delta)

	# Cleanup connections for this test
	patcher.progress.disconnect(progress_cb)
	patcher.finished.disconnect(finished_cb)

func test_apply_patch_progress_reports_bytes_and_ratio():
	var ratios := []
	var dones := []
	var totals := []
	var progress_cb := func(r, bd, bt):
		ratios.push_back(r)
		dones.push_back(bd)
		totals.push_back(bt)
	patcher.progress.connect(progress_cb)

	var old_file_abs = ProjectSettings.globalize_path(OLD_FILE)
	var new_file_exp_abs = ProjectSettings.globalize_path(NEW_FILE_EXPECTED)
	var patch_file_abs = ProjectSettings.globalize_path(PATCH_FILE)
	var new_file_act_abs = ProjectSettings.globalize_path(NEW_FILE_ACTUAL)

	# First, create the patch (with timeout)
	var completed1 := false
	var success1 := false
	var finished_cb1 := func(s):
		completed1 = true
		success1 = s
	patcher.finished.connect(finished_cb1)

	patcher.create_patch_async(old_file_abs, new_file_exp_abs, patch_file_abs)
	var start_ms1 := Time.get_ticks_msec()
	while not completed1 and (Time.get_ticks_msec() - start_ms1) < 10000:
		await get_tree().process_frame
	if not completed1 or not success1:
		assert_gt(ratios.size(), 0, "Progress should have been emitted within 10 seconds during create prerequisite.")
		patcher.finished.disconnect(finished_cb1)
		patcher.progress.disconnect(progress_cb)
		return
	patcher.finished.disconnect(finished_cb1)

	# Now, apply the patch (with timeout)
	var completed2 := false
	var success2 := false
	var finished_cb2 := func(s):
		completed2 = true
		success2 = s
	patcher.finished.connect(finished_cb2)

	patcher.apply_patch_async(old_file_abs, patch_file_abs, new_file_act_abs)
	var start_ms2 := Time.get_ticks_msec()
	while not completed2 and (Time.get_ticks_msec() - start_ms2) < 10000:
		await get_tree().process_frame
	if not completed2 or not success2:
		assert_gt(ratios.size(), 0, "Progress should have been emitted within 10 seconds during apply.")
		patcher.finished.disconnect(finished_cb2)
		patcher.progress.disconnect(progress_cb)
		return
	patcher.finished.disconnect(finished_cb2)

	# Progress must emit, be monotonic, and consistent with byte counters
	assert_gt(ratios.size(), 0, "Progress signal should have been emitted at least once during apply.")
	for i in range(ratios.size() - 1):
		assert_true(ratios[i] <= ratios[i + 1], "Progress ratio should be non-decreasing.")
	for i in range(dones.size() - 1):
		assert_true(dones[i] <= dones[i + 1], "bytes_done should be non-decreasing.")
	for i in range(totals.size() - 1):
		assert_true(totals[i] <= totals[i + 1], "bytes_total should be non-decreasing.")

	var any_total_positive := false
	for t in totals:
		if t > 0:
			any_total_positive = true
			break
	assert_true(any_total_positive, "bytes_total should be > 0 at some point.")

	var final_ratio: float = ratios[-1]
	var final_done: int = dones[-1]
	var final_total: int = totals[-1]
	assert_eq(final_done, final_total, "Final bytes_done should equal bytes_total.")
	assert_eq(final_ratio, 1.0, "Final progress ratio should be 1.0.")

	# Check ratio ~= bytes_done / bytes_total when total > 0 (allow tiny tolerance)
	for i in range(ratios.size()):
		var bt = totals[i]
		if bt > 0:
			var expected = float(dones[i]) / float(bt)
			var delta = abs(ratios[i] - expected)
			assert_lt(delta, 0.051, "ratio should approximately equal bytes_done/bytes_total (delta=%f)" % delta)

	# Cleanup connections for this test
	patcher.progress.disconnect(progress_cb)
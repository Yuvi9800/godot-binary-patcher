#include "hdiff_wrapper.h"

#include "HDiffPatch/libHDiffPatch/HPatch/patch.h"
#include <godot_cpp/core/memory.hpp>

#include <iostream>
#include <memory>
#include <vector>

// HDiffPatch requires C-style file access for its streaming interface.
// We'll create thin wrappers around std::ifstream and std::ofstream.

namespace {

// Stream interface for reading input files (old file, diff file)
struct FileReadStream {
    const hpatch_TStreamInput base;
    FILE *file_handle;

    static long long read_static(const hpatch_TStreamInput *stream,
            hpatch_StreamPos_t read_pos, unsigned char *out_data,
            unsigned char *out_data_end) {
        FileReadStream *self = (FileReadStream *)stream;
        fseek(self->file_handle, read_pos, SEEK_SET);
        size_t read_bytes = fread(out_data, 1, out_data_end - out_data, self->file_handle);
        if (read_bytes != (size_t)(out_data_end - out_data) && ferror(self->file_handle)) {
            return -1; // Read error
        }
        return read_bytes;
    }
};

// Stream interface for writing the output file
struct FileWriteStream {
    const hpatch_TStreamOutput base;
    FILE *file_handle;

    static int write_static(const hpatch_TStreamOutput *stream,
            hpatch_StreamPos_t write_pos, const unsigned char *data,
            const unsigned char *data_end) {
        FileWriteStream *self = (FileWriteStream *)stream;
        fseek(self->file_handle, write_pos, SEEK_SET);
        size_t write_bytes = fwrite(data, 1, data_end - data, self->file_handle);
        if (write_bytes != (size_t)(data_end - data)) {
            return -1; // Write error
        }
        return 0; // Success
    }
};

// Listener to handle patch progress and cancellation
struct PatchListener {
    const hpatch_TStreamInput base;
    hdiff_patch_wrapper::PatchStatus &status;

    static hpatch_BOOL on_progress(struct hpatch_TStreamInput *listener,
            hpatch_StreamPos_t step_bytes) {
        PatchListener *self = (PatchListener *)listener;
        self->status.current_size += step_bytes;
        return self->status.cancel_flag ? hpatch_FALSE : hpatch_TRUE;
    }
};

} // namespace

namespace hdiff_patch_wrapper {

bool apply_patch(
        const std::string &old_file_path,
        const std::string &diff_file_path,
        const std::string &out_new_file_path,
        PatchStatus &status) {
    // 1. Open files
    FILE *old_file = fopen(old_file_path.c_str(), "rb");
    if (!old_file) {
        return false;
    }
    FILE *diff_file = fopen(diff_file_path.c_str(), "rb");
    if (!diff_file) {
        fclose(old_file);
        return false;
    }
    FILE *new_file = fopen(out_new_file_path.c_str(), "wb");
    if (!new_file) {
        fclose(old_file);
        fclose(diff_file);
        return false;
    }

    // 2. Get file sizes for streaming
    fseek(old_file, 0, SEEK_END);
    long long old_file_size = ftell(old_file);
    fseek(old_file, 0, SEEK_SET);

    fseek(diff_file, 0, SEEK_END);
    long long diff_file_size = ftell(diff_file);
    fseek(diff_file, 0, SEEK_SET);

    // 3. Set up HDiffPatch stream objects
    FileReadStream old_stream = { { &(FileReadStream::read_static) }, old_file };
    old_stream.base.streamSize = old_file_size;

    FileReadStream diff_stream = { { &(FileReadStream::read_static) }, diff_file };
    diff_stream.base.streamSize = diff_file_size;

    FileWriteStream new_stream = { { &(FileWriteStream::write_static) }, new_file };

    // 4. Get compressed info to determine total work size
    hpatch_compressedDiffInfo diff_info;
    if (!get_compressedDiffInfo(&diff_info, &diff_stream.base)) {
        fclose(old_file);
        fclose(diff_file);
        fclose(new_file);
        return false;
    }
    status.total_size = diff_info.stepCount;
    status.current_size = 0;

    // 5. Set up progress listener
    PatchListener listener = { { &(PatchListener::on_progress) }, status };
    listener.base.streamSize = 0; // Not used for listener

    // 6. Execute the patch
    hpatch_BOOL patch_result = patch_stream_with_listener(
            &new_stream.base, &old_stream.base, &diff_stream.base, &listener.base);

    // 7. Clean up
    fclose(old_file);
    fclose(diff_file);
    fclose(new_file);

    return patch_result == hpatch_TRUE;
}

} // namespace hdiff_patch_wrapper
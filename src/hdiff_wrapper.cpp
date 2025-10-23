#include "hdiff_wrapper.h"
#include "hpatch_lite.h"
#include <stdio.h>
#include <vector>

// hpatch_lite requires a listener to handle I/O. This struct holds file handles
// and implements the read/write callbacks required by the patching function.
struct FileListener {
    hpatchi_listener_t base;
    FILE* old_file;
    FILE* new_file;
    PatchStatus* status;
};

// Callback to read from the old file.
static hpi_BOOL read_old_file(hpatchi_listener_t* listener, hpi_pos_t read_from_pos, unsigned char* out_data, hpi_size_t data_size) {
    FileListener* file_listener = (FileListener*)listener;
    if (fseek(file_listener->old_file, read_from_pos, SEEK_SET) != 0) return hpi_FALSE;
    return fread(out_data, 1, data_size, file_listener->old_file) == data_size;
}

// Callback to write to the new file.
static hpi_BOOL write_new_file(hpatchi_listener_t* listener, const unsigned char* data, hpi_size_t data_size) {
    FileListener* file_listener = (FileListener*)listener;
    return fwrite(data, 1, data_size, file_listener->new_file) == data_size;
}

// Callback to read from the patch file (diff data).
static hpi_BOOL read_diff_data(hpi_TInputStreamHandle diff_data, hpi_byte* out_data, hpi_size_t* data_size) {
    FILE* patch_file = (FILE*)diff_data;
    size_t bytes_read = fread(out_data, 1, *data_size, patch_file);
    if (bytes_read == 0 && ferror(patch_file)) {
        return hpi_FALSE; // Read error
    }
    *data_size = bytes_read;
    return hpi_TRUE;
}

void apply_patch(const char* old_filename, const char* patch_filename, const char* new_filename, PatchStatus* status) {
    FILE* patch_file = fopen(patch_filename, "rb");
    if (!patch_file) {
        status->success = false;
        status->finished = true;
        return;
    }

    hpi_compressType compress_type;
    hpi_pos_t new_size;
    hpi_pos_t uncompress_size;

    // Read patch metadata to get the expected size of the new file.
    if (!hpatch_lite_open(patch_file, read_diff_data, &compress_type, &new_size, &uncompress_size)) {
        fclose(patch_file);
        status->success = false;
        status->finished = true;
        return;
    }

    FileListener listener;
    listener.base.diff_data = patch_file;
    listener.base.read_diff = read_diff_data;
    listener.base.read_old = read_old_file;
    listener.base.write_new = write_new_file;
    listener.status = status;

    listener.old_file = fopen(old_filename, "rb");
    listener.new_file = fopen(new_filename, "wb");

    bool success = false;
    if (listener.old_file && listener.new_file) {
        // The patch function requires a temporary memory buffer (cache).
        std::vector<unsigned char> temp_cache(hpi_kMinCacheSize);
        success = hpatch_lite_patch(&listener.base, new_size, temp_cache.data(), temp_cache.size());
    }

    if (listener.old_file) fclose(listener.old_file);
    if (listener.new_file) fclose(listener.new_file);
    fclose(patch_file);

    status->success = success;
    if (success) {
        status->progress.store(1.0);
    }
    status->finished = true;
}
# Godot Binary Patcher

<p align="center">
    <img width="512" height="512" alt="image" src="https://github.com/NodotProject/godot-binary-patcher/blob/main/icon.png?raw=true" />
</p>

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Discord](https://img.shields.io/discord/1089846386566111322)](https://discord.gg/Rx9CZX4sjG) [![Mastodon](https://img.shields.io/mastodon/follow/110106863700290562?domain=mastodon.gamedev.place)](https://mastodon.gamedev.place/@krazyjakee) [![Youtube](https://img.shields.io/youtube/channel/subscribers/UColWkNMgHseKyU7D1QGeoyQ)](https://www.youtube.com/@GodotNodot) [![GitHub Sponsors](https://img.shields.io/github/sponsors/krazyjakee)](https://github.com/sponsors/krazyjakee) [![GitHub Stars](https://img.shields.io/github/stars/NodotProject/godot-binary-patcher)](https://github.com/NodotProject/godot-binary-patcher)

Godot Binary Patcher is a GDExtension that integrates the powerful [HDiffPatch](https://github.com/sisong/HDiffPatch) library to provide efficient binary diffing and patching capabilities within Godot projects. This extension is ideal for implementing auto-updating systems for games and applications, allowing you to distribute only the changes between versions, significantly reducing download sizes for your users.

## Features

-   **Create Binary Patches**: Generate a compact patch file representing the difference between two files (e.g., an old and a new version of your game executable).
-   **Apply Binary Patches**: Update an old file to a new version using a patch file.
-   **Asynchronous Operations**: Both creating and applying patches are performed on a separate thread, ensuring that your game or application remains responsive without freezing the UI.
-   **Cross-Platform**: Built as a GDExtension, it can be used on any platform that Godot supports.
-   **Signal-driven**: Emits signals for progress and completion, making it easy to integrate into your existing Godot workflows.

## Getting Started

### Installation

- Copy the compiled extension files into the `addons/godot-binary-patcher` directory in your Godot project.
- As a GDExtension, this plugin does not require activation in the Godot plugins menu, the class BinaryPatcher will simply become available.

### Basic Usage

The `BinaryPatcher` node provides two main functions for creating and applying patches. Here's a simple example of how to use it in GDScript:

```gdscript
extends Node

var patcher: BinaryPatcher

func _ready():
    # Instantiate the BinaryPatcher node
    patcher = BinaryPatcher.new()
    add_child(patcher)

    # Connect to the finished signal to get the result of the operation
    patcher.finished.connect(_on_patcher_finished)
    patcher.progress.connect(_on_patcher_progress)

    # Example: Create a patch
    create_game_patch("path/to/old_version.exe", "path/to/new_version.exe", "path/to/game.patch")

    # Example: Apply a patch
    # apply_game_patch("path/to/old_version.exe", "path/to/game.patch", "path/to/updated_version.exe")


func create_game_patch(old_file_path: String, new_file_path: String, patch_output_path: String):
    print("Creating patch...")
    patcher.create_patch_async(old_file_path, new_file_path, patch_output_path)

func apply_game_patch(old_file_path: String, patch_file_path: String, new_file_output_path: String):
    print("Applying patch...")
    patcher.apply_patch_async(old_file_path, patch_file_path, new_file_output_path)

func _on_patcher_finished(success: bool):
    if success:
        print("Operation completed successfully!")
    else:
        print("Operation failed.")

func _on_patcher_progress(ratio: float, bytes_done: int, bytes_total: int):
    print("Progress: %d%% (%d/%d bytes)" % [ratio * 100, bytes_done, bytes_total])

```

## API

### Methods

-   `create_patch_async(old_file: String, new_file: String, diff_file: String)`
    -   Starts an asynchronous operation to create a patch file.
    -   `old_file`: Path to the original file.
    -   `new_file`: Path to the new version of the file.
    -   `diff_file`: Path where the generated patch file will be saved.

-   `apply_patch_async(old_file: String, patch_file: String, new_file: String)`
    -   Starts an asynchronous operation to apply a patch to a file.
    -   `old_file`: Path to the original file to be patched.
    -   `patch_file`: Path to the patch file.
    -   `new_file`: Path where the patched file will be saved.


### Signals

-   `finished(success: bool)`
    -   Emitted when a `create_patch_async` or `apply_patch_async` operation is completed.
    -   `success`: A boolean value that is `true` if the operation was successful, and `false` otherwise.

-   `progress(ratio: float, bytes_done: int, bytes_total: int)`
    -   Emitted periodically during a `create_patch_async` or `apply_patch_async` operation.
    -   `ratio`: A float value between 0.0 and 1.0 representing the progress of the operation.
    -   `bytes_done`: The number of bytes processed so far.
    -   `bytes_total`: The total number of bytes to be processed.

## Contributing

Contributions are welcome! Please feel free to open an issue or submit a pull request.

## License

This project is licensed under the MIT License. See the `LICENSE` file for details.
Please see the various submodules and their own licenses within.

## 💖 Support Me
Hi! I’m krazyjakee 🎮, creator and maintain­er of the *NodotProject* - a suite of open‑source Godot tools (e.g. Nodot, Gedis, GedisQueue etc) that empower game developers to build faster and maintain cleaner code.

I’m looking for sponsors to help sustain and grow the project: more dev time, better docs, more features, and deeper community support. Your support means more stable, polished tools used by indie makers and studios alike.

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/krazyjakee)

Every contribution helps maintain and improve this project. And encourage me to make more projects like this!

*This is optional support. The tool remains free and open-source regardless.*

---

**Created with ❤️ for Godot Developers**  
For contributions, please open PRs on GitHub

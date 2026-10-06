# WAIVE-FRONT V2

[![GitHub Actions Workflow Status](https://img.shields.io/github/actions/workflow/status/superpositioncc/waive-front-v2/doxygen-gh-pages.yml?label=docs)](https://superpositioncc.github.io/waive-front-v2) [![C++](https://img.shields.io/badge/C++-%2300599C.svg?logo=c%2B%2B&logoColor=white)](#) [![GitHub Pages](https://img.shields.io/badge/GitHub%20Pages-121013?logo=github&logoColor=white)](https://superpositioncc.github.io/waive-front-v2)

<p align="center">
	<img src="assets/viewer.png" alt="WAIVE-FRONT Viewer" width="45%"/>
	<img src="assets/parameters.png" alt="WAIVE-FRONT Parameters" width="45%"/>
</p>

<p align="center">
<em>Interactive, fully automatable visual generation software based on European digital cultural heritage archives.</em>
</p>

# Quick Start (for non-coders)

Follow these instructions to get started.

### 1. Install the WAIVE-FRONT dataset
<details>
<summary>
<i>Click to open instructions</i>
</summary>

First of all, download the footage and metadata zip from [here](https://drive.google.com/file/d/1h3WZgfrcJxJCwXs8iOBzoWD9DIgm0oJs/view?usp=sharing). Unzip it into a folder called WAIVE in your Documents folder. You should be left with this structure:

```
Users/
├─ Your Name/
│  ├─ Documents/
│  │  ├─ WAIVE/
│  │  │  ├─ categories.json
│  │  │  ├─ stichting_natuurbeelden
│  │  │  ├─ ...
```

⚠️ **Make sure these files are in the correct place, otherwise WAIVE-FRONT won't be able to find them. If you run into any problems, this is the first thing you should check.**
</details>

### 2. Install WAIVE-FRONT

<details>
<summary>
<i>Click to open instructions</i>
</summary>

Download the build for your operating system from the [releases](https://github.com/superpositioncc/waive-front-v2/releases) page. Choose which plugin format you prefer and place it in the plugins path of your DAW. On macOS, the simplest way to open these folders is by opening Finder and then pressing `cmd+shift+g`, and pasting the path from below.

Common (system-wide) plugin paths:

|         | VST2                                 | VST3                                 | Audio Units                         |
| ------- | ------------------------------------ | ------------------------------------ | ----------------------------------- |
| macOS   | `/Library/Audio/Plug-ins/VST3`        | `/Library/Audio/Plug-ins/VST3`        | `/Library/Audio/Plug-ins/Components` |
| Linux   | `/usr/lib/vst`                       | `/usr/lib/vst3`                      | _n/a_                               |
| Windows | `C:\Program Files\Common Files\VST2` | `C:\Program Files\Common Files\VST3` | _n/a_                               |

In your DAW, rescan plugins if it does not automatically.

### Communication with WAIVE

If you use both [WAIVE](https://github.com/ThunderboomRecords/WAIVE) and WAIVE-FRONT at the same time on the same computer, they should communicate out of the box if your project is playing in your DAW.

WAIVE-FRONT needs UDP port 8000 to be available, because it will listen for OSC messages there. This way, you can use   to control the visuals.
</details>

&nbsp;

🎉 That's it, you're ready to start VJ'ing!

# Build instructions (for coders)

<details>
<summary>
<i>If you're interested in developing with us, click here to open build instructions</i>
</summary>

_Note: as of yet, Linux builds have been untested and therefore disabled in CMakeLists.txt. It should be straightforward to adjust the build steps to work on Linux. Please feel free to contribute with a pull request!_

The following steps have been written with MacOS users in mind. For Windows, the easiest way to build is to load the project into Visual Studio and run CMake from there -- it should work out of the box.

WAIVE-FRONT links FFmpeg statically, so the builds run without installing anything. You don't need to install FFmpeg yourself either:

- **macOS:** CMake builds a small FFmpeg 8.1 from source (`scripts/build-ffmpeg.sh`, about a minute). You only need the Xcode command line tools. For a universal build, pass `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` and install `nasm` (`brew install nasm`); without it the Intel part decodes more slowly.
- **Windows:** without further options CMake downloads a shared FFmpeg and copies its DLLs next to the binaries. That is fine for development, but large. The release builds use a static FFmpeg made with `scripts/build-ffmpeg.sh` in MSYS2 and passed with `-DWAIVE_FFMPEG_ROOT=<prefix>`; see `.github/workflows/build.yml`.
- To link against the FFmpeg on your system instead (e.g. from Homebrew), pass `-DWAIVE_SYSTEM_FFMPEG=ON`. The binaries then only run where the same FFmpeg is installed.

1. Clone the Git repository.
   ```bash
   git clone --recursive https://github.com/superpositioncc/waive-front-v2
   cd waive-front-v2
   ```
2. Create the `build` directory and step into it.
   ```bash
   mkdir build && cd build
   ```
3. Run CMake script. Required dependencies will automatically be downloaded according to your operating system.
   ```bash
   cmake ..
   ```
4. Run the generated Makefile.
   ```bash
   make
   ```
5. Your binaries will be in the `build/bin` directory.
6. Documentation for the code can be built by running `doxygen` in the root directory of this repository.

Every push and pull request is built for macOS (universal) and Windows by `.github/workflows/build.yml`. The builds can be downloaded from the run's artifacts.

## Development

Want to add new features or improve on existing ones? Squash some bugs? Pull requests are very welcome! Documentation for the code is available [here](https://superpositioncc.github.io/waive-front-v2/).

## Shipping on MacOS

Building a fully functional and production-ready version on MacOS requires a paid Apple Developer plan.

1. Copy `Icon.icns` from the `assets` folder into the App Bundle's `Contents/Resources` folder. Create the folder if it does not exist.
2. Copy `Info.plist` from the `assets` folder into the App Bundle's `Contents` folder, overwriting the one that is already there.
3. Obtain your Team ID and an app-specific password from the Apple Developer website. Create a Developer ID Application certificate and install on your system, using XCode.
4. Run `security find-identity -p basic -v` and note the hash of the Developer ID Application certificate.
5. Navigate to the folder that contains the app.
6. Run `codesign --deep --force --options=runtime --entitlements <path_to_entitlements.plist> --sign <hash_of_certificate> --timestamp WAIVE-FRONT-V2.app` to sign the app bundle. Replace `entitlements.plist` can be found in the `assets` folder.
7. Run `zip -r WAIVE-FRONT-V2.zip WAIVE-FRONT-V2.app` to create a zip archive.
8. Run `xcrun notarytool submit WAIVE-FRONT-V2.zip --apple-id <your_apple_id_email_address> --password <your_app_specific_password> --team-id <your_team_id> --wait` to send the app to Apple for notarization.
9. If all went well, `spctl -vvv --assess --type exec WAIVE-FRONT-V2.app` should return `accepted`.
10. Your zip file is ready to ship.
</details>

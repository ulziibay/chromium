# Chromium Compilation Guide

Step-by-step instructions for checking out and compiling a custom Chromium build.

## Step 1: Install depot_tools

```bash
# Clone depot_tools if not already installed
if [ ! -d "$HOME/depot_tools" ]; then
  git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git $HOME/depot_tools
fi

# Add depot_tools to PATH
export PATH="$PATH:$HOME/depot_tools"

# Verify installation
which gclient gn ninja
```

## Step 2: Clone Source Code

Create a folder and clone the repo into subdirectory called src. This ensures that Github based way to get the code
creates the same folder structure as the `fetch` based method described in https://chromium.googlesource.com/chromium/src/+/HEAD/docs/linux/build_instructions.md#get-the-code.

```bash
mkdir
cd ~/chromium
if [ ! -d "src" ]; then
  git clone https://github.com/ulziibay/chromium.git src
fi
cd src
```

## Step 3: Install Chromium Dependencies

```bash
./build/install-build-deps.sh
```

## Step 4: Run Hooks and Sync subrepos

When using `fetch`, there are .gclient and .gclient_entries files that are created. However, because we are not using fetch,
add the following `.gclient` under `chromium` folder.

```
solutions = [
  {
    "managed": False,
    "name": "src",
    "url": "git@github.com:ulziibay/chromium.git",
    "custom_deps": {},
    "deps_file": ".DEPS.git",
    "safesync_url": "",
  },
]
```

```bash
gclient sync
```
```bash
gclient runhooks
```

## Step 5: Configure Build

```bash
gn gen out/Default --args='is_debug=false symbol_level=1'
```

**Note:** When cloning from GitHub instead of using `fetch`, you must run `gclient sync` to download all dependencies specified in the DEPS file:

```bash
gclient sync
```

This ensures all required third-party dependencies and submodules are properly checked out before building.

## Step 6: Build Chromium

```bash
ninja -C out/Default chrome -j $(nproc)
```

## Step 7: Verify Build

```bash
./out/Default/chrome --version
```

The executable is located at: `~/chromium/src/out/Default/chrome`

## Rebuilding After Modifications

```bash
cd ~/chromium/src
ninja -C out/Default chrome
```

For a clean build:

```bash
rm -rf out/Default
gn gen out/Default --args='is_debug=false symbol_level=1'
ninja -C out/Default chrome
```

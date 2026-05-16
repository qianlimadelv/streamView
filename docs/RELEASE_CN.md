# 发布说明

StreamView 目前还没有正式打包发布。本文件先定义推荐的产物结构，避免后续
Linux/macOS/Windows 打包方式不一致。

## 版本号

CLI 版本来自根目录 CMake project version：

```bash
streamview --version
```

## 产物结构

推荐归档名称：

- `streamview-<version>-linux-x86_64.tar.gz`
- `streamview-<version>-macos-arm64.tar.gz`
- `streamview-<version>-windows-x86_64.zip`

推荐内容：

```text
streamview-<version>/
  bin/
    streamview
  docs/
    CLI.md
    BUILD.md
    CODEC-SCOPE.md
  LICENSE
  README.md
```

## 发布前检查

发布前至少执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build --output-on-failure
./build/apps/streamview-cli/streamview --version
```

## 本地安装产物检查

可以用 CMake install 安装到本地目录，检查发布内容：

```bash
cmake --install build --prefix out/install
```

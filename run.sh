#!/usr/bin/env bash
set -euo pipefail

# reformat whole codebase
find src -name '*.cpp' -or -name '*.h' | while read f; do
  clang-format-11 "$f" > "$f.tmp"
  if ! diff -Naur "$f" "$f.tmp" ; then
    mv "$f.tmp" "$f"
  else
    rm "$f.tmp"
  fi
done

# update 'tags' file
ctags -R src glad /usr/include/SDL2

# build the code (C++ and shaders)
make -j`nproc`

find src -name '*.cpp' -or -name '*.h' | xargs wc -l | tail -n 1

# force validation layers
export VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation
export VK_LAYER_PATH=/home/ace/source/Vulkan-ValidationLayers/bin/layers

# run
(gdb --batch -q -ex=run --args bin/vulkanisch.exe ${1-})

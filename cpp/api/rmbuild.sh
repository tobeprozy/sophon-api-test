#!/bin/bash

current_dir="./"

# 获取当前文件夹中的所有子文件夹
dirs=$(find $current_dir -mindepth 1 -maxdepth 1 -type d)

for dir in $dirs
do
    path="$dir/build"
    if [ -d "$path" ]; then
        rm -rf "$path"
    fi
done

#! /bin/bash
scripts_dir=$(dirname $(readlink -f "$0"))
top_dir=$scripts_dir
pushd $top_dir/cpp/api

source ./rmbuild.sh

popd

pushd $top_dir

git add . 
git commit -m "update"
git push 

popd

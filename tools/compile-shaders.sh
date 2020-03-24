#/bin/bash
# This script compiles GLSL shaders to SPIR-V using glslc
# Requirements:
#  - clang-format

verts_ext=".vert"
frags_ext=".frag"
compiler=glslc

this_dir=$(pwd)
cd "$(dirname "${BASH_SOURCE[0]}")"
. ./os.sh
cd "$this_dir"

directory=$1
if [[ -z "$directory" ]]; then
	directory="."
fi
echo "== Compiling [$verts_ext] and [$frags_ext] in [$directory]..."

verts=$(ls "$directory" | grep .vert$)
frags=$(ls "$directory" | grep .frag$)

for v in $verts; do
	echo "  $v => $v.spv"
	$compiler$ext "$directory/$v" -o "$directory/$v.spv"
done
for f in $frags; do
	echo "  $f => $f.spv"
	$compiler$ext "$directory/$f" -o "$directory/$f.spv"
done

exit

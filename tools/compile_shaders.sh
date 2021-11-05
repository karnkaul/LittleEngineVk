#/bin/bash
# This script compiles GLSL shaders to SPIR-V using glslc
# Requirements:
#  - glslc

verts_ext=".vert"
frags_ext=".frag"
compiler=glslc

this_dir=$(pwd)
cd "$(dirname "${BASH_SOURCE[0]}")"
. ./os.sh
cd "$this_dir"

debug=false
if [[ "$1" == "-d" ]] || [[ "$1" == "--debug" ]]; then
  debug=true
  directory=$2
else
  directory=$1
fi
if [[ -z "$directory" ]]; then
  directory="."
fi
echo "== Compiling [$verts_ext] and [$frags_ext] in [$directory]..."

verts=$(ls "$directory" | grep .vert$)
frags=$(ls "$directory" | grep .frag$)

# src, dst, flags
compile_shader() {
  echo "  $2"
  $compiler$ext "$directory/$1" $3 -o "$directory/$2" || exit 1
}

for v in $verts; do
  compile_shader "$v" "$v.spv"
  [[ $debug == true ]] && compile_shader "$v" "$v-d.spv" "-g"
done
for f in $frags; do
  compile_shader "$f" "$f.spv"
  [[ $debug == true ]] && compile_shader "$f" "$f-d.spv" "-g"
done

exit

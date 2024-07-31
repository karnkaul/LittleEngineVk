#!/bin/bash

src=lib/glsl
dst=assets/shaders

for glsl in $(cd $src && ls | grep -e .vert$ -e .frag$); do
  spirv=$glsl.spv
  echo -e "$glsl\t => $spirv"
  glslc $src/$glsl -o $dst/$spirv || exit 1
done

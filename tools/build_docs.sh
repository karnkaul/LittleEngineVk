# This script builds docs using Doxygen and pushes the docs subtree to a git branch
# Requirements:
#  - doxygen

doxyfile=Doxyfile
docs=docs
remote=origin
branch=gh-pages

push=false
nobuild=false
clean=false
force_push=false

for arg in "$@"; do
  [ "$arg" == "-p" ] && push=true
  [ "$arg" == "-c" ] && clean=true
  [ "$arg" == "-n" ] && nobuild=true
  [ "$arg" == "-f" ] && force_push=true
done

this_dir=$(pwd)
cd "$(dirname "${BASH_SOURCE[0]}")"
. ./os.sh

cd "$this_dir/$docs"

if [ ! -f ./$doxyfile ]; then
  echo "$doxyfile not found in $(pwd)!"
  exit 1
fi

if [ "$clean" == "true" ]; then
  cd html
  rm -rf *.html *.css *.js *.png *.jpg *.svg
  cd ..
  echo "-- Cleaned ./$docs --"
fi

if [ "$nobuild" == "false" ]; then
  doxygen $doxyfile
  echo "== Built docs using ./$docs/$doxyfile =="
fi

if [ "$push" == "true" ]; then
  cd html
  git checkout $branch
  git fetch
  git add .
  push="push"
  if [[ "$force_push" == "true" ]]; then
    git commit --amend --no-edit
    push="$push --force-with-lease"
    log_msg="Amended commit and force pushed"
  else
    git commit -m "[Automated] Update docs"
    log_msg="Created a commit and pushed"
  fi
  git $push $remote $branch
  echo "== $log_msg ./$docs to $remote/$branch =="
fi

cd "$this_dir"

exit

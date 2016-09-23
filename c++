#!/bin/bash

# Get the current working directory
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

export ICECC_VERSION="x86_64:$DIR/668ddd370be90ffe3a682e6ac90331b1.tar.gz"
"$DIR/icecc/icecc" "$DIR/clang_darwin_on_darwin/bin/clang++" -mmacosx-version-min=10.11 $*
exit $?

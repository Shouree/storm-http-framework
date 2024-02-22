#!/bin/bash

# Script located on each server used to build Storm. Launches the relevant parts of the remainder of the build process.
# Not ideal to store inside the repo used to build Storm, as this script attempts to checkout the repository, and could thereby modify itself.

if [[ $# != 4 ]]
then
    echo "Usage: <arch-list> <version> <date> <hash>"
    exit 1
fi

archlist="$1"
version="$2"
date="$3"
hash="$4"

# Store other builds from STDIN.
mkdir -p ~/build
cd ~/build
cat - > storm-other.tar.gz

# Make sure the repo is cloned.
if [[ ! -e storm ]]
then
    git clone git@storm-lang.org:storm.git
    cd storm
else
    cd storm
    git fetch
fi


# Make sure it is up to date.
git checkout -f "$hash" || { echo "Hash not found. Did you forget to push?"; exit 1; }
git submodule init
git submodule update

# Use compatibility mode for now.
export STORM_USE_COMPAT=1

# Extract the archive inside the release directory.
mkdir -p release
cd release
tar xzf ~/build/storm-other.tar.gz
rm ~/build/storm-other.tar.gz
cd ..

# Now we can build! Do it inside a virtual X-session, so that we can run our tests.
xvfb-run -s "-screen 0 640x480x24" ./release.sh step "$archlist" "$version" "$date" "$hash" ~/build/storm/release/release_notes.md || { echo "Build failed. Aborting."; exit 1; }

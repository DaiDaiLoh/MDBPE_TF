#!/bin/bash

set -e
# DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
DIR=`dirname "$(readlink -f "$0")"`

echo "Generating GLAD bindings"
echo "  dir: ${DIR}"

echo ""
echo "Testing glad availability (if this fails, move to glad folder)"
python -m glad --help

echo ""
echo "Generate bindings"
for ogl in `echo 3.3 4.4 4.5 4.6`
do
  echo "  OpenGL ${ogl}"
  echo "    Core"
  python -m glad --profile core --api "gl=${ogl}" --generator c --out-path "${DIR}/core-${ogl}"
  echo "    Compatibility"
  python -m glad --profile compatibility --api "gl=${ogl}" --generator c --out-path "${DIR}/compatibility-${ogl}"
done

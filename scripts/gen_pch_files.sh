#!/usr/bin/env bash

# Get absolute folder for this script
selfFolderPath="`cd "${BASH_SOURCE[0]%/*}"; pwd -P`/" # Command to get the absolute path

echo "###############"
echo "AVDECC LIBRARY PCH FILES"
${selfFolderPath}list_includes.sh -public include "include/la/avdecc/controller/*" -private src include "src/controller/*"
echo ""

echo "###############"
echo "AVDECC CONTROLLER LIBRARY PCH FILES"
${selfFolderPath}list_includes.sh -public include/la/avdecc/controller -private src/controller include/la/avdecc/controller
echo ""

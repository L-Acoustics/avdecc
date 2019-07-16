#!/bin/bash

echo "###############"
echo "AVDECC LIBRARY PCH FILES"
./scripts/list_includes.sh -public include "include/la/avdecc/controller/*" -private src include "src/controller/*"
echo ""

echo "###############"
echo "AVDECC CONTROLLER LIBRARY PCH FILES"
./scripts/list_includes.sh -public include/la/avdecc/controller -private src/controller include/la/avdecc/controller
echo ""

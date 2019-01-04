#!/bin/bash

# Check if a .git file or folder exists (we allow submodules, thus check file and folder), as well as a root cmake file
if [[ ! -e ".git" || ! -f "CMakeLists.txt" ]]; then
	echo "ERROR: Must be run from the root folder of your project (where your main CMakeLists.txt file is)"
	exit 1
fi

function updateCopyrightYear()
{
	local filePattern="$1"
	local newYear="$2"
	
	echo "Updating Copyright for all $filePattern files"
	find . -iname "$filePattern" -not -path "./3rdparty/*" -not -path "./externals/*" -not -path "./_*" -exec sed -i {} -r -e "s/Copyright \(C\) 2016-([0-9]+), L-Acoustics and its contributors/Copyright \(C\) 2016-$newYear, L-Acoustics and its contributors/" \;
}

updateCopyrightYear "*.[chi]pp" "2019"
updateCopyrightYear "*.[ch]" "2019"
updateCopyrightYear "*.mm" "2019"
updateCopyrightYear "*.in" "2019"
updateCopyrightYear "*.md" "2019"

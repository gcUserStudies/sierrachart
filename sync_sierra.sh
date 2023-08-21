#!/bin/bash

# Sierra Chart Local Github Repo synced with github
ACS_SRC="/mnt/c/SierraChart_Source/ACS_Source/"

# List of ACS Destinations we want to sync to
ACS_DIRS=(
  /mnt/c/SierraChart/ACS_Source/
  /mnt/c/SierraChart/SierraChartInstance_2/ACS_Source/
  /mnt/c/SierraChart/SierraChartInstance_3/ACS_Source/
  /mnt/c/SierraChart/SierraChartInstance_4/ACS_Source/
  /mnt/c/SierraChart_Dev/ACS_Source/
)

# Rsync Options
# Only sync the one 3rd party header dir/hpp file along with
# specific cpp custom study files
OPTIONS=(
  -va
  --include 'gcUserStudies_*.cpp'
  --include 'nlohmann'
  --include 'json.hpp'
  --exclude '*'
)

# Loop through each dir and if destination dir exists, sync files to it
for i in "${ACS_DIRS[@]}"
do
  if [ -d "$i" ]; then
    rsync "${OPTIONS[@]}" "$ACS_SRC" "$i"
  fi
done

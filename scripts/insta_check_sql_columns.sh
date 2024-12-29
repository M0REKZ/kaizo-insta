#!/bin/bash

errors=0
while read -r header; do
	if ! grep -q "$header" src/game/server/instagib/sql_colums_all.h; then
		echo "Error: header $header is missing in src/game/server/instagib/sql_colums_all.h"
		errors="$((errors + 1))"
	fi
done < <(find . -name "sql_columns.h" | cut -c 7-)

if [ "$errors" -gt 0 ]; then
	exit 1
fi

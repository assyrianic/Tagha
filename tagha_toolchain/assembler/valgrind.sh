#!/bin/bash
cd "$(dirname "$0")"

#valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -v ./tagha_assembler `*.tasm`

for i in $(find . -name '*.tasm'); do
  valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -v ./tagha_assembler "$i"
done

# for SERIOUS debugging!
#--vgdb-error=0

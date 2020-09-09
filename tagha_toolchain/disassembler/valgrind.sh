#!/bin/bash
cd "$(dirname "$0")"

#valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -v ./tagha_disassembler `*.tasm`

for i in $(find . -name '*.tbc'); do
  valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes -v ./tagha_disassembler "$i"
done

# for SERIOUS debugging!
#--vgdb-error=0

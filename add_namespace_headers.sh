#!/usr/bin/env bash
#
# Inserts namespace tadah::core into header files after the #define guard
# and just before the final #endif line.

NAMESPACE_OPEN='namespace tadah {\nnamespace core {\n'
NAMESPACE_CLOSE='} // namespace core\n} // namespace tadah\n'
FALLBACK_LOG='namespace_insertion_headers_fallback.log'

> "$FALLBACK_LOG"

if [ "$#" -lt 1 ]; then
  echo "Usage: $0 file1.h file2.h ..."
  exit 1
fi

for file in "$@"; do
  if [[ "$file" != *.h ]]; then
    echo "Skipping $file (not .h)"
    continue
  fi

  if [ ! -f "$file" ]; then
    echo "File not found: $file"
    continue
  fi

  # Find the #ifndef and #define lines
  GUARD_START_LINE=$(grep -En '^[[:space:]]*#ifndef[[:space:]]+[A-Za-z0-9_]+_H|^[[:space:]]*#ifndef[[:space:]]+[A-Za-z0-9_]+_h' "$file" | head -n1 | cut -d: -f1)
  GUARD_DEFINE_LINE=$(grep -En '^[[:space:]]*#define[[:space:]]+[A-Za-z0-9_]+_H|^[[:space:]]*#define[[:space:]]+[A-Za-z0-9_]+_h' "$file" | head -n1 | cut -d: -f1)
  
  # Final #endif
  ENDIF_LINE=$(grep -En '^[[:space:]]*#endif' "$file" | tail -n 1 | cut -d: -f1)

  # Check that lines are valid and in ascending order
  if [ -n "$GUARD_START_LINE" ] && \
     [ -n "$GUARD_DEFINE_LINE" ] && \
     [ -n "$ENDIF_LINE" ] && \
     [ "$GUARD_START_LINE" -lt "$GUARD_DEFINE_LINE" ] && \
     [ "$GUARD_DEFINE_LINE" -lt "$ENDIF_LINE" ]; then

    # Insert namespace open after the #define line
    sed -i "${GUARD_DEFINE_LINE}a\\
${NAMESPACE_OPEN}
" "$file"

    # Insert namespace close exactly at the #endif line
    sed -i "${ENDIF_LINE}i\\
${NAMESPACE_CLOSE}
" "$file"
  else
    # Fallback - missing guard pattern or something unusual
    echo "Fallback insertion in $file" | tee -a "$FALLBACK_LOG"
    # Insert at file top
    sed -i "1i\\
${NAMESPACE_OPEN}
" "$file"
    # Insert at file end
    echo -e "\n${NAMESPACE_CLOSE}" >> "$file"
  fi
done


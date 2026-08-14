#!/usr/bin/env python3
"""Count the assessed units in doc/md3/CompletenessInventory.md.

The inventory is hand-written on purpose -- a generated checklist cannot know to look for a
feature that has no implementation anywhere, which is the whole point of that document. Its
*summary table*, however, is arithmetic over rows somebody else wrote, and that is exactly the
kind of number that goes stale the moment a section is edited without the total being revisited.
It has already happened: three sections changed status in one pass and the Implemented count
stayed where it was, leaving a table that summed correctly and described the tree incorrectly.

So the rows stay hand-written and the counting becomes reproducible. Run this, paste its table.

Counting rule, taken verbatim from the document's own Summary section:

  "a wholly-absent feature counts once; a per-surface-broken-out feature counts each surface row,
   including its 'engine' row where present"

and, also from that section, the one "Unconfirmed / likely absent" row is counted conservatively
as Not implemented while keeping its own honest wording in place.

Usage:  python3 tools/md3/count-inventory-rows.py [path]
Exits non-zero if the parts do not sum to the total, because a counter whose own arithmetic
disagrees with itself discredits both numbers.
"""

import collections
import pathlib
import re
import sys

BUCKETS = ["Implemented", "Partial", "Not implemented", "N/A (justified)"]


def classify(cell):
    """Map a Status cell to its bucket, or None when the cell is not a status."""
    text = re.sub(r"\*+", "", cell).strip().lower()
    if text.startswith("implemented"):
        return "Implemented"
    if text.startswith("partial"):
        return "Partial"
    if text.startswith("not implemented"):
        return "Not implemented"
    if text.startswith("n/a"):
        return "N/A (justified)"
    if text.startswith("unconfirmed"):
        # Documented rule: counted conservatively as Not implemented.
        return "Not implemented"
    return None


def count(markdown):
    counts = collections.Counter()
    tabled = tableless = 0

    for section in re.split(r"\n## ", markdown):
        heading = section.split("\n", 1)[0]
        if not re.match(r"^\d+\.", heading):
            continue  # not a numbered feature section

        found_row = False
        in_table = False
        for line in section.split("\n"):
            if re.match(r"^\|\s*(Item|Surface)\s*\|\s*Status", line):
                in_table = True
                continue
            if not in_table:
                continue
            if not line.startswith("|"):
                in_table = False
                continue
            if re.match(r"^\|[\s\-|]+\|?$", line):
                continue  # separator row
            cells = [c.strip() for c in line.strip().strip("|").split("|")]
            if len(cells) < 2:
                continue
            bucket = classify(cells[1])
            if bucket:
                counts[bucket] += 1
                found_row = True

        if found_row:
            tabled += 1
        else:
            # A section with no per-surface table is one wholly-absent (or wholly-partial)
            # assessed unit; its verdict is the first bold run in the section body.
            bold = re.search(r"\*\*([^*]+)", section)
            bucket = classify(bold.group(1)) if bold else None
            counts[bucket or "Not implemented"] += 1
            tableless += 1

    return counts, tabled, tableless


def main():
    path = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "doc/md3/CompletenessInventory.md")
    counts, tabled, tableless = count(path.read_text(encoding="utf-8"))
    total = sum(counts.values())

    print(f"Source: {path}")
    print(f"Sections with a per-surface table: {tabled}")
    print(f"Sections counted as one unit:      {tableless}")
    print()
    print("| Status | Count |")
    print("| --- | ---: |")
    for bucket in BUCKETS:
        if counts.get(bucket):
            print(f"| {bucket} | {counts[bucket]} |")
    print(f"| **Total rows** | **{total}** |")

    parts = sum(counts[b] for b in BUCKETS)
    if parts != total:
        print(f"\nFAIL: buckets sum to {parts} but {total} rows were counted.", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())

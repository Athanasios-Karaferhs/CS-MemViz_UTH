import os
import sys

DEFAULT = """#ifndef MALLOC_PROB_H
#define MALLOC_PROB_H

#define MALLOC_PROBABILITY 0.0
#define ANALYZED_LINES 0

#endif
"""


def score_line(line):
    if "->" in line:
        return 3
    if "node" in line.lower() or "list" in line.lower():
        return 2
    return -2


def analyze_c_file(input_file, output_h_file, start_line=1, end_line=None):
    print(f"Analyzing {input_file} lines {start_line}-{end_line or 'end'}...")
    score = 0.0
    counted_lines = 0

    with open(input_file, "r") as f:
        for i, line in enumerate(f, start=1):
            if i < start_line:
                continue
            if end_line is not None and i > end_line:
                break
            score += score_line(line)
            counted_lines += 1

    result = 0 if counted_lines == 0 else score / counted_lines
    # read the file, count existing entries and append a new one
    with open(output_h_file, "r") as f:
        file_content = f.read()

    # Count how many times MALLOC_PROBABILITY already appears in the file
    count = file_content.count("MALLOC_PROBABILITY")
    file_content = file_content.replace("#endif\n", "")
    # Rewrite the file with the old content + new numbered macro + new #endif
    with open(output_h_file, "w") as f:
        f.write(file_content)
        f.write(f"#define MALLOC_PROBABILITY_{count} {result}\n")
        f.write(f"#define ANALYZED_LINES {count} {counted_lines}\n\n")
        f.write("#endif\n")


if __name__ == "__main__":
    if len(sys.argv) >= 3:
        start = int(sys.argv[3]) if len(sys.argv) > 3 else 1
        end = int(sys.argv[4]) if len(sys.argv) > 4 else None
        analyze_c_file(sys.argv[1], sys.argv[2], start, end)
    else:
        print("Usage: python analyze.py <input.c> <output.h> [start_line] [end_line]")

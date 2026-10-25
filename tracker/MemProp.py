import os
import sys

THRESHOLD = 0.4
MIN_SCORE, MAX_SCORE = -2.0, 3.0  # range of score_line()
PROB_CSV = "malloc_prob.csv"


def score_line(line):
    if "->" in line:
        return 3
    if "node" in line.lower() or "list" in line.lower():
        return 2
    return -2


def analyze_c_file(input_file, start_line=1, end_line=None):
    score = 0.0
    counted = 0

    with open(input_file, "r") as f:
        for i, line in enumerate(f, start=1):
            if i < start_line:
                continue
            if end_line is not None and i > end_line:
                break
            score += score_line(line)
            counted += 1

    avg = 0.0 if counted == 0 else score / counted
    # map the average score from [-2, 3] to a probability in [0, 1]
    prob = (avg - MIN_SCORE) / (MAX_SCORE - MIN_SCORE)
    prob = max(0.0, min(1.0, prob))
    passed = 1 if prob > THRESHOLD else 0

    new_file = not os.path.exists(PROB_CSV)
    with open(PROB_CSV, "a") as f:
        if new_file:
            f.write("start_line,end_line,probability,passed\n")
        f.write(f"{start_line},{end_line},{prob:.4f},{passed}\n")

    print(
        f"lines {start_line}-{end_line}: prob={prob:.4f} -> {'PASS' if passed else 'SKIP'}"
    )


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python MemProp.py <input.c> [start_line] [end_line]")
        sys.exit(1)
    start = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    end = int(sys.argv[3]) if len(sys.argv) > 3 else None
    analyze_c_file(sys.argv[1], start, end)

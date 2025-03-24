import os

def run_test(name: str) -> int | None:
    test_ts = name + '.ts.txt'
    test_ltl = name + '.ltl.txt'
    test_ans = name + '.ans'
    test_out = name + '.out'

    for f in [test_ts, test_ltl, test_ans]:
        if not os.path.exists(f):
            print(f"[[Warning]]: {f} not found, skipping test")
            return None

    if os.system(f"LTL --ts {test_ts} -S --ltl {test_ltl} > {test_out}") != 0:
        os.system(f"rm {test_out}")
        print(f"[[Error]]: LTL crashed on {name.split('/')[-1]} at {test_ans}")
        return 0

    if os.system(f"diff -BZ {test_ans} {test_out} > /dev/null") != 0:
        os.system(f"rm {test_out}")
        print(f"[[Failed]]: LTL gave wrong output on {test_ts}")
        return 0

    os.system(f"rm {test_out}") # clean up
    return 1

def main():
    path = __file__
    path = path[:path.rfind('/')]

    # find all the subfolders
    results = []
    for root, _, files in os.walk(path):
        for file in files:
            if file.endswith('.ans'):
                results.append(run_test(root + '/' + file[:-4]))

    if len(results) == 0:
        print("No tests found")
        return

    num_success = sum([1 for x in results if x == 1])
    num_failed = sum([1 for x in results if x == 0])
    num_skipped = sum([1 for x in results if x is None])
    print(f"Tests passed: {num_success} / {len(results)}")
    print(f"Tests failed: {num_failed} / {len(results)}")
    print(f"Tests skipped: {num_skipped} / {len(results)}")

if __name__ == '__main__':
    main()

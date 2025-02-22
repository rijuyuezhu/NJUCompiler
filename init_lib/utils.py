import subprocess
import os
import sys

STRICT_CMP = False


def iprint(*args: str, end="\n") -> None:
    print("\033[0;36m", end="")
    print(*args, end="")
    print("\033[0m", end="")
    print(end, end="")


def sprint(*args: str, end="\n") -> None:
    print("\033[0;33m", end="")
    print(*args, end="")
    print("\033[0m", end="")
    print(end, end="")


def eprint(*args: str, end="\n") -> None:
    print("\033[0;31m", end="")
    print(*args, end="")
    print("\033[0m", end="")
    print(end, end="")


def crun(cmd: str) -> None:
    exit_code = os.system(cmd)
    if exit_code != 0:
        eprint(f"Error when running command '{cmd}'")
        sys.exit(exit_code)


def rtest(cmd: str, *, expect_fail: bool = False) -> tuple[str, str]:
    global final_run_test
    final_run_test = cmd

    handle = subprocess.Popen(
        cmd, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE
    )
    stdout, stderr = handle.communicate()
    if (handle.returncode != 0 and not expect_fail) or (
        handle.returncode == 0 and expect_fail
    ):
        eprint(
            f"{"Error" if not expect_fail else "No error"} when running command '{cmd}':"
        )
        eprint("stdout:")
        print(stdout.decode("utf-8"))
        eprint("stderr:")
        print(stderr.decode("utf-8"))
        sys.exit(handle.returncode)
    return stdout.decode("utf-8"), stderr.decode("utf-8")


def normalize(x: str) -> str:
    # remove CRLF end of file; and remove spaces end of line
    res = ""
    is_first_line = True
    for line in x.split("\n"):
        if is_first_line:
            is_first_line = False
        else:
            res += "\n"
        res += line.rstrip()
    return res.rstrip()


def tupleassert(x: tuple[str, str], y: tuple[str, str]) -> None:
    if not STRICT_CMP:
        xout, xerr = x
        yout, yerr = y
        xout = normalize(xout)
        xerr = normalize(xerr)
        yout = normalize(yout)
        yerr = normalize(yerr)
        x = (xout, xerr)
        y = (yout, yerr)
    if x != y:
        eprint("Assert fail.")
        eprint("Expected:")
        iprint("yout.")
        print(y[0])
        iprint("yerr.")
        print(y[1])
        eprint("Got:")
        iprint("xout.")
        print(x[0])
        iprint("xerr.")
        print(x[1])

        eprint("Final run test.")
        print(final_run_test)
        eprint("Stack trace.")

        import traceback

        traceback.print_stack()
        sys.exit(1)


def from_file(filepath: str) -> str:
    with open(filepath, "r") as f:
        return f.read()

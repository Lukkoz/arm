"""Plain-text robot programs: a simple subset of Kawasaki AS and CNC G-code (see PROGRAMMING.md)."""
from .parser import parse


def run_text_program(text):
    """Checks the whole program, prints the problems, and runs it only if there are no errors."""
    steps, errors, warnings = parse(text)
    for warning in warnings:
        print("Warning: " + warning)
    if errors:
        for error in errors:
            print("Error: " + error)
        print(f"Program not started: {len(errors)} error(s)")
        return
    if not steps:
        print("The program is empty")
        return

    from .runner import run_steps      # robot imports only when the program really runs
    run_steps(steps)
    print("Program done")

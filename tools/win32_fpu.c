// Runs a program with the x87 control word a Win32 process starts with, 0x27f (53-bit precision), where Linux starts
// one with 0x37f (64-bit). wibo leaves the word as it is, and mwccarm's switch statements depend on it: it groups the
// cases with a floating-point density test whose result differs between the two precisions (see docs/matching.md).
//
//     win32_fpu tools/wibo tools/mwccarm/dsi/1.1p1/mwccarm.exe ...
//
// An exec resets the control word, so the child asks to be traced, which stops it once the exec is done, and the
// parent sets the word there and lets it go. Linux x86-64 only; configure.py builds it.
#include <signal.h>
#include <stdio.h>
#include <sys/ptrace.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#define WIN32_CONTROL_WORD 0x27f

int main(int argc, char **argv) {
    struct user_fpregs_struct regs;
    int status;
    pid_t pid;

    if (argc < 2) {
        fprintf(stderr, "usage: %s program [arguments]\n", argv[0]);
        return 2;
    }

    pid = fork();
    if (pid == 0) {
        ptrace(PTRACE_TRACEME, 0, 0, 0);
        execv(argv[1], argv + 1);
        perror(argv[1]);
        _exit(127);
    }

    if (waitpid(pid, &status, 0) < 0) {
        return 1;
    }
    // The compiler would run at Linux's precision and quietly build other code, so failing to set the word is an error
    if (!WIFSTOPPED(status)) {
        fprintf(stderr, "win32_fpu: %s was not stopped after its exec\n", argv[1]);
        return WIFEXITED(status) && WEXITSTATUS(status) != 0 ? WEXITSTATUS(status) : 1;
    }
    if (ptrace(PTRACE_GETFPREGS, pid, 0, &regs) < 0) {
        perror("win32_fpu: PTRACE_GETFPREGS");
        kill(pid, SIGKILL);
        return 1;
    }
    regs.cwd = WIN32_CONTROL_WORD;
    if (ptrace(PTRACE_SETFPREGS, pid, 0, &regs) < 0 || ptrace(PTRACE_DETACH, pid, 0, 0) < 0) {
        perror("win32_fpu: PTRACE_SETFPREGS");
        kill(pid, SIGKILL);
        return 1;
    }

    if (waitpid(pid, &status, 0) < 0) {
        return 1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : 1;
}

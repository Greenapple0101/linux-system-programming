/* myshell.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <termios.h>

#define MAXLINE   1024
#define MAXARGS    128
#define MAXPIPE    10
#define MAXJOBS    16

typedef enum { FG, BG, ST } job_state;

typedef struct job_t {
    int     jid;                  /* job ID */
    pid_t   pgid;                 /* process group ID */
    job_state state;              /* FG, BG or ST */
    char    cmdline[MAXLINE];     /* command line */
} job_t;

job_t jobs[MAXJOBS];
int   next_jid = 1;

/* function prototypes */
void eval(char *cmdline);
void eval_pipe(char *cmdline);
int  parseline(char *buf, char **argv);
int  builtin_command(char **argv);
char *trim(char *s);

/* job management */
void initjobs(job_t *jobs) {
    for (int i = 0; i < MAXJOBS; i++) {
        jobs[i].jid = 0;
        jobs[i].pgid = 0;
        jobs[i].state = BG;
        jobs[i].cmdline[0] = '\0';
    }
}

int addjob(job_t *jobs, pid_t pgid, job_state state, char *cmdline) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].jid == 0) {
            jobs[i].jid = next_jid++;
            jobs[i].pgid = pgid;
            jobs[i].state = state;
            strncpy(jobs[i].cmdline, cmdline, MAXLINE-1);
            jobs[i].cmdline[MAXLINE-1] = '\0';
            return jobs[i].jid;
        }
    }
    fprintf(stderr, "myshell: too many jobs\n");
    return 0;
}

void deletejob(job_t *jobs, pid_t pgid) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].jid != 0 && jobs[i].pgid == pgid) {
            jobs[i].jid = 0;
            jobs[i].pgid = 0;
            jobs[i].state = BG;
            jobs[i].cmdline[0] = '\0';
            return;
        }
    }
}

job_t *getjob_by_jid(job_t *jobs, int jid) {
    for (int i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid == jid)
            return &jobs[i];
    return NULL;
}

job_t *getjob_by_pgid(job_t *jobs, pid_t pgid) {
    for (int i = 0; i < MAXJOBS; i++)
        if (jobs[i].pgid == pgid)
            return &jobs[i];
    return NULL;
}

job_t *fgjob(job_t *jobs) {
    for (int i = 0; i < MAXJOBS; i++)
        if (jobs[i].jid != 0 && jobs[i].state == FG)
            return &jobs[i];
    return NULL;
}

void listjobs(job_t *jobs) {
    for (int i = 0; i < MAXJOBS; i++) {
        if (jobs[i].jid != 0) {
            printf("[%d] ", jobs[i].jid);
            switch (jobs[i].state) {
            case BG: printf("Running    ");   break;
            case FG: printf("Foreground ");   break;
            case ST: printf("Stopped    ");   break;
            }
            printf("%s", jobs[i].cmdline);
        }
    }
}

/* signal handlers */
void sigchld_handler(int sig) {
    int olderrno = errno;
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG|WUNTRACED)) > 0) {
        job_t *job = getjob_by_pgid(jobs, pid);
        if (WIFSTOPPED(status)) {
            if (job) job->state = ST;
        } else {
            if (job) deletejob(jobs, job->pgid);
        }
    }
    errno = olderrno;
}

void sigint_handler(int sig) {   /* Ctrl-C */
    job_t *job = fgjob(jobs);
    if (job) kill(-job->pgid, SIGINT);
}

void sigtstp_handler(int sig) {  /* Ctrl-Z */
    job_t *job = fgjob(jobs);
    if (job) kill(-job->pgid, SIGTSTP);
}

/* trim whitespace */
char *trim(char *s) {
    char *end;
    while (*s && isspace((unsigned char)*s)) s++;
    if (*s == '\0') return s;
    end = s + strlen(s) - 1;
    while (end > s && isspace((unsigned char)*end)) *end-- = '\0';
    return s;
}

int main() {
    char cmdline[MAXLINE];
    struct sigaction sa;

    /* register signal handlers */
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGCHLD, &sa, NULL);

    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, NULL);

    initjobs(jobs);

    while (1) {
        printf("CSE4100-SP-P2> ");
        fflush(stdout);
        if (fgets(cmdline, MAXLINE, stdin) == NULL) {
            if (feof(stdin)) exit(0);
            continue;
        }
        eval(cmdline);
    }
    return 0;
}

/* eval: pipeline 여부에 따라 분기 */
void eval(char *cmdline) {
    if (strchr(cmdline, '|') != NULL) {
        eval_pipe(cmdline);
        return;
    }

    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;
    pid_t pid;

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL) return;
    if (builtin_command(argv)) return;

    if ((pid = fork()) < 0) {
        perror("fork error");
        return;
    }

    if (pid == 0) {  /* child */
        setpgid(0, 0);
        signal(SIGINT,  SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        execvp(argv[0], argv);
        fprintf(stderr, "%s: Command not found.\n", argv[0]);
        exit(1);
    }

    /* parent */
    setpgid(pid, pid);
    if (!bg) {
        addjob(jobs, pid, FG, cmdline);
        tcsetpgrp(STDIN_FILENO, pid);
        int status;
        if (waitpid(pid, &status, WUNTRACED) < 0)
            perror("waitfg error");
        tcsetpgrp(STDIN_FILENO, getpid());
        if (fgjob(jobs)) deletejob(jobs, pid);
    } else {
        int jid = addjob(jobs, pid, BG, cmdline);
        printf("[%d] %d\n", jid, pid);
    }
}

/* eval_pipe: handle pipelines */
void eval_pipe(char *cmdline) {
    char *commands[MAXPIPE];
    int ncmd = 0;
    char *tok, *copy = strdup(cmdline);
    tok = strtok(copy, "|");
    while (tok && ncmd < MAXPIPE) {
        commands[ncmd++] = trim(tok);
        tok = strtok(NULL, "|");
    }

    /* check background */
    char check[MAXLINE];
    strncpy(check, cmdline, MAXLINE);
    int bg = parseline(check, (char **)malloc(sizeof(char*)*MAXARGS));

    int in_fd = 0, pipefd[2];
    pid_t pid, pgid = 0;

    for (int i = 0; i < ncmd; i++) {
        if (i < ncmd-1 && pipe(pipefd) < 0) {
            perror("pipe error");
            free(copy);
            return;
        }
        if ((pid = fork()) < 0) {
            perror("fork error");
            free(copy);
            return;
        }
        if (pid == 0) {  /* child */
            if (in_fd) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            if (i < ncmd-1) {
                dup2(pipefd[1], STDOUT_FILENO);
                close(pipefd[0]);
                close(pipefd[1]);
            }
            if (i == 0) setpgid(0, 0);
            else        setpgid(0, pgid);

            char *argv[MAXARGS];
            parseline(commands[i], argv);
            if (argv[0] == NULL || builtin_command(argv)) exit(0);
            execvp(argv[0], argv);
            fprintf(stderr, "%s: Command not found.\n", argv[0]);
            exit(1);
        }
        /* parent */
        if (i == 0) {
            pgid = pid;
            setpgid(pid, pgid);
        } else {
            setpgid(pid, pgid);
        }
        if (in_fd) close(in_fd);
        if (i < ncmd-1) {
            close(pipefd[1]);
            in_fd = pipefd[0];
        }
    }

    if (!bg) {
        addjob(jobs, pgid, FG, cmdline);
        tcsetpgrp(STDIN_FILENO, pgid);
        int status;
        if (waitpid(-pgid, &status, WUNTRACED) < 0)
            perror("waitfg error");
        tcsetpgrp(STDIN_FILENO, getpid());
        if (fgjob(jobs)) deletejob(jobs, pgid);
    } else {
        int jid = addjob(jobs, pgid, BG, cmdline);
        printf("[%d] %d\n", jid, pgid);
    }
    free(copy);
}

/* parseline: split into argv, handle & */
int parseline(char *buf, char **argv) {
    int argc = 0, bg = 0;
    char *tok = strtok(buf, " \t\n");
    while (tok) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
    if (argc > 0 && strcmp(argv[argc-1], "&") == 0) {
        bg = 1;
        argv[--argc] = NULL;
    }
    return bg;
}

/* builtin_command: exit, cd, jobs, fg, bg, kill */
int builtin_command(char **argv) {
    if (strcmp(argv[0], "exit") == 0) {
        char *sh = getenv("SHELL");
        if (sh) {
            /* 현재 프로세스를 종료하지 않고
               로그인 셸로 대체합니다. */
            execlp(sh, sh, NULL);
            /* execlp 실패 시 그냥 나가기 */
        }
        exit(0);
    }
    if (!strcmp(argv[0], "cd")) {
        if (!argv[1]) fprintf(stderr, "cd: missing argument\n");
        else if (chdir(argv[1]) < 0) perror("cd error");
        return 1;
    }
    if (!strcmp(argv[0], "jobs")) {
        listjobs(jobs);
        return 1;
    }
    if (!strcmp(argv[0], "fg")) {
        if (!argv[1]) { fprintf(stderr, "fg: requires %%jobid\n"); return 1; }
        int jid = (argv[1][0]=='%')?atoi(&argv[1][1]):atoi(argv[1]);
        job_t *job = getjob_by_jid(jobs, jid);
        if (!job) { fprintf(stderr, "fg: no such job\n"); return 1; }
        job->state = FG;
        tcsetpgrp(STDIN_FILENO, job->pgid);
        kill(-job->pgid, SIGCONT);
        waitpid(-job->pgid, NULL, WUNTRACED);
        tcsetpgrp(STDIN_FILENO, getpid());
        deletejob(jobs, job->pgid);
        return 1;
    }
    if (!strcmp(argv[0], "bg")) {
        if (!argv[1]) { fprintf(stderr, "bg: requires %%jobid\n"); return 1; }
        int jid = (argv[1][0]=='%')?atoi(&argv[1][1]):atoi(argv[1]);
        job_t *job = getjob_by_jid(jobs, jid);
        if (!job) { fprintf(stderr, "bg: no such job\n"); return 1; }
        job->state = BG;
        kill(-job->pgid, SIGCONT);
        return 1;
    }
    if (!strcmp(argv[0], "kill")) {
        if (!argv[1]) { fprintf(stderr, "kill: requires %%jobid\n"); return 1; }
        int jid = (argv[1][0]=='%')?atoi(&argv[1][1]):atoi(argv[1]);
        job_t *job = getjob_by_jid(jobs, jid);
        if (!job) { fprintf(stderr, "kill: no such job\n"); return 1; }
        kill(-job->pgid, SIGTERM);
        return 1;
    }
    if (!strcmp(argv[0], "&")) return 1;  /* ignore singleton & */
    return 0;  /* not a builtin */
}

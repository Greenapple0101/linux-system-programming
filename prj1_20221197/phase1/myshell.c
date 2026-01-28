/* myshell.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MAXLINE 1024
#define MAXARGS 128

/* 함수 프로토타입 */
void eval(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

int main() {
    char cmdline[MAXLINE];

    while (1) {
        /* 프롬프트 출력 */
        printf("CSE4100-SP-P2> ");
        fflush(stdout);

        /* 입력 읽기 */
        if (fgets(cmdline, MAXLINE, stdin) == NULL) {
            if (feof(stdin)) { /* Ctrl-D */
                printf("\n");
                exit(0);
            }
            continue;
        }

        /* 평가 */
        eval(cmdline);
    }
    return 0; /* 절대 도달하지 않음 */
}

/* eval: 명령어 파싱 후 내장/외부 명령어 처리 */
void eval(char *cmdline) {
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;      /* 백그라운드 여부 */
    pid_t pid;   /* 자식 PID */

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)  /* 빈 줄 */
        return;

    /* 내장 명령어 처리 */
    if (builtin_command(argv))
        return;

    /* 외부 명령어 실행 */
    if ((pid = fork()) < 0) {
        perror("fork error");
        return;
    }

    if (pid == 0) {  /* 자식 프로세스 */
        if (execvp(argv[0], argv) < 0) {
            fprintf(stderr, "%s: Command not found.\n", argv[0]);
            exit(1);
        }
    }

    /* 부모 프로세스: 포그라운드라면 대기 */
    if (!bg) {
        int status;
        if (waitpid(pid, &status, 0) < 0)
            perror("waitpid error");
    } else {
        /* 백그라운드면 PID 출력 */
        printf("[%d] %s", pid, cmdline);
    }
}

/* builtin_command: 내장 명령어면 처리 후 1, 아니면 0 리턴 */
int builtin_command(char **argv) {
    /* exit */
    if (strcmp(argv[0], "exit") == 0) {
        exit(0);
    }
    /* cd */
    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else if (chdir(argv[1]) < 0) {
            perror("cd error");
        }
        return 1;
    }
    /* singleton & 무시 */
    if (strcmp(argv[0], "&") == 0)
        return 1;

    return 0; /* 내장 명령어 아님 */
}

/* parseline: buf를 공백 기준으로 쪼개 argv 배열에 채움. bg 플래그 반환 */
int parseline(char *buf, char **argv) {
    char *delim;
    int argc = 0;
    int bg;

    /* 마지막 개행문자를 공백으로 */
    buf[strlen(buf) - 1] = ' ';

    /* 선행 공백 건너뛰기 */
    while (*buf && (*buf == ' '))
        buf++;

    /* argv 채우기 */
    while ((delim = strchr(buf, ' '))) {
        argv[argc++] = buf;
        *delim = '\0';
        buf = delim + 1;
        while (*buf && (*buf == ' '))
            buf++;
    }
    argv[argc] = NULL;

    if (argc == 0)  /* 빈 줄 */
        return 1;

    /* 백그라운드(&) 검사 */
    if ((bg = (strcmp(argv[argc - 1], "&") == 0)) != 0)
        argv[--argc] = NULL;

    return bg;
}

/* myshell.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

#define MAXLINE 1024
#define MAXARGS 128
#define MAXPIPE 10  // 파이프를 사용하는 최대 명령어 개수

/* 함수 프로토타입 */
void eval(char *cmdline);
void eval_pipe(char *cmdline);
int parseline(char *buf, char **argv);
int builtin_command(char **argv);

/* trim: 문자열의 앞뒤 공백, 탭, 개행 제거 */
static char *trim(char *s) {
    char *end;
    // 앞쪽 공백 제거
    while (*s == ' ' || *s == '\t' || *s == '\n')
        s++;
    if (*s == '\0')  // 빈 문자열이면 그대로 반환
        return s;
    // 뒤쪽 공백 제거
    end = s + strlen(s) - 1;
    while (end > s && (*end == ' ' || *end == '\t' || *end == '\n'))
        *end-- = '\0';
    return s;
}

int main() {
    char cmdline[MAXLINE];

    while (1) {
        /* 셸 프롬프트 출력 */
        printf("CSE4100-SP-P2> ");
        fflush(stdout);

        /* 표준 입력으로부터 명령어 읽기 */
        if (fgets(cmdline, MAXLINE, stdin) == NULL) {
            if (feof(stdin)) {  // Ctrl-D 입력 시 종료
                printf("\n");
                exit(0);
            }
            continue;
        }

        /* 평가: 파이프(|)가 포함되어 있으면 파이프 처리 함수 호출 */
        eval(cmdline);
    }
    return 0;
}

/* eval: 명령어 라인을 파이프 존재 여부에 따라 처리 */
void eval(char *cmdline) {
    // 파이프(|)가 있으면 파이프 처리 함수 호출
    if (strchr(cmdline, '|') != NULL) {
        eval_pipe(cmdline);
        return;
    }

    // 파이프가 없으면 기존 방식으로 처리
    char *argv[MAXARGS];
    char buf[MAXLINE];
    int bg;  // 백그라운드 실행 여부
    pid_t pid;

    strcpy(buf, cmdline);
    bg = parseline(buf, argv);
    if (argv[0] == NULL)
        return;

    if (builtin_command(argv))
        return;

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

    /* 부모 프로세스: 포그라운드 실행 시 자식 종료까지 대기 */
    if (!bg) {
        int status;
        if (waitpid(pid, &status, 0) < 0)
            perror("waitpid error");
    } else {
        printf("[%d] %s", pid, cmdline);
    }
}

/* eval_pipe: 파이프를 포함한 명령어 라인 처리 함수
   - 파이프(|)를 구분자로 명령어를 분리한 후, 각 명령어에 대해 새로운 프로세스를 생성하고,
     앞 프로세스의 출력을 다음 프로세스의 입력으로 연결합니다.
*/
void eval_pipe(char *cmdline) {
    char *commands[MAXPIPE];
    int num_commands = 0;
    char *token;
    char *cmdline_copy = strdup(cmdline);  // 원본 보존을 위해 복사
    if (!cmdline_copy) {
        perror("strdup error");
        return;
    }

    // '|' 문자를 구분자로 명령어 분리 후, trim()으로 앞뒤 공백 제거
    token = strtok(cmdline_copy, "|");
    while (token != NULL && num_commands < MAXPIPE) {
        commands[num_commands++] = trim(token);
        token = strtok(NULL, "|");
    }

    int in_fd = 0;  // 첫 번째 명령어의 입력은 표준 입력(STDIN)
    int pipefd[2];
    pid_t pid;

    for (int i = 0; i < num_commands; i++) {
        // 마지막 명령어가 아니라면 새로운 파이프 생성
        if (i < num_commands - 1) {
            if (pipe(pipefd) < 0) {
                perror("pipe error");
                free(cmdline_copy);
                return;
            }
        }

        if ((pid = fork()) < 0) {
            perror("fork error");
            free(cmdline_copy);
            return;
        }

        if (pid == 0) {  /* 자식 프로세스 */
            // 이전 파이프의 읽기 엔드가 있다면 이를 표준 입력(STDIN)으로 설정
            if (in_fd != 0) {
                if (dup2(in_fd, STDIN_FILENO) < 0) {
                    perror("dup2 error");
                    exit(1);
                }
                close(in_fd);
            }
            // 현재 명령어가 마지막이 아니라면, 표준 출력을 파이프의 쓰기 엔드로 리다이렉션
            if (i < num_commands - 1) {
                if (dup2(pipefd[1], STDOUT_FILENO) < 0) {
                    perror("dup2 error");
                    exit(1);
                }
                close(pipefd[0]);
                close(pipefd[1]);
            }
            // 명령어 토큰 분리: parseline() 함수는 strtok()를 이용하여 분리합니다.
            char *argv[MAXARGS];
            int bg = parseline(commands[i], argv);
            if (argv[0] == NULL || builtin_command(argv))
                exit(0);
            execvp(argv[0], argv);
            fprintf(stderr, "%s: Command not found.\n", argv[0]);
            exit(1);
        } else {  /* 부모 프로세스 */
            // 이전 파이프의 읽기 엔드 닫기
            if (in_fd != 0)
                close(in_fd);
            // 마지막 명령어가 아니라면 pipefd[0]을 다음 명령어의 입력으로 사용
            if (i < num_commands - 1) {
                close(pipefd[1]);
                in_fd = pipefd[0];
            }
        }
    }

    // 부모는 모든 자식 프로세스가 종료될 때까지 대기
    for (int i = 0; i < num_commands; i++) {
        wait(NULL);
    }
    free(cmdline_copy);
}

/* parseline: 입력 버퍼를 공백, 탭, 개행 문자를 기준으로 분리하여 argv 배열에 저장
   - 백그라운드 실행 여부(&)도 처리합니다.
*/
int parseline(char *buf, char **argv) {
    int argc = 0;
    int bg = 0;
    char *token = strtok(buf, " \t\n");
    while (token != NULL) {
        argv[argc++] = token;
        token = strtok(NULL, " \t\n");
    }
    argv[argc] = NULL;
    if (argc == 0)
        return bg;
    if (strcmp(argv[argc - 1], "&") == 0) {
        bg = 1;
        argv[--argc] = NULL;
    }
    return bg;
}

/* builtin_command: 내장 명령어(exit, cd, & 등)를 처리합니다.
   - 파이프 외부에서만 사용하도록 합니다.
*/
int builtin_command(char **argv) {
    if (strcmp(argv[0], "exit") == 0)
        exit(0);
    if (strcmp(argv[0], "cd") == 0) {
        if (argv[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else if (chdir(argv[1]) < 0) {
            perror("cd error");
        }
        return 1;
    }
    if (strcmp(argv[0], "&") == 0)
        return 1;
    return 0;
}
# Phase 1: Building and Testing Your Shell

이 단계에서는 리눅스의 기본 셸 구조를 처음부터 직접 구현했습니다.  
목표는 간단한 명령어를 읽고, fork-exec을 통해 자식 프로세스에서 명령을 실행하는 구조를 만드는 것이었습니다.

## 구현한 기능

- `fork()`를 사용하여 자식 프로세스를 생성
- `execvp()`로 자식 프로세스에서 명령 실행
- `cd`, `exit`은 셸 자체에서 직접 처리 (built-in)
- `waitpid()`로 자식 프로세스가 종료될 때까지 대기
- 명령 프롬프트 출력 (`CSE4100-SP-P2>`)
- 잘못된 명령어 입력 시 에러 메시지 출력

## 내부 구조

```c
while (1) {
    print_prompt();
    input = read_line();
    args = parse(input);
    if (!builtin(args))
        fork_and_exec(args);
}

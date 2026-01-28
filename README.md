# CSE4100 System Programming Projects

본 저장소는 시스템 프로그래밍 강의의 프로젝트 과제들을 포함합니다. 각 프로젝트는 리눅스 시스템 프로그래밍의 핵심 개념을 실습하며 구현한 결과물입니다.

## 프로젝트 정보

- **학번**: 20221197
- **이름**: 백서연
- **이메일**: yorange50@gmail.com

## 프로젝트 구조

```
cse4100/
├── prj1_20221197/          # 쉘 구현 프로젝트
│   ├── document_20221197.pdf
│   ├── phase1/              # 기본 쉘 구현
│   ├── phase2/              # 리다이렉션 및 파이프
│   └── phase3/              # 백그라운드 실행 및 Job Control
├── prj2_20221197/          # 자료구조 라이브러리
│   ├── document_20221197.docx
│   └── [소스 파일들]
├── prj3_20221197/          # 네트워크 프로그래밍
│   ├── document.pdf
│   ├── task_1/              # Stock Server/Client (select 기반)
│   └── task_2/              # Stock Server/Client (개선 버전)
└── prj4_20221197/          # 메모리 할당기 구현
    ├── document.pdf
    └── mm.c
```

---

## Project 1: 쉘 구현 (Shell Implementation)

리눅스의 기본 쉘을 단계적으로 구현한 프로젝트입니다. 프로세스 생성, 파이프, 리다이렉션, 백그라운드 실행 등 쉘의 핵심 기능을 직접 구현합니다.

### Phase 1: 기본 쉘 구현

**목표**: 기본적인 명령어 실행 기능을 가진 쉘 구현

**구현 기능**:
- 명령어 입력 및 파싱
- `fork()`를 통한 자식 프로세스 생성
- `execvp()`를 통한 명령어 실행
- `waitpid()`를 통한 자식 프로세스 대기
- 내장 명령어 처리 (`cd`, `exit`)
- 명령 프롬프트 출력 (`CSE4100-SP-P2>`)
- 에러 처리 및 메시지 출력

**주요 파일**:
- `myshell.c`: 메인 쉘 구현
- `csapp.c`, `csapp.h`: CS:APP 교재의 유틸리티 함수들
- `Makefile`: 빌드 설정

**컴파일 및 실행**:
```bash
cd prj1_20221197/phase1
make
./myshell
```

**학습 내용**:
- 프로세스 생성 및 관리 (`fork`, `exec`, `wait`)
- 프로세스 간 통신의 기본 개념
- 쉘의 기본 동작 원리

---

### Phase 2: 리다이렉션 및 파이프

**목표**: 리다이렉션(`>`, `<`)과 파이프(`|`) 기능 추가

**구현 기능**:
- 출력 리다이렉션 (`command > file`)
- 입력 리다이렉션 (`command < file`)
- 파이프 연결 (`command1 | command2`)
- 다중 파이프 지원 (`command1 | command2 | command3`)
- `pipe()` 시스템 콜을 통한 프로세스 간 통신
- `dup2()`를 통한 파일 디스크립터 재지정

**주요 구현 사항**:
- 파이프 단위로 명령어 분리 및 파싱
- 각 명령어마다 별도의 프로세스 생성
- 파이프를 통한 표준 입력/출력 연결
- 프로세스 그룹 관리
- 파이프 파일 디스크립터의 적절한 닫기 처리

**컴파일 및 실행**:
```bash
cd prj1_20221197/phase2
make
./myshell
```

**테스트 예시**:
```bash
ls | grep test
cat file.txt | grep keyword | sort
echo "hello" > output.txt
cat < input.txt
```

**학습 내용**:
- 파이프를 통한 프로세스 간 데이터 전달
- 파일 디스크립터의 개념과 재지정
- 다중 프로세스 협업 구조
- 데드락 방지를 위한 파일 디스크립터 관리

---

### Phase 3: 백그라운드 실행 및 Job Control

**목표**: 백그라운드 실행과 Job Control 기능 구현

**구현 기능**:
- 백그라운드 실행 (`command &`)
- `jobs`: 현재 백그라운드 및 중단된 작업 목록 출력
- `fg %n`: Job 번호 n을 포그라운드로 전환
- `bg %n`: 중단된 Job을 백그라운드로 재실행
- `kill %n`: 특정 Job 종료
- 시그널 처리 (`SIGINT`, `SIGTSTP`, `SIGCONT`, `SIGCHLD`)
- 프로세스 그룹 및 제어 터미널 관리
- Job 상태 관리 (Running, Stopped, Terminated)

**주요 구현 사항**:
- Job 테이블을 구조체 배열로 구현
- `&` 토큰 인식 및 백그라운드 실행 플래그 설정
- `addjob()`, `deletejob()` 함수를 통한 Job 리스트 관리
- `sigchld_handler()`에서 자식 프로세스 상태 변화 감지
- `tcsetpgrp()`를 통한 포그라운드 프로세스 그룹 제어
- 시그널 핸들러를 통한 Job 상태 업데이트

**컴파일 및 실행**:
```bash
cd prj1_20221197/phase3
make
./myshell
```

**테스트 예시**:
```bash
sleep 10 &
jobs
fg %1
# Ctrl+Z로 중단
bg %1
kill %1
```

**학습 내용**:
- Job Control의 개념과 구현 방법
- 프로세스 그룹과 제어 터미널
- 시그널 처리 및 비동기 이벤트 관리
- 리눅스 쉘의 핵심 기능 구현

---

## Project 2: 자료구조 라이브러리 (Data Structure Library)

다양한 자료구조를 구현한 라이브러리 프로젝트입니다. 실제 시스템 프로그래밍에서 사용되는 자료구조들을 직접 구현합니다.

### 구현된 자료구조

**1. List (연결 리스트)**
- `list.h`, `list.c`: 양방향 연결 리스트 구현
- 리스트 요소 삽입, 삭제, 순회 기능
- 매크로를 통한 타입 독립적 구현

**2. Hash Table (해시 테이블)**
- `hash.h`, `hash.c`: 해시 테이블 구현
- 체이닝 방식의 충돌 해결
- 해시 함수 및 버킷 관리

**3. Bitmap (비트맵)**
- `bitmap.h`, `bitmap.c`: 비트맵 자료구조 구현
- 비트 단위 연산 및 관리
- 메모리 효율적인 집합 표현

**4. Debug Utilities (디버그 유틸리티)**
- `debug.h`, `debug.c`: 디버깅을 위한 유틸리티 함수
- 메모리 덤프 및 상태 출력

**5. Hex Dump (16진수 덤프)**
- `hex_dump.h`, `hex_dump.c`: 메모리 내용을 16진수로 출력
- 디버깅 및 메모리 분석에 유용

### 주요 파일

- `main.c`: 자료구조 테스트 프로그램
- `Makefile`: 빌드 설정
- 각 자료구조별 헤더 및 구현 파일

### 컴파일 및 실행

```bash
cd prj2_20221197
make
./testlib
```

### 학습 내용

- 타입 독립적 자료구조 설계
- 매크로를 활용한 제네릭 프로그래밍
- 메모리 관리 및 효율성
- 자료구조의 실제 활용

---

## Project 3: 네트워크 프로그래밍 (Network Programming)

네트워크를 통한 클라이언트-서버 통신을 구현한 프로젝트입니다. Stock Server와 Client를 구현하여 동시성 처리와 네트워크 프로그래밍을 학습합니다.

### Task 1: Stock Server/Client (select 기반)

**목표**: `select()` 시스템 콜을 사용한 동시성 처리

**구현 기능**:
- Stock Server: 주식 정보를 관리하는 서버
- Stock Client: 서버에 연결하여 주식 정보 조회 및 거래
- `select()`를 통한 다중 클라이언트 처리
- 이진 탐색 트리(BST)를 사용한 주식 데이터 관리
- 클라이언트 풀(Pool) 관리

**주요 파일**:
- `stockserver.c`: Stock Server 구현
- `stockclient.c`: Stock Client 구현
- `echo.c`: 에코 서버 예제
- `multiclient.c`: 다중 클라이언트 테스트 프로그램
- `stock.txt`: 초기 주식 데이터

**컴파일 및 실행**:
```bash
cd prj3_20221197/task_1
make

# 터미널 1: 서버 실행
./stockserver 8080

# 터미널 2: 클라이언트 실행
./stockclient localhost 8080
```

**주요 구현 사항**:
- `select()`를 통한 I/O 다중화
- 파일 디스크립터 집합 관리
- 클라이언트 연결 풀 관리
- 이진 탐색 트리를 사용한 주식 데이터 구조
- 요청 파싱 및 응답 생성

**학습 내용**:
- 네트워크 소켓 프로그래밍
- I/O 다중화 및 동시성 처리
- 클라이언트-서버 아키텍처
- 이벤트 기반 프로그래밍

---

### Task 2: Stock Server/Client (개선 버전)

**목표**: Task 1의 개선 및 최적화

**구현 기능**:
- Task 1의 기능을 기반으로 개선된 버전
- 성능 최적화 및 버그 수정
- 추가 기능 구현 (필요 시)

**컴파일 및 실행**:
```bash
cd prj3_20221197/task_2
make
./stockserver 8080
./stockclient localhost 8080
```

---

## Project 4: 메모리 할당기 구현 (Memory Allocator)

동적 메모리 할당 함수(`malloc`, `free`, `realloc`)를 직접 구현한 프로젝트입니다. 힙 메모리 관리의 핵심 개념을 학습합니다.

### 구현 기능

**메모리 할당 함수**:
- `mm_malloc()`: 메모리 할당
- `mm_free()`: 메모리 해제
- `mm_realloc()`: 메모리 재할당

**구현 방식**:
- 명시적 자유 리스트(Explicit Free List) 사용
- First Fit 또는 Best Fit 할당 전략
- 인접한 자유 블록 병합(Coalescing)
- 힙 확장(Heap Extension) 기능

**주요 구현 사항**:
- 블록 헤더 및 푸터를 통한 메타데이터 관리
- 자유 블록 리스트 관리
- 블록 분할(Splitting) 및 병합(Coalescing)
- 정렬(Alignment) 처리
- 힙 초기화 및 확장

**주요 파일**:
- `mm.c`: 메모리 할당기 구현
- `mm.h`: 인터페이스 정의 (제공됨)
- `memlib.c`: 메모리 시스템 인터페이스 (제공됨)

**컴파일 및 테스트**:
```bash
cd prj4_20221197
# 제공된 Makefile 사용 (일반적으로 별도 제공)
make
./mdriver  # 메모리 할당기 테스트 드라이버
```

**성능 지표**:
- 할당 효율성 (Utilization)
- 처리량 (Throughput)
- 메모리 단편화 최소화

**학습 내용**:
- 힙 메모리 관리 구조
- 동적 메모리 할당 알고리즘
- 메모리 단편화 문제 및 해결
- 시스템 프로그래밍의 메모리 관리

---

## 공통 사항

### 개발 환경

- **운영체제**: Linux
- **컴파일러**: GCC
- **언어**: C
- **빌드 도구**: Make

### 컴파일 옵션

대부분의 프로젝트에서 다음 컴파일 옵션을 사용합니다:
- `-Wall`: 모든 경고 메시지 출력
- `-O2`: 최적화 레벨 2
- `-lpthread`: 스레드 라이브러리 링크 (필요 시)

### 빌드 및 실행

각 프로젝트 디렉토리에는 `Makefile`이 포함되어 있습니다:

```bash
# 빌드
make

# 정리
make clean

# 실행
./[실행파일명]
```

### 문서

각 프로젝트에는 상세한 설명이 포함된 문서가 있습니다:
- `prj1_20221197/document_20221197.pdf`
- `prj2_20221197/document_20221197.docx`
- `prj3_20221197/document.pdf`
- `prj4_20221197/document.pdf`

---

## 주요 학습 내용 요약

### 시스템 프로그래밍 핵심 개념

1. **프로세스 관리**
   - 프로세스 생성 및 종료
   - 프로세스 간 통신 (파이프, 시그널)
   - Job Control 및 프로세스 그룹

2. **파일 시스템**
   - 파일 디스크립터 관리
   - 리다이렉션 및 파이프
   - 파일 I/O 작업

3. **네트워크 프로그래밍**
   - 소켓 프로그래밍
   - 클라이언트-서버 모델
   - I/O 다중화

4. **메모리 관리**
   - 동적 메모리 할당
   - 힙 관리 알고리즘
   - 메모리 효율성

5. **동시성 처리**
   - 프로세스 기반 동시성
   - I/O 다중화
   - 이벤트 기반 프로그래밍

---

## 참고 자료

- CS:APP (Computer Systems: A Programmer's Perspective) 3rd Edition
- Linux 시스템 콜 매뉴얼 (`man` 페이지)
- 각 프로젝트별 제공 문서

---

## 라이선스

본 프로젝트는 교육 목적으로 작성되었습니다.

---

## 연락처

프로젝트에 대한 문의사항이 있으시면 다음으로 연락해주세요:
- 이메일: yorange50@gmail.com

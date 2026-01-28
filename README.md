# System Programming Portfolio

리눅스 시스템 프로그래밍의 핵심 개념을 구현한 프로젝트 모음입니다. 쉘 구현, 자료구조 라이브러리, 네트워크 프로그래밍, 메모리 할당기 등 시스템 레벨 프로그래밍 기술을 다룹니다.

## About

- **Author**: 백서연
- **Contact**: yorange50@gmail.com

## 프로젝트 구조

```
.
├── prj1_20221197/          # Unix Shell Implementation
│   ├── phase1/              # Basic Shell
│   ├── phase2/              # Redirection and Pipe
│   └── phase3/              # Background Execution and Job Control
├── prj2_20221197/          # Data Structure Library
│   ├── list.c, list.h
│   ├── hash.c, hash.h
│   ├── bitmap.c, bitmap.h
│   └── main.c
├── prj3_20221197/          # Network Programming
│   ├── task_1/              # Stock Server/Client (select-based)
│   └── task_2/              # Enhanced Version
└── prj4_20221197/          # Memory Allocator
    └── mm.c
```

---

## Project 1: Unix Shell Implementation

리눅스 쉘의 핵심 기능을 구현한 프로젝트입니다. 프로세스 관리, 파이프, 리다이렉션, 백그라운드 실행, Job Control 등 실제 쉘과 동일한 기능을 제공합니다.

### Phase 1: Basic Shell

**구현 내용**: 기본 명령어 실행 기능을 가진 쉘 구현

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
- `csapp.c`, `csapp.h`: 시스템 프로그래밍 유틸리티 함수
- `Makefile`: 빌드 설정

**컴파일 및 실행**:
```bash
cd prj1_20221197/phase1
make
./myshell
```

**사용 예시**:
```bash
$ ls -la
$ cd /home/user
$ exit
```

**기술 스택**:
- 프로세스 생성 및 관리 (`fork`, `exec`, `wait`)
- 프로세스 간 통신
- 시스템 콜 활용

---

### Phase 2: Redirection and Pipe

**구현 내용**: 리다이렉션(`>`, `<`)과 파이프(`|`) 기능 구현

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

**사용 예시**:
```bash
ls | grep test
cat file.txt | grep keyword | sort
echo "hello" > output.txt
cat < input.txt
```

**기술 스택**:
- 파이프를 통한 프로세스 간 데이터 전달
- 파일 디스크립터 재지정 (`dup2`)
- 다중 프로세스 협업 구조
- 데드락 방지를 위한 파일 디스크립터 관리

---

### Phase 3: Background Execution and Job Control

**구현 내용**: 백그라운드 실행과 Job Control 기능 구현

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

**사용 예시**:
```bash
sleep 10 &
jobs
fg %1
# Ctrl+Z로 중단
bg %1
kill %1
```

**기술 스택**:
- Job Control 구현
- 프로세스 그룹 및 제어 터미널 관리
- 시그널 처리 (`SIGINT`, `SIGTSTP`, `SIGCONT`, `SIGCHLD`)
- 비동기 이벤트 처리

**실무 활용 사례**:
- **Docker 컨테이너**: 컨테이너 내부에서 명령어 실행 및 프로세스 관리
- **CI/CD 파이프라인**: Jenkins, GitHub Actions 등에서 빌드 스크립트 실행
- **서버 관리 도구**: Ansible, Puppet 등에서 원격 명령 실행 및 프로세스 제어
- **임베디드 시스템**: 제한된 환경에서의 커스텀 쉘 구현
- **터미널 에뮬레이터**: iTerm2, Windows Terminal 등에서 명령어 실행 환경 제공

---

## Project 2: Data Structure Library

시스템 프로그래밍에서 널리 사용되는 자료구조를 구현한 라이브러리입니다. 타입 독립적 설계와 메모리 효율성을 고려하여 구현했습니다.

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

**주요 API**:
- `list_init()`, `list_insert()`, `list_remove()`: 리스트 연산
- `hash_insert()`, `hash_find()`, `hash_delete()`: 해시 테이블 연산
- `bitmap_set()`, `bitmap_get()`, `bitmap_reset()`: 비트맵 연산

### 기술 스택

- 타입 독립적 자료구조 설계
- 매크로를 활용한 제네릭 프로그래밍
- 메모리 효율적 구현
- 실무 적용 가능한 자료구조

**실무 활용 사례**:
- **운영체제 커널**: Linux 커널의 리스트, 해시 테이블 (예: `list.h`, `hlist`)
- **데이터베이스**: 인덱스 구조 (B-tree, 해시 인덱스), 테이블 관리
- **네트워크 스택**: 라우팅 테이블, 연결 관리, 패킷 큐잉
- **캐시 시스템**: Redis, Memcached의 내부 자료구조 (해시 테이블, 리스트)
- **파일 시스템**: inode 관리, 디렉토리 구조, 블록 할당 (비트맵)

---

## Project 3: Network Programming - Concurrent Server

네트워크 기반 클라이언트-서버 통신을 구현한 프로젝트입니다. `select()`를 활용한 I/O 다중화로 다중 클라이언트를 동시에 처리하는 서버를 구현했습니다.

### Implementation: Stock Trading Server (select-based)

**구현 내용**: `select()` 시스템 콜을 사용한 동시성 처리

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

**주요 기능**:
- 실시간 주식 정보 조회
- 주식 거래 (구매/판매)
- 다중 클라이언트 동시 처리
- 이진 탐색 트리를 사용한 효율적인 데이터 관리

**주요 구현 사항**:
- `select()`를 통한 I/O 다중화
- 파일 디스크립터 집합 관리
- 클라이언트 연결 풀 관리
- 이진 탐색 트리를 사용한 주식 데이터 구조
- 요청 파싱 및 응답 생성

**기술 스택**:
- 네트워크 소켓 프로그래밍
- I/O 다중화 (`select()`)
- 동시성 처리
- 이벤트 기반 아키텍처

**실무 활용 사례**:
- **웹 서버**: Nginx, Apache의 동시 연결 처리 및 요청 라우팅
- **API 서버**: REST API, GraphQL 서버의 다중 클라이언트 처리
- **실시간 서비스**: 채팅 서버 (Slack, Discord), 주식 거래 시스템, 게임 서버
- **IoT 서버**: 센서 데이터 수집 서버, 디바이스 관리 서버
- **로드 밸런서**: 트래픽 분산 및 다중 서버 관리
- **마이크로서비스**: 서비스 간 통신 및 이벤트 기반 아키텍처

---

### Optimization: Enhanced Version

**구현 내용**: 초기 구현의 성능 최적화 및 안정성 개선

**개선 사항**:
- 성능 최적화
- 버그 수정 및 안정성 향상
- 코드 리팩토링

**컴파일 및 실행**:
```bash
cd prj3_20221197/task_2
make
./stockserver 8080
./stockclient localhost 8080
```

**개선 사항**:
- 성능 최적화
- 메모리 관리 개선
- 에러 처리 강화

---

## Project 4: Custom Memory Allocator

동적 메모리 할당 함수(`malloc`, `free`, `realloc`)를 직접 구현한 프로젝트입니다. 힙 메모리 관리 알고리즘과 메모리 단편화 최소화 기법을 구현했습니다.

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
- `mm.h`: 인터페이스 정의
- `memlib.c`: 메모리 시스템 인터페이스

**컴파일 및 테스트**:
```bash
cd prj4_20221197
make
./mdriver  # 메모리 할당기 테스트 드라이버
```

**성능 지표**:
- 할당 효율성 (Utilization)
- 처리량 (Throughput)
- 메모리 단편화 최소화

**기술 스택**:
- 힙 메모리 관리 구조
- 동적 메모리 할당 알고리즘 (First Fit / Best Fit)
- 메모리 단편화 최소화 (Coalescing)
- 블록 메타데이터 관리

**실무 활용 사례**:
- **메모리 관리자**: 운영체제의 힙 관리자, 커널 메모리 할당자
- **가비지 컬렉터**: Java, Python, Go 등의 GC 구현 및 최적화
- **게임 엔진**: Unity, Unreal Engine의 커스텀 메모리 할당자 및 메모리 풀링
- **임베디드 시스템**: 제한된 메모리 환경에서의 효율적 메모리 관리
- **고성능 서버**: 메모리 풀링, 커스텀 할당자 (예: TCMalloc, jemalloc)
- **데이터베이스**: 버퍼 풀 관리, 쿼리 실행 중 메모리 할당 최적화

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

---

## 기술 요약

### 핵심 기술

1. **프로세스 관리**
   - 프로세스 생성 및 종료 (`fork`, `exec`, `wait`)
   - 프로세스 간 통신 (파이프, 시그널)
   - Job Control 및 프로세스 그룹 관리

2. **파일 시스템**
   - 파일 디스크립터 관리
   - 리다이렉션 및 파이프 구현
   - 파일 I/O 작업

3. **네트워크 프로그래밍**
   - 소켓 프로그래밍
   - 클라이언트-서버 아키텍처
   - I/O 다중화 (`select()`)

4. **메모리 관리**
   - 동적 메모리 할당 알고리즘
   - 힙 관리 및 단편화 최소화
   - 메모리 효율성 최적화

5. **동시성 처리**
   - 프로세스 기반 동시성
   - I/O 다중화
   - 이벤트 기반 프로그래밍

### 실무 적용 분야

이 프로젝트들은 다음 분야의 실무에 직접적으로 활용됩니다:

- **시스템 소프트웨어 개발**: 운영체제, 드라이버, 펌웨어 개발
- **인프라/백엔드 개발**: 서버 개발, 네트워크 프로그래밍, 분산 시스템
- **임베디드 시스템**: IoT, 자동차, 로봇, 산업용 장비
- **게임 개발**: 게임 엔진, 게임 서버, 실시간 멀티플레이어 시스템
- **보안/네트워크**: 방화벽, 라우터, 네트워크 보안 솔루션
- **클라우드/인프라**: 컨테이너 오케스트레이션, 가상화 기술

---

## References

- CS:APP (Computer Systems: A Programmer's Perspective) 3rd Edition
- Linux 시스템 콜 매뉴얼 (`man` 페이지)

---

## Contact

프로젝트에 대한 문의사항이 있으시면 다음으로 연락해주세요:
- 이메일: yorange50@gmail.com

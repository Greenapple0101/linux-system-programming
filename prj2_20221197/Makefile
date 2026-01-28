# 컴파일러 및 컴파일 옵션
CC = gcc
CFLAGS = -Wall

# 소스 및 오브젝트 파일 목록
SRCS = bitmap.c \
       debug.c \
       hash.c \
       hex_dump.c \
       list.c \
       main.c
       # round.c (필요하다면 여기서 주석을 해제하거나 경로를 올바르게 지정)

OBJS = $(SRCS:.c=.o)

# 최종 생성될 실행 파일 이름
TARGET = testlib

# PHONY(가상) 타겟 선언
.PHONY: all clean runscript

# 기본 빌드 타겟
all: $(TARGET)

# 실행 파일(testlib) 빌드 규칙
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# 각 .c 파일을 .o 파일로 컴파일하는 규칙
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 의존성 선언(헤더 파일 변경 시 해당 오브젝트 파일 재컴파일)
bitmap.o: bitmap.c bitmap.h limits.h
debug.o: debug.c debug.h
hash.o: hash.c hash.h
hex_dump.o: hex_dump.c hex_dump.h
list.o: list.c list.h
main.o: main.c bitmap.h debug.h hash.h hex_dump.h list.h
# round.o: round.c round.h (round.c를 사용하지 않는다면 제거)

# 빌드 산출물 정리
clean:
	rm -f $(TARGET) $(OBJS)

# 스크립트 실행을 위한 가상 타겟 (필요 시 사용)
runscript:
	@echo "여기에 스크립트 실행 로직을 추가하세요."

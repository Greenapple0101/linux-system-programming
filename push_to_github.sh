#!/bin/bash

# GitHub 저장소에 현재 폴더의 모든 파일을 업로드하는 스크립트

cd /Users/baegseoyeon/Desktop/cse4100

# 기존 .git 폴더가 있으면 삭제
if [ -d .git ]; then
    rm -rf .git
fi

# Git 저장소 초기화
git init

# 원격 저장소 추가
git remote add origin https://github.com/Greenapple0101/cse4100.git

# 모든 파일 추가
git add .

# 커밋
git commit -m "Replace all files with current folder contents"

# main 브랜치로 설정
git branch -M main

# 강제 푸시 (기존 내용 덮어쓰기)
git push -f origin main

echo "완료되었습니다!"

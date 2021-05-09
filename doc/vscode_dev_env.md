# Visual Studio 개발 환경 설정

## References
- [Visual Studio Code Dev](https://chromium.googlesource.com/chromium/src/+/master/docs/vscode.md) 참고

## Tips
- C/C++ 필요 없음
- clangd 설치
  - `Clangd:Path`를 VS Code에서 설치한 clangd 경로로 설정함
    ```
    ~/.config/Code/User/globalStorage/llvm-vs-code-extensions.vscode-clangd/install/11.0.0/clangd_11.0.0/bin/clangd
    ```
  - `Clangd:On Config Changed`를 `restart`로 지정
  - `src`에서 다음을 실행해서 DB 업데이트
    ```
    tools/clang/scripts/generate_compdb.py -p out/<build> > compile_commands.json
    ```

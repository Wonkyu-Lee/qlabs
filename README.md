# QLabs
Chromium SDK를 이용한 샘플 앱

## Chromium/src에 적용
```
$ cd .. && git apply qlabs/src.patch
```

## Sample learn_mojo
```
$ gn gen out/Default
$ ninja -C out/Default learn_mojo

# Run single-process mojo example
$ out/Default/mojo_single

# Run multi-process mojo example
$ out/Default/mojo_multi
```



```bash
meson setup ./build

cd ./build
meson compile
meson test
```

```bash
export LSAN_OPTIONS="suppressions=/path/to/lsan.supp"
export ASAN_OPTIONS="symbolize=1:fast_unwind_on_malloc=0"
./breezy > ./stdout 2> ./stderr
```

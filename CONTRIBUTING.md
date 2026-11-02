<!-- SPDX-License-Identifier: Apache-2.0 -->
# Contributing to PandaManager

## Developer Certificate of Origin (DCO)

By contributing, you certify that:
1. The contribution was created by you, or you have the right to submit it.
2. The contribution is licensed under Apache-2.0.

Sign your commits: `git commit -s`

## Build
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build build -j$(nproc)
ctest --test-dir build
```

## Code style
```bash
clang-format -i src/*.cpp include/*.hpp
clang-tidy src/*.cpp -- -std=c++20
```

## Pull Request guidelines
- One feature/fix per PR
- All tests must pass
- Code coverage must not decrease
- REUSE/SPDX headers required on every new file

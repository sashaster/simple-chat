[![CI](https://github.com/Sashaster/simple-chat/actions/workflows/ci.yml/badge.svg?branch=main&event=push)](https://github.com/Sashaster/simple-chat/actions/workflows/ci.yml) ![Static Badge](https://img.shields.io/badge/3.14+-violet?style=flat&logo=cmake&logoColor=%23064F8C&label=CMake) ![Static Badge](https://img.shields.io/badge/linux-grey?logo=linux&logoColor=gold)

# Simple chat

<sub>A client/server chat application built with sockets in C++.</sub>

---

### Features

- multi-client support
- message broadcasting
- logging
- graceful shutdown on Ctrl+C

### Building

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

### Running

```bash
#run server
./build/chat server

#run client
./build/chat client
```

### Configuration

| Option    | Default     | Possible values          |
|-----------|-------------|--------------------------|
 | log-level | `info`      | `debug`, `info`, `error` |
  | host      | `127.0.0.1` | any valid IP address     |
   | port      | `8080`      | `1-65535`                |

Example:
```bash
./build/chat server --host=0.0.0.0 ---port=10000 --log-level=debug
```

### Testing

```bash
ctest --test-dir build --output-on-failure
```

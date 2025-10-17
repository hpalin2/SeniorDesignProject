# SuctionSense Crow Server

This repository now contains a C++ implementation of the SuctionSense operating room suction dashboard built with the [Crow](https://crowcpp.org/) microframework. It serves the same UI and data that previously lived in the React application while providing a lightweight C++ backend that can be deployed as a single binary.

## Prerequisites

- CMake 3.18+
- A C++20 compatible compiler (GCC 11+, Clang 13+, or MSVC 2022)
- Git

Crow automatically pulls in its dependencies (Boost, asio, fmt, etc.) through CMake's `FetchContent`.

## Building

```bash
cmake -S . -B build
cmake --build build
```

## Running

```bash
./build/room-suction-status
```

The server listens on port `18080` by default. Override the port by setting the `PORT` environment variable before launching the binary.

Open `http://localhost:18080/` to see the dashboard. The following helper endpoints are also available:

- `GET /api/rooms` – JSON payload describing the current room status.
- `GET /health` – simple health probe that returns `ok`.

## Project Structure

```
.
├── CMakeLists.txt        # Build configuration
├── README.md             # Project documentation
└── src
    └── main.cpp          # Crow application entry point
```

## Development Notes

- The dashboard data is currently seeded with static room information that mirrors the original React demo. Replace the entries in `src/main.cpp` with real data sources as needed.
- Styling is handled with embedded CSS so that the server responds with a single self-contained document. Adjust `render_dashboard` if you want to load external assets instead.

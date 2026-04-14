# Skye3D

A **server-client 3D multiplayer game engine** built with C++, Raylib for rendering, and Squirrel for scripting.

![License](https://img.shields.io/badge/license-MIT-blue.svg)
![Language](https://img.shields.io/badge/language-C%2B%2B23-purple.svg)
![Framework](https://img.shields.io/badge/Raylib-orange.svg)
![Framework](https://img.shields.io/badge/SDL-green.svg)
![Framework](https://img.shields.io/badge/GLFW-darkred.svg)
![Networking](https://img.shields.io/badge/ENET-lightblue.svg)

---

## Documentation

| Resource                                                                           | Description                      |
| ---------------------------------------------------------------------------------- | -------------------------------- |
| [Scripting Docs](gamedata/docs.md)                                                 | Squirrel scripting API reference |
| [BSP File Spec](SPEC.md)                                                           | BSP format documentation         |
| [Quake Specs](https://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm) | Quake format documentation       |
| [Quake Level Viewer](https://github.com/bytesiz3d/quake-level-viewer)              | Level loading reference          |

---

## Getting Started

### Prerequisites

- C++23 compiler
- [Raylib](https://www.raylib.com/)
- [ENet](#)
- [ImGui](#)
- [rlImGui](#)
- [squirrel](#)

### Build

```bash
git clone https://github.com/yourname/skye3d.git
cd skye3d

mkdir build && cd build
cmake -G "Ninja" ..
ninja
```

### Run

```bash
# Start as server
edit `settings.cfg` > set 'is_hosting=true'

# Start as client
edit `settings.cfg` > set 'is_hosting=false'
```

---

## Project Structure

```
sky3d/
├── gamedata/
│   ├── scripts/
│   │   ├── client/       # Client-side scripts
│   │   ├── server/       # Server-side scripts
│   │   └── shared/       # Shared scripts
│   └── docs.md           # Scripting documentation
├── src/
│   ├── include/          # Header Files
│   ├── lib/              # Third-Party Libraries
│   ├── game/             # Core game loop
│   ├── system/           # System & Engine
└── README.md
```

---

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

---

## Acknowledgments

- [Raylib](https://www.raylib.com/) — Simple and easy-to-use library for game programming
- [ENet](http://enet.bespin.org/) — Reliable UDP networking library
- [Squirrel](http://squirrel-lang.org/) — High-level scripting language
- [Quake Level Viewer](https://github.com/bytesiz3d/quake-level-viewer) — Reference implementation

---

<p align="center">
  <i>Built with ❤️ by Skye</i>
</p>

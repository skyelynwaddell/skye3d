# Skye3D

A **server-client 3D multiplayer game engine** built with C++, Raylib for rendering, and LUA for scripting.

![License](https://img.shields.io/badge/license-GPL-blue.svg)
![Language](https://img.shields.io/badge/language-C%2B%2B23-purple.svg)
![Language](https://img.shields.io/badge/scripting-LUA-magenta.svg)
![Framework](https://img.shields.io/badge/Raylib-orange.svg)
![Framework](https://img.shields.io/badge/SDL-green.svg)
![Framework](https://img.shields.io/badge/GLFW-darkred.svg)
![Networking](https://img.shields.io/badge/ENET-lightblue.svg)

---

## Documentation

| Resource                                                                           | Description                 |
| ---------------------------------------------------------------------------------- | --------------------------- |
| [Scripting Docs](gamedata/docs.md)                                                 | LUA scripting API reference |
| [BSP File Spec](SPEC.md)                                                           | BSP format documentation    |
| [Quake Specs](https://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm) | Quake format documentation  |
| [Quake Level Viewer](https://github.com/bytesiz3d/quake-level-viewer)              | Level loading reference     |

---

## Getting Started

### Prerequisites

- C++23 compiler
- [Raylib](https://www.raylib.com/)
- [ENet](#)
- [ImGui](#)
- [rlImGui](#)
- [lua & sol2](#)

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
skye3d/
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

This project is licensed under the GPL License - see the [LICENSE](LICENSE) file for details.

---

## This wouldn't be possible without the following

- [QuakeWorld](https://github.com/id-software/quake) — Created by id Software as an online version of the original Quake
- [Raylib](https://www.raylib.com/) — Simple and easy-to-use library for game programming
- [ENet](http://enet.bespin.org/) — Reliable UDP networking library
- [Lua](https://www.lua.org/) — High-level scripting language
- [sol2](https://github.com/ThePhd/sol2) — Lua-to-C++ Bindings
- [Quake Level Viewer](https://github.com/bytesiz3d/quake-level-viewer) — Reference implementation

---

<p align="center">
  <i>Built with ❤️ by Skye</i>
</p>

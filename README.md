# dmn-cpp

`dmn-cpp` is a modern C++ wrapper around the HCL Domino C API. It provides a
small RAII-oriented interface for opening NSF databases, reading and writing
notes, working with views, running DQL queries, sending mail, handling ACL data,
and building Domino add-ins.

The library still links to the Domino runtime, but application code does not
need to be written directly against the Domino C headers. Domino handles and 
memory blocks are represented with project types such as `dmn::dhandle_t` and 
`dmn::os::block_id`, and higher-level classes own, lock, unlock, and release
resources for you.

## Quickstart

Add `dmn-cpp` as a custom vcpkg registry in your `vcpkg-configuration.json`:

```json
{
  "registries": [
    {
      "kind": "git",
      "repository": "https://github.com/NexelOfficial/vcpkg-registry.git",
      "baseline": "<baseline>",
      "packages": ["dmn"]
    }
  ]
}
```

Replace `<baseline>` with the commit hash listed in the relevant registry release. You can find the current baseline on the [latest release page](https://github.com/NexelOfficial/dmn-cpp/releases/latest). Make sure to also add [the default registry](https://learn.microsoft.com/en-us/vcpkg/reference/vcpkg-json#builtin-baseline) if you haven't configured it yet.

Then add `dmn` as a port in your `vcpkg.json`:

```json
{
  "dependencies": [
    "dmn"
  ]
}

```

## Building

Requirements:

- CMake 3.20 or newer.
- A C++23 compiler.
- vcpkg available through `VCPKG_ROOT`.
- HCL Domino/Notes runtime libraries and headers.

On Windows, the build expects Domino headers and `notes.lib` under
`third_party/include` and `third_party/lib/notes.lib`.

```powershell
cmake --preset win
cmake --build --preset release
```

On Linux, set `Notes_ExecDirectory` to the Domino executable/library directory
so CMake can locate `libnotes.so`.

```sh
# This is often already set when installing HCL Domino
export Notes_ExecDirectory=/opt/hcl/domino/notes/latest/linux
cmake --preset unix
cmake --build build
```

## Testing

`CMakeLists.txt` enables `CTest` and adds the `tests/` subdirectory when
`BUILD_TESTING` is on. After configuring and building, run the test suite from
the generated build tree:

```sh
# Windows
ctest --test-dir build -C Release --output-on-failure
# Linux
ctest --test-dir build --output-on-failure
```

## Packaging With CPack

The project includes CPack configuration in `CMakeLists.txt`. After configuring
and building the project, create a distributable package from the build
directory:

```shell
# Windows
cpack --config ./build/CPackConfig.cmake -C Release
# Linux
cpack --config ./build/CPackConfig.cmake
```

CPack writes packages to `build/packages`. The generated package name includes
the project version, operating system, and processor architecture:

```text
dmn-<version>-<system>-<processor>
```

The package contains the built `dmn` library, the public headers, and the CMake
package files needed by downstream projects using `find_package(dmn REQUIRED)`.

The CMake target is exported as `dmn::dmn`:

```cmake
find_package(dmn REQUIRED)
target_link_libraries(my_app PRIVATE dmn::dmn)
```

> _This documentation was written with AI assistance and reviewed by a human._

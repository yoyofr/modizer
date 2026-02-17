[简体中文](README.zh-CN.md) | English
# hashlib

[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Language](https://img.shields.io/badge/Language-C%2B%2B11-blue.svg)]()

`Cra3z' hashlib` is a C++11 header-only hash algorithm library which like Python's hashlib.

supported algorithms:
* md5
* sha1
* sha224
* sha256
* sha384
* sha512
* sha3_224
* sha3_256
* sha3_384
* sha3_512

## Usage

### Using headers
hashlib is a header-only library, so you can simply copy the headers in the directory `include/hashlib` to where you need it.
However, it is recommended to embed hashlib as a subproject in your project, or install it first and then import it via CMake's `find_package`.
```cmake
find_package(hashlib REQUIRED)
target_link_libraries(<your-target> hashlib::hashlib)
```
#### Single header
You can also generate a single header `hashlib.hpp` which combines all headers in the directory `include/hashlib`, see [Generate single header](#generate-single-header).

### Using C++20 module
hashlib supports C++20 module (need the standard module `std`); you can generate the module file, and then import it, see [Generate C++20 module](#generate-c20-module).
```cpp
import std;
import hashlib;

int main() {
    hashlib::md5 md5;
    md5.update("hello world");
    std::println("{}", md5.hexdigest());
}
```


## Building & Installation

### Requirements
- CMake 3.22+ (for building)
- C++ compiler which supports C++11 or newer standard

```shell
mkdir <your-build-dir>
cd <your-build-dir>
cmake -S . -B <your-build-dir>
cmake --build <your-build-dir>
cmake --install <your-build-dir> --prefix <your-install-dir>
```

### CMake Options

| Option | Default | Description            |
|--------|---------|------------------------|
| HASHLIB_TESTS | ON      | enable tests           |
| HASHLIB_BUILD_SINGLE_HEADER | OFF     | generate single header |
| HASHLIB_BUILD_MODULE | OFF     | generate C++20 module  |

#### Generate single header
using the following command, the single header `hashlib.hpp` which combines all headers in the directory `include/hashlib` will be generated in your build directory.
```shell
cmake -S . -B <your-build-dir> -DHASHLIB_BUILD_SINGLE_HEADER=ON
```

#### Generate C++20 module
using the following command, the module file `hashlib.cppm` will be generated in your build directory.
```shell
cmake -S . -B <your-build-dir> -DHASHLIB_BUILD_MODULE=ON
```

### using conan package manager
You can also install hashlib by [conan package manager](https://github.com/conan-io).
```shell
conan install --requires=hashlib/[*]
```

## Examples

* get md5 of a string by using `hashlib::md5`

```cpp
#include <iostream>
#include <hashlib/md5.hpp>

int main() {
    hashlib::md5 md5{std::string("hello world")};
    std::cout << md5.hexdigest() << '\n'; // 5eb63bbbe01eeed093cb22bb8f5acdc3
}
```

* using the method `update` or `operator<<`

```cpp
#include <iostream>
#include <hashlib/md5.hpp>

int main() {
    {
        hashlib::md5 md5;
        md5.update(std::string("hello"));
        md5.update(std::string(" world"));
        std::cout << md5.hexdigest() << '\n'; // 5eb63bbbe01eeed093cb22bb8f5acdc3
    }
    {
        hashlib::md5 md5;
        md5 << std::string("hello") << std::string(" world");
        std::cout << md5.hexdigest() << '\n'; // 5eb63bbbe01eeed093cb22bb8f5acdc3
    }
}
```

* hashing the bytes from iterators and ranges whose elements are `char`, `unsigned char`, `signed char` or `std::byte (since c++17)`
```cpp
#include <iostream>
#include <hashlib/sha1.hpp>

int main() {
    unsigned char data[] = {0x00, 0xff, 0x55, 0xaa, 0x12, 0x34, 0x56, 0x78};
    hashlib::sha1 sha1;
    sha1.update(data);
    std::cout << sha1.hexdigest() << '\n'; // f9d9a450e6e14895936f8dc796e30209528de337
}
```

```cpp
#include <iostream>
#include <fstream>
#include <hashlib/sha1.hpp>

int main() {
    std::ifstream file("example.txt", std::ios::in | std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file" << std::endl;
        return EXIT_FAILURE;
    }
    hashlib::sha1 sha1;
    sha1.update(
        std::istreambuf_iterator<char>{file}, 
        std::istreambuf_iterator<char>{}
    );
    std::cout << sha1.hexdigest() << '\n'; // output the sha1 digest of the file example.txt
}
```

# Change Log

## Unreleased
### Added

### Changed

### Removed
- `hastype` keyword because I never had to implement compile time typechecking, so it's not useful

## 1.2.2
### Added
- adding `import` keyword (handled by parser), throwing an error if a cyclic included is detected

### Changed
- CMakeLists.txt to add `install` rules: installing Ark in bin/ and the Ark standard library in share/.Ark/lib/
- updated documentation

## 1.2.1
### Added
- runtime typechecking
- exceptions (in the C++ Ark API)

### Changed
- updated the FFI to add the runtime typechecking

### Removed
- unnecessary destructors removed to let the compiler auto generate T(T&&) (to avoid implicitly using T(const T&))

## 1.2.0
### Added
- syntactic sugar handling in the parser
- GMP lib to handle very large number
- REPL (can be launched from the CLI)
- tests

### Changed
- changed syntax: using `{...}` as a `(begin ...)` and `[...]` as a `(list ...)`
- updated documentation according the new syntax
- the lexer is now using a Token structure to store the line and column as well as the token itself
- generating the FFI using include/Ark/MakeFFI.hpp, everything defined in one file to avoid having 2 files to update
- tests

### Removed
- dozerg::HugeNumber, it was too slow

## 1.1.0
### Added
- test.cpp to try to embed Ark into a C++ project
- updated the documentation
- the compiler can now return a read only version of the bytecode being executed
- the VM can take a bytecode or a filename
- *OOP* test with Ark using closures
- closures support
- Types.hpp (for the VM) to store the definitions of the NFT (Nil/True/False enum class) and the PageAddr_t
- Function.hpp to get a lambda from the interpreter and call it from C++ code

### Changed
- CMakeLists.txt, adding an option to chose between compiling main.cpp or test.cpp
- moved the VM FFI into include/Ark/VM

## 1.0.0
## Added
- beginning of the documentation
- compiler (ark code to ark bytecode)
- bytecode reader (human readable format)
- dozerg::HugeNumber to handle big numbers
- simple VM handling all the instructions, able to run an ark bytecode
- interpreter and VM FFI
- logger

## 0.1.0
### Added
- Node (to represent an AST node and a Node in the language)
- Environment to map variables and values
- Program executing Ark code from the AST
- standard library (builtin functions)
- Lexer and parser
- default CLI can handle the interpreter
- tests
- utils to play with files

## 0.0.1
### Added
- utils to play with strings and numbers
- default CLI (using clipp)
- CMakeLists to compile the project
- ryjen::format to format strings
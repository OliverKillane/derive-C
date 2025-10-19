{ pkgs ? import <nixpkgs> {} }:

# For development & testing
# - When upgrading, please also update the .vscode/settings.json and .clangd configuration

let
  llvm20 = pkgs.llvmPackages_20;
  crt    = llvm20.compiler-rt-libc;  # has the sanitizer headers under .dev/include
in
llvm20.stdenv.mkDerivation {
  name = "derive-c-clang";

  buildInputs =
    (with pkgs; [
      cmake
      ninja
      doxygen
      graphviz
    ]) ++ (with llvm20; [
      clang-tools
      libcxx
      clang
      llvm
      compiler-rt      # libs for -fsanitize=...
      lld
      crt              # keep the attr around (its .dev output provides headers)
      lldb
    ]);

  # Expose sanitizer headers to the compiler/clangd without editing CMake
  shellHook = ''
    # Prefer the compiler-rt *dev* include that has <sanitizer/...>
    export CPATH=${crt.dev}/include''${CPATH:+:$CPATH}
    export C_INCLUDE_PATH=${crt.dev}/include''${C_INCLUDE_PATH:+:$C_INCLUDE_PATH}
    export CPLUS_INCLUDE_PATH=${crt.dev}/include''${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}

    echo "Sanitizer headers -> ${crt.dev}/include (added to CPATH/C_INCLUDE_PATH/CPLUS_INCLUDE_PATH)"
  '';
}

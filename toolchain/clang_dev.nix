{ pkgs ? import <nixpkgs> {} }:

let
  pkgs = import <nixpkgs> {};
  pkgsUnstable = import (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/nixos-unstable.tar.gz";
    sha256 = "1zb5ca8jqavb19j7g06a41jg6bvpr20b9lihvham6qywhgaqprz9";
  }) {};

  uv = pkgsUnstable.uv;

  llvm20 = pkgs.llvmPackages_20;
  crt    = llvm20.compiler-rt-libc;
in
llvm20.stdenv.mkDerivation {
  name = "derive-c-clang";

  buildInputs =
    (with pkgs; [
      cmake
      ninja
      doxygen
      graphviz
    ]) ++ [
      uv
    ] ++ (with llvm20; [
      clang-tools
      libcxx
      clang
      llvm
      compiler-rt
      lld
      crt
      lldb
    ]);

  shellHook = ''
    export CPATH=${crt.dev}/include''${CPATH:+:$CPATH}
    export C_INCLUDE_PATH=${crt.dev}/include''${C_INCLUDE_PATH:+:$C_INCLUDE_PATH}
    export CPLUS_INCLUDE_PATH=${crt.dev}/include''${CPLUS_INCLUDE_PATH:+:$CPLUS_INCLUDE_PATH}

    echo "Sanitizer headers -> ${crt.dev}/include"
    echo "uv from nixpkgs-unstable: $(uv --version || echo 'not yet built')"

    # Generate VSCode settings
    (
      cd scripts
      uv run intellisense \
        --vscode-dir=../.vscode \
        --clangd-path="${llvm20.clang-tools}/bin/clangd" \
        --clang-path="${llvm20.clang}/bin/clang" \
        --resource-dir="$(${llvm20.clang}/bin/clang -print-resource-dir)"
    )

    echo "============================================================="
    echo "A shell for derive-c development, with all conveniences."
    echo "clang version:"
    clang --version
    echo "============================================================="
  '';
}

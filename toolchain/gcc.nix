{ pkgs ? import <nixpkgs> {} }:

# For GCC compatibility check only
let
  pkgsUnstable = import (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/nixos-unstable.tar.gz";
    sha256 = "1zb5ca8jqavb19j7g06a41jg6bvpr20b9lihvham6qywhgaqprz9";
  }) {};

  uv = pkgsUnstable.uv;

  gcc = pkgs.gcc14;
  myStdenv = pkgs.stdenvAdapters.overrideCC pkgs.stdenv gcc;
in
myStdenv.mkDerivation {
  name = "derive-c-gcc14";

  buildInputs = (with pkgs; [
    cmake
    ninja
    doxygen
    graphviz
    binutils
    gdb
    pkg-config
    gcc  # optional, keeps gcc tools on PATH in the shell
  ]) ++ [
    uv
  ];

  shellHook = ''
    echo "============================================================="
    echo "A shell for testing with gcc compilation. Development"
    echo "conveniences (e.g. coverage) are not supported."
    echo "GCC version:"
    gcc --version
    echo "============================================================="
  '';
}

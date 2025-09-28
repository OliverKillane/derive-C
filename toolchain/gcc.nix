{ pkgs ? import <nixpkgs> {} }:

# For GCC compatibility check only

let
  myStdenv = pkgs.stdenvAdapters.overrideCC pkgs.stdenv pkgs.gcc15;
in
myStdenv.mkDerivation {
  name = "derive-c-gcc";

  buildInputs = with pkgs; [
    cmake
    ninja
    doxygen
    graphviz
    gcc15
    binutils
    gdb
    pkg-config
  ];
}
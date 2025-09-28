{ pkgs ? import <nixpkgs> {} }:

# For GCC compatibility check only
let
  gcc = pkgs.gcc14;
  myStdenv = pkgs.stdenvAdapters.overrideCC pkgs.stdenv gcc;
in
myStdenv.mkDerivation {
  name = "derive-c-gcc14";

  buildInputs = with pkgs; [
    cmake
    ninja
    doxygen
    graphviz
    binutils
    gdb
    pkg-config
    gcc  # optional, keeps gcc tools on PATH in the shell
  ];
}

{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    # pkgs.gcc14
    pkgs.libclang
    pkgs.clang-tools_18  # Includes clang-format
  ];
}
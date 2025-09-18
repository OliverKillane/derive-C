{ pkgs ? import <nixpkgs> {} }:

let
  # pin to LLVM/Clang 20 everywhere
  myStdenv = pkgs.llvmPackages_20.stdenv;
  llvm20   = pkgs.llvmPackages_20;
in

myStdenv.mkDerivation {
  name = "derive-c-dev-shell";

  buildInputs =
    (with pkgs; [
      cmake
      ninja
      doxygen
      graphviz
    ])
    ++ (with llvm20; [
      clang-tools  # clang-format & clang-tidy for 20
      libcxx       # libc++-20
      clang        # clang-20
      llvm         # the umbrella LLVM-20 set (includes llvm-cov, llvm-profdata, etc.)
    ]);
}

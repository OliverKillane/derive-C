{ pkgs ? import <nixpkgs> {} }:

let
  # pin to LLVM/Clang 19 everywhere
  myStdenv = pkgs.llvmPackages_19.stdenv;
  llvm19   = pkgs.llvmPackages_19;
in

myStdenv.mkDerivation {
  name = "derive-c-dev-shell";

  buildInputs =
    # first the generic tools from pkgs
    (with pkgs; [
      cmake
      ninja
      doxygen
      graphviz
      lcov
      gcovr
    ])
    # then concat the llvm-19 tools
    ++ (with llvm19; [
      clang        # clang-19
      clang-tools  # clang-format & clang-tidy for 19
      libcxx       # libc++-19
      llvm         # the umbrella LLVM-19 set (includes llvm-cov, llvm-profdata, etc.)
    ]);
}

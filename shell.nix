{ pkgs ? import <nixpkgs> {} }:

pkgs.mkShell {
  buildInputs = [
    # pkgs.gcc14
    pkgs.libclang
    pkgs.clang-tools_18  # Includes clang-format

    # Added for documentation support
    pkgs.doxygen
    pkgs.graphviz        # Optional: needed for diagrams like call graphs
    pkgs.unzip           # Optional: for extracting doxygen-awesome-css
  ];
}
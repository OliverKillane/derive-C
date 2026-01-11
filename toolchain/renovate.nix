{ pkgs ? import <nixpkgs> {} }:

let
  pkgsUnstable = import (builtins.fetchTarball {
    url = "https://github.com/NixOS/nixpkgs/archive/nixos-unstable.tar.gz";
    sha256 = "1zb5ca8jqavb19j7g06a41jg6bvpr20b9lihvham6qywhgaqprz9";
  }) {};

in
pkgs.mkShell {
  name = "renovate-shell";

  buildInputs = [
    pkgsUnstable.renovate
  ];

  shellHook = ''
    echo "============================================================="
    echo "A shell for running renovate for updating cmake dependencies."
    echo "Renovate version:"
    renovate --version
    echo "============================================================="
  '';
}

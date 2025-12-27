{ pkgs ? import <nixpkgs> {} }:

let
  llvm = pkgs.llvmPackages_20;
  crt  = llvm.compiler-rt-libc;

  # IMPORTANT: use the wrapped clang so it can link (finds crt*.o and libgcc)
  clang = llvm.clang;

  llvmProjectSrc = pkgs.fetchFromGitHub {
    owner = "llvm";
    repo  = "llvm-project";
    rev   = "llvmorg-20.1.0-rc3";
    hash  = "sha256-mLSBoyq24FD+khynC6sC5IkDFqizgAn07lR9+ZRXlV0=";
  };

  msanLibcxx = pkgs.stdenv.mkDerivation {
    pname = "libcxx-msan";
    version = "20.1.0-rc3";
    src = llvmProjectSrc;

    nativeBuildInputs = with pkgs; [ cmake ninja python3 ];
    buildInputs = [ crt ];

    # compiler-rt headers during build (needed for <sanitizer/msan_interface.h>, etc)
    NIX_CFLAGS_COMPILE   = "-I${crt.dev}/include";
    NIX_CXXFLAGS_COMPILE = "-I${crt.dev}/include";

    configurePhase = ''
      runHook preConfigure

      # -------------------------
      # 1) Host tools (NO MSAN)
      # -------------------------
      cmake -S "$src/llvm" -B host -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER=${clang}/bin/clang \
        -DCMAKE_CXX_COMPILER=${clang}/bin/clang++ \
        -DLLVM_ENABLE_PROJECTS= \
        -DLLVM_ENABLE_RUNTIMES= \
        -DLLVM_TARGETS_TO_BUILD="X86" \
        -DLLVM_INCLUDE_TESTS=OFF \
        -DLLVM_BUILD_TOOLS=ON \
        -DLLVM_BUILD_UTILS=OFF \
        -DLLVM_BUILD_EXAMPLES=OFF

      ninja -C host llvm-min-tblgen

      # -------------------------
      # 2) Runtimes (WITH MSAN)
      # -------------------------
      cmake -S "$src/runtimes" -B build -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_C_COMPILER=${clang}/bin/clang \
        -DCMAKE_CXX_COMPILER=${clang}/bin/clang++ \
        -DCMAKE_INSTALL_PREFIX=$out \
        -DLLVM_MAIN_SRC_DIR="$src/llvm" \
        -DLLVM_TABLEGEN="$PWD/host/bin/llvm-min-tblgen" \
        -DLLVM_USE_SANITIZER=MemoryWithOrigins \
        -DLLVM_ENABLE_RUNTIMES="libcxx;libcxxabi" \
        -DLIBCXX_ENABLE_SHARED=ON \
        -DLIBCXXABI_ENABLE_SHARED=ON \
        -DLIBCXX_ENABLE_STATIC=OFF \
        -DLIBCXXABI_ENABLE_STATIC=OFF \
        -DLIBCXX_ENABLE_EXCEPTIONS=ON \
        -DLIBCXXABI_ENABLE_EXCEPTIONS=ON \
        -DLIBCXX_ENABLE_RTTI=ON \
        -DLIBCXXABI_ENABLE_RTTI=ON \
        -DLIBCXXABI_USE_LLVM_UNWINDER=OFF

      runHook postConfigure
    '';

    buildPhase = ''
      runHook preBuild
      ninja -C build
      runHook postBuild
    '';

    installPhase = ''
      runHook preInstall
      ninja -C build install
      runHook postInstall
    '';
  };

in
pkgs.mkShell {
  name = "derive-c-clang-msan-min";

  # Minimal: just enough to configure/build with clang, and provide MSan libc++
  buildInputs =
    (with pkgs; [ cmake ninja ]) ++
    (with llvm; [ clang compiler-rt compiler-rt-libc ]) ++
    [ msanLibcxx ];

  shellHook = ''
    echo "=== derive-C MSan shell (minimal) ==="
    echo "clang: $(${clang}/bin/clang --version | head -n1)"
    echo "msan libc++ prefix: ${msanLibcxx}"
    echo "msan libc++ headers: ${msanLibcxx}/include/c++/v1"
    echo "msan libc++ libs: ${msanLibcxx}/lib"
    echo "compiler-rt headers: ${crt.dev}/include"

    export CC=${clang}/bin/clang
    export CXX=${clang}/bin/clang++

    # CMake will read these to wire libc++ + msan headers correctly for MSan builds
    export NIX_MSAN_LIBCXX=${msanLibcxx}
    export NIX_MSAN_CXXINC=${msanLibcxx}/include/c++/v1
    export NIX_MSAN_LIBDIR=${msanLibcxx}/lib
    export NIX_MSAN_RTINC=${crt.dev}/include

    # Helpful defaults for MSan runs
    export LD_LIBRARY_PATH="${msanLibcxx}/lib''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    export MSAN_OPTIONS="halt_on_error=1:exit_code=86:strip_path_prefix=$(pwd)/''${MSAN_OPTIONS:+:$MSAN_OPTIONS}"

    echo "NIX_MSAN_LIBCXX=$NIX_MSAN_LIBCXX"
    echo "NIX_MSAN_CXXINC=$NIX_MSAN_CXXINC"
    echo "NIX_MSAN_LIBDIR=$NIX_MSAN_LIBDIR"
    echo "NIX_MSAN_RTINC=$NIX_MSAN_RTINC"
  '';
}

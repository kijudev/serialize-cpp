{
  description = "C++ DevShell";

  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-unstable";
  };

  outputs =
    { self, nixpkgs }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
      };
      llvm = pkgs.llvmPackages_latest;
    in
    {
      devShells.${system}.default =
        pkgs.mkShell.override
          {
            stdenv = llvm.libcxxStdenv;
          }
          {
            packages = [
              llvm.clang-tools
              llvm.lld
              llvm.lldb

              pkgs.gnumake
              pkgs.cppcheck
              pkgs.bear
              pkgs.doctest
            ];

            LD_LIBRARY_PATH = pkgs.lib.makeLibraryPath ([
              llvm.libcxx
            ]);

            shellHook = ''
              echo "======== C++23 DevShell ========"
                echo "Compiler : $(clang++ --version | head -1)"
                echo ""
                echo "Build    : make"
            '';
          };

    };
}

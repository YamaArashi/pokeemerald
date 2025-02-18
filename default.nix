{ pkgs ? import <nixpkgs> {}, stdenv ? pkgs.stdenv }:

let
  nix-filter = import (pkgs.fetchFromGitHub {
    owner = "numtide";
    repo = "nix-filter";
    rev = "3b821578685d661a10b563cba30b1861eec05748";
    hash = "sha256-RizGJH/buaw9A2+fiBf9WnXYw4LZABB5kMAZIEE5/T8=";
  });

  # binutils-unwrapped-all-targets does not contain GNU as or ld on aarch64-darwin
  # this might be an oversight as aarch64-apple-darwin is not supported target by GNU binutils
  # https://github.com/NixOS/nixpkgs/issues/201753
  arm-binutils = pkgs.binutils-unwrapped.overrideAttrs(o: {
    configurePlatforms = [ "build" "host" ];
    configureFlags = o.configureFlags ++ [
      "--target=arm-none-eabi"
      "--program-prefix=arm-none-eabi-"
      "--disable-nls"
    ];
    doInstallCheck = false;
  });

  agbcc = stdenv.mkDerivation {
    name = "agbcc";
    dontConfigure = true;
    nativeBuildInputs = with pkgs; [ bash arm-binutils ];
    stripDebugList = [ "bin" ];
    NIX_CFLAGS_COMPILE = [ "-Wno-format-security" ];
    postPatch = "substituteInPlace libc/Makefile --replace /bin/bash bash";
    buildPhase = "sh build.sh";

    installPhase = ''
      sh install.sh $TMPDIR
      cp -r $TMPDIR/tools/agbcc $out
    '';

    src = pkgs.fetchFromGitHub {
      owner = "pret";
      repo = "agbcc";
      rev = "d59cfb5ac1ce13d4bc9875ffef1666b118338d33";
      hash = "sha256-pY8hAJaxGh4tZPM1v4uAgQvCsYs/9rzP3tWLYyM7h+M=";
    };
  };

  arm-toolchain = let
    cpp = pkgs.writeShellScript "cpp" ''${pkgs.gcc-unwrapped}/bin/gcc -E -nostdinc -I${agbcc} "$@"'';
  in pkgs.runCommandLocal "wrap-cc" {} ''
    mkdir -p $out/bin
    ln -s ${agbcc}/bin/agbcc_arm $out/bin/arm-none-eabi-gcc
    ln -s ${cpp} $out/bin/arm-none-eabi-cpp
    for bin in ${arm-binutils}/bin/*; do
      ln -s $bin $out/bin/$(basename $bin)
    done
  '';

  pipefail-shell = pkgs.writeShellScript "pipefail-shell" ''${pkgs.bash}/bin/bash -o pipefail "$@"'';
in stdenv.mkDerivation {
  name = "pokemon-emerald";
  dontConfigure = true;
  nativeBuildInputs = with pkgs; [ pkg-config zlib libpng ];
  preBuild = "rm -f tools/agbcc && ln -s ${agbcc} tools/agbcc";
  makeFlags = [ "CC=cc" "CXX=c++" "TOOLCHAIN=${arm-toolchain}" "SHELL=${pipefail-shell}" ];
  src = nix-filter {
    root = ./.;
    include = [
      ./asm
      ./common_syms
      ./constants
      ./data
      ./gflib
      ./graphics
      ./include
      ./libagbsyscall
      ./sound
      ./src
      ./tools
      ./Makefile
      ./rom.sha1
      (nix-filter.matchExt "mk")
      (nix-filter.matchExt "txt")
    ];
  };
  installPhase = ''
    mkdir -p $out
    cp pokeemerald.gba $out/pokemon-emerald.gba
  '';
}

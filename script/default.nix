{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  cilkRtsSrc ? ../.
}:

stdenv.mkDerivation rec {
  name = "cilk-plus-rts-with-stats";

  src = cilkRtsSrc;

  buildInputs = [
    pkgs.libtool
    pkgs.automake
    pkgs.autoconf
  ];

  preConfigure = "libtoolize && aclocal && automake --add-missing && autoconf";

  enableParallelBuilding = true;

  meta = {
    description = "";
    license = "MIT";
    homepage = http://deepsea.inria.fr/;
  };
}
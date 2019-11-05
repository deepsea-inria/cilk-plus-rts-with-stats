{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  cilkRtsSrc ? ./.,
  gcc ? pkgs.gcc7,
  libtool ? pkgs.libtool,
  automake ? pkgs.automake,
  autoconf ? pkgs.autoconf
}:

pkgs.stdenvNoCC.mkDerivation rec {
  name = "cilk-plus-rts-with-stats";

  src = cilkRtsSrc;

  buildInputs = [ libtool automake autoconf gcc ];

  preConfigure = "libtoolize && aclocal && automake --add-missing && autoconf";

  enableParallelBuilding = true;

  meta = {
    description = "A version of the Cilk Plus runtime system for GCC that can collect and output stats collected by the work-stealing scheduler.";
    license = "MIT";
    homepage = http://deepsea.inria.fr/;
  };
}

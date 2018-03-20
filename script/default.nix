{ pkgs   ? import <nixpkgs> {},
  stdenv ? pkgs.stdenv,
  fetchurl
}:

stdenv.mkDerivation rec {
  name = "cilk-plus-rts-with-stats-${version}";
  version = "v1.0";

  src = fetchurl {
    url = "https://github.com/deepsea-inria/cilk-plus-rts-with-stats/archive/${version}.tar.gz";
    sha256 = "0c8ahsd4qk9kw6g8bqlifds664h837k1xs71nw4rwb1ysy9dccas";
  };

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
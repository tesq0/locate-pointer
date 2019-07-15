with import <nixpkgs> {};
stdenv.mkDerivation rec {
  name = "animated-pointer";
  buildInputs = [ pkgconfig cairo xorg.libX11 ];
}

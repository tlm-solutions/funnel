{ naersk, src, lib, pkg-config, cmake, protobuf }:

naersk.buildPackage {
  pname = "funnel";
  version = "0.1.0";

  src = ./.;

  cargoSha256 = lib.fakeSha256;

  nativeBuildInputs = [ pkg-config protobuf cmake ];

  meta = with lib; {
    description = "service which takes the incoming data";
    homepage = "https://github.com/dump-dvb/funnel";
  };
}

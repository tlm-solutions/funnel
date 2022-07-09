{ naersk, src, lib, pkg-config, cmake, protobuf, postgresql}:

naersk.buildPackage {
  pname = "funnel";
  version = "0.1.0";

  src = ./.;

  cargoSha256 = lib.fakeSha256;

  nativeBuildInputs = [ pkg-config protobuf cmake postgresql ];

  meta = with lib; {
    description = "service which takes the incoming data";
    homepage = "https://github.com/dump-dvb/funnel";
  };
}

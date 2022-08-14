{ stdenv, lib, pkg-config, openssl, fetchFromGitHub, cmake, protobuf, websocketpp, boost17x, asio, glibc_multi, grpc, which, telegrams}:
let 
  websocketpp = stdenv.mkDerivation rec {
    pname = "websocket++";
    version = "0.8.2";

    src = fetchFromGitHub {
      owner = "zaphoyd";
      repo = "websocketpp";
      rev = version;
      sha256 = "sha256-9fIwouthv2GcmBe/UPvV7Xn9P2o0Kmn2hCI4jCh0hPM=";
    };

    nativeBuildInputs = [ cmake ];

    meta = with lib; {
      homepage = "https://www.zaphoyd.com/websocketpp/";
      description = "C++/Boost Asio based websocket client/server library";
      license = licenses.bsd3;
      platforms = platforms.unix;
      maintainers = with maintainers; [ revol-xut ];
    };
  };

in 
stdenv.mkDerivation {
  pname = "telegram-yeeter";
  version = "0.1.0";

  src = ./.;
  
  phases = [ "unpackPhase" "buildPhase" "installPhase" ];

  buildPhase = ''
    cp ${telegrams}/proto/telegram.proto ./telegram.proto
    mkdir -p src/protobuf
    protoc -I ./ --grpc_out=./src/protobuf/ --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ./telegram.proto
    protoc -I=./ --cpp_out=./src/protobuf/ ./telegram.proto
    cmake .
    make
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp telegram-yeeter $out/bin/
  '';

  nativeBuildInputs = [ pkg-config cmake];

  buildInputs = [ openssl glibc_multi protobuf websocketpp asio grpc which ]; #(asio.override({ stdenv = stdenv; })) ];

  meta = with lib; {
    description = "service which takes the incoming data";
    homepage = "https://github.com/dump-dvb/funnel";
  };
}

{ 
  stdenv, 
  lib, 
  pkg-config, 
  openssl, 
  fetchFromGitHub, 
  cmake, 
  protobuf, 
  asio, 
  prometheus-cpp,
  grpc, 
  which, 
  dump-dvb-rust,
  curlpp,
  curlFull
}:
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

  json_struct = stdenv.mkDerivation rec {
    pname = "json_struct";
    version = "0.0.1";

    src = fetchFromGitHub {
      owner = "jorgen";
      repo = "json_struct";
      rev = "bb94319b1d63058d70dad45114a08950f6b0d977";
      sha256 = "sha256-NWRkCZEpZrkYdzE72lL/1NtM1j54OxfUaWA+0vcM2Do=";
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
  pname = "funnel";
  version = "0.1.0";

  src = ./.;
  
  phases = [ "unpackPhase" "buildPhase" "installPhase" ];

  buildPhase = ''
    cp ${dump-dvb-rust}/proto/telegram.proto ./telegram.proto
    mkdir -p src/protobuf
    protoc -I ./ --grpc_out=./src/protobuf/ --plugin=protoc-gen-grpc=`which grpc_cpp_plugin` ./telegram.proto
    protoc -I=./ --cpp_out=./src/protobuf/ ./telegram.proto
    cmake .
    make
  '';

  installPhase = ''
    mkdir -p $out/bin
    cp funnel $out/bin/
  '';

  nativeBuildInputs = [ pkg-config cmake];

  buildInputs = [ openssl protobuf websocketpp asio grpc which json_struct prometheus-cpp curlFull curlpp ];

  meta = with lib; {
    description = "service which takes the incoming data";
    homepage = "https://github.com/dump-dvb/funnel";
  };
}

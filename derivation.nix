{ 
  stdenv, 
  lib, 
  pkg-config, 
  openssl, 
  fetchFromGitHub, 
  cmake, 
  protobuf, 
  asio,
  websocketpp,
  prometheus-cpp,
  grpc, 
  which, 
  dump-dvb-rust,
  curlpp,
  curlFull
}:
let 
  /*websocketpp = stdenv.mkDerivation rec {
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
  };*/

  json_struct = stdenv.mkDerivation rec {
    pname = "json_struct";
    version = "0.0.1";

    src = fetchFromGitHub {
      owner = "jorgen";
      repo = "json_struct";
      rev = "de80d452b3cc1688dfc2967dbfef5cb501e925d3";
      sha256 = "sha256-lyeM+jkZRfclF7gCWj0ZGvuH5O/UAczPUd7rcVgh+T4=";
    };

    cmakeFlags = ["-DJSON_STRUCT_OPT_BUILD_TESTS=OFF"];

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

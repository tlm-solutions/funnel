{ stdenv
, lib
, pkg-config
, openssl
, cmake
, protobuf
, asio
, websocketpp
, prometheus-cpp
, grpc
, which
, tlms-rust
, curlpp
, curlFull
, json-structs-src
}:
let
  json_struct = stdenv.mkDerivation {
    pname = "json_struct";
    version = "0.0.1";

    src = json-structs-src;

    cmakeFlags = [
      "-DJSON_STRUCT_OPT_BUILD_TESTS=OFF"
      "-DJSON_STRUCT_OPT_BUILD_EXAMPLES=OFF"
      "-DCMAKE_INSTALL_PREFIX='./'"
    ];

    nativeBuildInputs = [ cmake ];

    meta = with lib; {
      homepage = "https://github.com/jorgen/json_structs/";
      description = "json_struct is a header only library that parses JSON to C++ structs and serializing structs to JSON.";
      platforms = platforms.unix;
      maintainers = with maintainers; [ revol-xut ];
    };
  };
in
stdenv.mkDerivation {
  pname = "funnel";
  version = "0.2.0";

  src = ./.;

  phases = [ "unpackPhase" "buildPhase" "installPhase" ];

  buildPhase = ''
    cp ${tlms-rust}/proto/telegram.proto ./telegram.proto
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

  nativeBuildInputs = [ pkg-config cmake ];

  buildInputs = [ openssl protobuf websocketpp asio grpc which json_struct prometheus-cpp curlFull curlpp ];

  meta = with lib; {
    description = "service which takes the incoming data";
    homepage = "https://github.com/tlm-solutions/funnel";
  };
}

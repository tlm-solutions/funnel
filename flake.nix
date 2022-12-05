{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-22.11";

    utils = {
      url = "github:numtide/flake-utils";
    };

    dump-dvb-rust = {
      url = "github:dump-dvb/dump-dvb.rs";
      flake = false;
    };

    json-structs = {
      url = "github:jorgen/json_struct";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, utils, dump-dvb-rust, json-structs, ... }:
    (utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};

          package = pkgs.callPackage ./derivation.nix {
            #stdenv = pkgs.clang13Stdenv;
            dump-dvb-rust = dump-dvb-rust;
            json-structs-src = json-structs;
          };
        in
        rec {
          checks = packages;
          packages = {
            funnel = package;
            default = package;
          };
        }
      ) // {
      overlays.default = final: prev: {
        inherit (self.packages.${prev.system})
          funnel;
      };
      hydraJobs =
        let
          hydraSystems = [
            "x86_64-linux"
            "aarch64-linux"
          ];
        in
        builtins.foldl'
          (hydraJobs: system:
            builtins.foldl'
              (hydraJobs: pkgName:
                nixpkgs.lib.recursiveUpdate hydraJobs {
                  ${pkgName}.${system} = self.packages.${system}.${pkgName};
                }
              )
              hydraJobs
              (builtins.attrNames self.packages.${system})
          )
          { }
          hydraSystems;
    });
}

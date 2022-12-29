{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-22.11";

    utils = {
      url = "github:numtide/flake-utils";
    };

    tlms-rust = {
      url = "github:tlm-solutions/tlms.rs";
      flake = false;
    };

    json-structs = {
      url = "github:jorgen/json_struct";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, utils, tlms-rust, json-structs, ... }:
    (utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};

          package = pkgs.callPackage ./derivation.nix {
            tlms-rust = tlms-rust;
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

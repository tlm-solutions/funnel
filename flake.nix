{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixos-24.11";

    utils = { url = "github:numtide/flake-utils"; };

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
    (utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};

        package = pkgs.callPackage ./derivation.nix {
          tlms-rust = tlms-rust;
          json-structs-src = json-structs;
        };
      in rec {
        checks = packages;
        packages = {
          funnel = package;
          default = package;
          docs = (pkgs.nixosOptionsDoc {
            options = (nixpkgs.lib.nixosSystem {
              inherit system;
              modules = [ self.nixosModules.default ];
            }).options.TLMS;
          }).optionsCommonMark;
        };
        devShells.default = pkgs.mkShell {
          nativeBuildInputs =
            (with packages.funnel; buildInputs ++ nativeBuildInputs);
        };
      }) // {
        overlays.default = final: prev: {
          inherit (self.packages.${prev.system}) funnel;
        };

        nixosModules = rec {
          default = funnel;
          funnel = import ./nixos-module;
        };

        hydraJobs = let hydraSystems = [ "x86_64-linux" "aarch64-linux" ];
        in builtins.foldl' (hydraJobs: system:
          builtins.foldl' (hydraJobs: pkgName:
            nixpkgs.lib.recursiveUpdate hydraJobs {
              ${pkgName}.${system} = self.packages.${system}.${pkgName};
            }) hydraJobs (builtins.attrNames self.packages.${system})) { }
        hydraSystems;
      });
}

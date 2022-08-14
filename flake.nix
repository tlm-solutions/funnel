{
  inputs = {
    nixpkgs.url = "github:revol-xut/nixpkgs/master";

    utils = {
      url = "github:numtide/flake-utils";
    };

    telegrams = {
      url = "github:dump-dvb/telegrams";
      flake = false;
    };
  };

  outputs = { self, nixpkgs, utils, telegrams, ... }:
    (utils.lib.eachDefaultSystem
      (system:
        let
          pkgs = nixpkgs.legacyPackages.${system};

          package = pkgs.callPackage ./derivation.nix {
            #stdenv = pkgs.clang13Stdenv;
            telegrams = telegrams;
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

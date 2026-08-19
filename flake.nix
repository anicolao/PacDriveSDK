{
  description = "A development environment for the USB Button tool";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};
      in
      {
        devShells.default = pkgs.mkShell {
          name = "usb-button-dev";

          # Add the required packages to the development environment
          buildInputs = [
            pkgs.gcc
            pkgs.gnumake
            pkgs.hidapi
            pkgs.pkg-config
          ];
        };
      }
    );
}

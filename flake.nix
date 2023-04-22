{
  description = "EasySplash";

  inputs = {
    nixpkgs.url = "nixpkgs/nixos-22.11";
    rust.url = "github:nix-community/fenix";
  };

  outputs = { nixpkgs, rust, ... }:
    let
      forAllSystems = nixpkgs.lib.genAttrs nixpkgs.lib.systems.flakeExposed;
    in
    {
      devShell = forAllSystems
        (system:
          let
            pkgs = nixpkgs.legacyPackages.${system};

            rust-toolchain = with rust.packages.${system};
              let
                msrvToolchain = toolchainOf {
                  channel = "1.66.0";
                  sha256 = "sha256-S7epLlflwt0d1GZP44u5Xosgf6dRrmr8xxC+Ml2Pq7c=";
                };
              in
              combine [
                (msrvToolchain.withComponents [ "rustc" "cargo" "rust-src" "clippy" ])
                (latest.withComponents [ "rustfmt" "rust-analyzer" ])
              ];
          in
          pkgs.mkShell {
            buildInputs = with pkgs; [
              glib
              gst_all_1.gst-libav
              gst_all_1.gst-plugins-base
              gst_all_1.gst-plugins-good
              gst_all_1.gstreamer
              pkg-config
              rust-toolchain
              systemd
            ];
          }
        );
    };
}

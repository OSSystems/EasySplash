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
                  channel = "1.59.0";
                  sha256 = "sha256-4IUZZWXHBBxcwRuQm9ekOwzc0oNqH/9NkI1ejW7KajU=";
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

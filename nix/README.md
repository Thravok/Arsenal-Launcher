# Arsenal Launcher Nix Packaging

Arsenal Launcher can be built with Nix via this repository's flake.

## Binary cache

Optionally create an [arsenal](https://cachix.org/) Cachix cache, then add to your Nix config:

```nix
nix.settings = {
  trusted-substituters = [ "https://arsenal.cachix.org" ];
  trusted-public-keys = [
    # paste the public key from `cachix show arsenal` here
  ];
};
```

Also uncomment the matching `nixConfig` entries in `flake.nix`.

## Flake

After adding `github:Thravok/Arsenal-Launcher` to your flake inputs:

```nix
{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    arsenal = {
      url = "github:Thravok/Arsenal-Launcher";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    { nixpkgs, arsenal, ... }:
    {
      nixosConfigurations.foo = nixpkgs.lib.nixosSystem {
        system = "x86_64-linux";
        modules = [
          {
            environment.systemPackages = [ arsenal.packages.${pkgs.system}.arsenal ];
          }
        ];
      };
    };
}
```

Or via overlay:

```nix
{
  nixpkgs.overlays = [ arsenal.overlays.default ];
  environment.systemPackages = [ pkgs.arsenal ];
}
```

## Ad-hoc usage

```bash
nix run github:Thravok/Arsenal-Launcher
nix shell github:Thravok/Arsenal-Launcher
nix profile install github:Thravok/Arsenal-Launcher
```

## Packages

- `arsenal` — wrapped build with runtime libs/JDKs for Minecraft
- `arsenal-unwrapped` — minimal build for advanced customization

The wrapped package accepts the same overrides as upstream Prism packaging (`jdks`, `msaClientID`, `gamemodeSupport`, etc.).

# ESPHome Aldes EasyHome

ESPHome integration for Aldes EasyHome VMC over proprietary iBUS.

## Branches
- `main`: stable releases
- `dev/ibus-rx`: receive-only iBUS framing and diagnostics

## Development status
The receiver is under development. No commands are transmitted in this branch.

## Design
ESPHome device YAML imports `packages/easyhome.yaml` from this repository. That package loads a custom `aldes_ibus` external component. Network credentials and API keys stay in the local ESPHome configuration.

## License
GPL-3.0, consistent with this repository's existing LICENSE.
